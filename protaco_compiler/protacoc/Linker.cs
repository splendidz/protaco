// Linker.cs. Cross-definition rules: name resolution (message or enum), cycle check, topological sort, field semantics,
// enum values, fixed block layout. Input file order is irrelevant (all files are loaded first, then looked up by name).
using System;
using System.Collections.Generic;
using System.Linq;

namespace Protacoc
{
    internal static class Linker
    {
        private static readonly System.Text.RegularExpressions.Regex DoubleText =
            new System.Text.RegularExpressions.Regex("^-?[0-9]+(\\.[0-9]+)?([eE][-+]?[0-9]+)?$");

        // Keys that clash with generated names (C++ accessors and members, C# properties). Case-insensitive.
        private static readonly HashSet<string> GeneratedNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "ext", "buf", "cache", "serialize", "msgname", "msgversion", "msgminversion", "msgschemahash", "schemahashes",
            "setenvelope", "getenvelope", "isschemakey", "visitvar", "visittext", "tostring", "kfixedoffset",
            "kfixedsize", "kallowextension", "equals", "gethashcode", "gettype", "getid", "setid", "getcorr", "setcorr",
            "gettimestamp", "settimestamp", "settimestampasnow", "trygettimefromtimestamp", "trygettimefromtimestampparts", "protaco", "view", "tryview",
        };

        // Where a type appears. Decides which kinds are allowed.
        private enum Place
        {
            Field,
            Element,
            MapKey,
            MapValue,
            ArrayElement,
        }

        // Fixed block limit: the largest .NET byte array (Protaco.cs Codec.MaxFrameBytes) minus room for the frame header.
        public const long MaxFixedBlock = 0x7FFF0000;

        private static readonly System.Text.RegularExpressions.Regex VersionNamespace =
            new System.Text.RegularExpressions.Regex("^v[0-9]+$");

        // Must equal the runtime limit (Protaco.hpp kMaxDepth, Protaco.cs Codec.MaxDepth). Self references are not counted:
        // the runtime checks their depth while decoding.
        public const int MaxDepth = 100;

        // Must equal the runtime limit (Protaco.hpp kMaxSchemaVersions, Protaco.cs Codec.MaxSchemaVersions).
        public const int MaxVersions = 255;

        private static readonly HashSet<string> CppKeywords = new HashSet<string>
        {
            "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
            "char", "char16_t", "char32_t", "class", "compl", "const", "constexpr", "const_cast", "continue", "decltype",
            "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
            "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
            "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register",
            "reinterpret_cast", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
            "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
            "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
        };

        private static readonly HashSet<string> CsKeywords = new HashSet<string>
        {
            "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked", "class", "const",
            "continue", "decimal", "default", "delegate", "do", "double", "else", "enum", "event", "explicit", "extern",
            "false", "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit", "in", "int", "interface",
            "internal", "is", "lock", "long", "namespace", "new", "null", "object", "operator", "out", "override",
            "params", "private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed", "short",
            "sizeof", "stackalloc", "static", "string", "struct", "switch", "this", "throw", "true", "try", "typeof",
            "uint", "ulong", "unchecked", "unsafe", "ushort", "using", "virtual", "void", "volatile", "while",
        };

        // Messages in dependency order (referenced messages first) and enums by name. False on error.
        public static bool Link(List<SchemaDef> defs, Diag diag, out List<MessageDef> order, out List<EnumDef> enums)
        {
            order = null;
            enums = defs.OfType<EnumDef>().OrderBy(e => e.Name, StringComparer.Ordinal).ToList();
            List<MessageDef> messages = defs.OfType<MessageDef>().ToList();
            Dictionary<string, SchemaDef> byName = new Dictionary<string, SchemaDef>();
            foreach (SchemaDef def in defs)
            {
                SchemaDef other;
                if (byName.TryGetValue(def.Name, out other))
                {
                    diag.Error(def.Pos, Codes.DupMessage, "duplicate name '" + def.Name + "' (first defined in " + other.FilePath + ")");
                    continue;
                }
                byName.Add(def.Name, def);
            }
            foreach (EnumDef enumDef in enums)
            {
                // C# helper class <Enum>Names must not collide with another definition.
                if (byName.ContainsKey(enumDef.Name + "Names"))
                    diag.Error(enumDef.Pos, Codes.DupMessage, "'" + enumDef.Name + "Names' is the C# name helper of enum '" + enumDef.Name + "'; rename the other definition");
            }
            CheckNamespaces(defs, diag);
            foreach (EnumDef enumDef in enums)
                CheckEnum(enumDef, diag);
            foreach (ConstDef constDef in defs.OfType<ConstDef>())
                CheckConst(constDef, diag);
            foreach (MessageDef message in messages)
            {
                CheckFields(message, byName, diag);
                CheckVersions(message, diag);
            }
            CheckNestedVersions(messages, diag);
            foreach (MessageDef message in messages.Where(m => m.Fixed))
                CheckFixedMessage(message, diag);
            if (diag.ErrorCount != 0)
                return false;
            order = TopoSort(messages, diag);
            if (order == null)
                return false;
            // Dependencies come first, so depth and fingerprints of referenced messages are ready.
            foreach (MessageDef message in order)
            {
                if (!Layout(message, diag))
                    return false;
                message.Depth = message.Dependencies.Count == 0 ? 0 : 1 + message.Dependencies.Max(d => d.Depth);
                if (message.Depth > MaxDepth)
                    diag.Error(message.Pos, Codes.TooDeep, "'" + message.Name + "' nests messages " + message.Depth + " levels deep. The maximum is " + MaxDepth);
            }
            return diag.ErrorCount == 0;
        }

        private static void CheckDefName(SchemaDef def, string kind, Diag diag)
        {
            if (VersionNamespace.IsMatch(def.Name))
                diag.Error(def.Pos, Codes.ReservedName, kind + " name '" + def.Name + "' is reserved for version namespaces (v1, v2, ...)");
            if (CppKeywords.Contains(def.Name) || CsKeywords.Contains(def.Name) || PyEmitter.Keywords.Contains(def.Name))
                diag.Error(def.Pos, Codes.Reserved, "'" + def.Name + "' is a C++, C# or Python keyword");
        }

        // Version rules. README.md "Versioning".
        private static bool CheckVersionRange(SchemaDef def, string kind, Diag diag)
        {
            CheckDefName(def, kind, diag);
            if (def.MinVersion > def.Version)
            {
                diag.Error(def.Pos, Codes.BadVersion, "min_version " + def.MinVersion + " is above version " + def.Version);
                return false;
            }
            if (def.Version - def.MinVersion + 1 > MaxVersions)
                diag.Error(def.Pos, Codes.BadVersion, "min_version " + def.MinVersion + " to version " + def.Version + " spans more than " + MaxVersions + " versions. Raise min_version");
            return true;
        }

        // added/removed of one field or enum value.
        private static void CheckLifetime(string key, int added, int removed, bool keepsBytes, SchemaDef def, SourcePos pos, Diag diag)
        {
            if (added > def.Version)
                diag.Error(pos, Codes.BadVersion, "'" + key + "' added in " + added + ", above version " + def.Version);
            if (removed == 0)
                return;
            if (removed > def.Version)
                diag.Error(pos, Codes.BadVersion, "'" + key + "' removed in " + removed + ", above version " + def.Version);
            else if (removed <= added)
                diag.Error(pos, Codes.BadVersion, "'" + key + "' removed in " + removed + ", not after added " + added);
            else if (!keepsBytes && removed <= def.MinVersion)
                diag.Error(pos, Codes.DeadLine, "'" + key + "' removed in " + removed + " and min_version is " + def.MinVersion + ": no supported version has it. Delete the line");
        }

        private static void CheckVersions(MessageDef message, Diag diag)
        {
            if (!CheckVersionRange(message, "message", diag))
                return;
            foreach (FieldDef field in message.Fields)
                CheckLifetime(field.Key, field.Added, field.Removed, field.Type.IsFixed, message, field.Pos, diag);
        }

        // Enum values: int32, unique keys and numbers (removed numbers stay taken), at least one value in the latest version.
        // Removed value lines stay: they keep the number taken and the name for ToString of old frames.
        private static void CheckEnum(EnumDef enumDef, Diag diag)
        {
            if (!CheckVersionRange(enumDef, "enum", diag))
                return;
            HashSet<string> keys = new HashSet<string>();
            HashSet<string> pyNames = new HashSet<string>();
            Dictionary<int, string> numbers = new Dictionary<int, string>();
            foreach (EnumValueDef value in enumDef.Values)
            {
                if (!keys.Add(value.Key))
                    diag.Error(value.Pos, Codes.DupKey, "duplicate enum value key '" + value.Key + "'");
                else if (!pyNames.Add(PyEmitter.Name(value.Key)))
                    diag.Error(value.Pos, Codes.NameClash, "Python name '" + PyEmitter.Name(value.Key) + "' clashes (a Python keyword gets a trailing '_')");
                if (CppKeywords.Contains(value.Key) || CsKeywords.Contains(value.Key))
                    diag.Error(value.Pos, Codes.Reserved, "'" + value.Key + "' is a C++ or C# keyword");
                string other;
                if (numbers.TryGetValue(value.Number, out other))
                    diag.Error(value.Pos, Codes.DupNumber, "'" + value.Key + "' reuses number " + value.Number + " of '" + other + "'. Numbers are never reused, removed values included");
                else
                    numbers.Add(value.Number, value.Key);
                CheckLifetime(value.Key, value.Added, value.Removed, true, enumDef, value.Pos, diag);
            }
            if (enumDef.First == null)
                diag.Error(enumDef.Pos, Codes.MissingTop, "enum '" + enumDef.Name + "' has no value in version " + enumDef.Version);
        }

        // Const values: unique keys, no keywords, not the C# class name (a member cannot share its class name).
        private static void CheckConst(ConstDef constDef, Diag diag)
        {
            CheckDefName(constDef, "const", diag);
            HashSet<string> keys = new HashSet<string>();
            HashSet<string> pyNames = new HashSet<string>();
            foreach (ConstValueDef value in constDef.Values)
            {
                if (!keys.Add(value.Key))
                    diag.Error(value.Pos, Codes.DupKey, "duplicate const key '" + value.Key + "'");
                else if (!pyNames.Add(PyEmitter.Name(value.Key)))
                    diag.Error(value.Pos, Codes.NameClash, "Python name '" + PyEmitter.Name(value.Key) + "' clashes (a Python keyword gets a trailing '_')");
                if (CppKeywords.Contains(value.Key) || CsKeywords.Contains(value.Key))
                    diag.Error(value.Pos, Codes.Reserved, "'" + value.Key + "' is a C++ or C# keyword");
                if (value.Key == constDef.Name)
                    diag.Error(value.Pos, Codes.NameClash, "const key '" + value.Key + "' equals the const name (C# class member)");
            }
            if (constDef.Values.Count == 0)
                diag.Error(constDef.Pos, Codes.MissingTop, "const '" + constDef.Name + "' has no value");
        }

        // fixed: true. One layout of fixed fields only, so the message can sit inline in another message's fixed block.
        // No ext: it would need a variable block. The fingerprint carries ext 0.
        private static void CheckFixedMessage(MessageDef message, Diag diag)
        {
            if (message.AllowExtensionSet && message.AllowExtension)
                diag.Error(message.Pos, Codes.FixedMessage, "'" + message.Name + "' is fixed: true and cannot allow extension data");
            message.AllowExtension = false;
            if (message.UsesVersions)
                diag.Error(message.Pos, Codes.FixedMessage, "'" + message.Name + "' is fixed: true and has one layout: no min_version, added or removed");
            foreach (FieldDef field in message.Fields)
            {
                if (field.Type.Named().Any(t => t.Resolved == message))
                    diag.Error(field.Pos, Codes.FixedMessage, "'" + message.Name + "' is fixed: true and cannot contain itself ('" + field.Key + "')");
                else if (!field.Type.IsFixed)
                    diag.Error(field.Pos, Codes.FixedMessage, "'" + field.Key + "' (" + field.Type + ") is a variable field. A fixed: true message has fixed fields "
                        + "only: scalars, string(N), enums, fixed messages and T[N] of those");
            }
        }

        // Nested bodies carry no version, so a message used as a field type has exactly one layout.
        private static void CheckNestedVersions(List<MessageDef> messages, Diag diag)
        {
            HashSet<MessageDef> reported = new HashSet<MessageDef>();
            foreach (MessageDef message in messages)
            {
                foreach (FieldDef field in message.Fields)
                {
                    foreach (TypeRef type in field.Type.Named())
                    {
                        MessageDef target = type.Resolved;
                        if (target == null || !target.UsesVersions || !reported.Add(target))
                            continue;
                        diag.Error(target.Pos, Codes.NestedVersion, "'" + target.Name + "' is used as a field type (" + message.Name + "." + field.Key
                            + ") and cannot use min_version, added or removed. Nested bodies carry no version: add a new message instead");
                    }
                }
            }
        }

        // One run = one namespace, so output files stay flat (<Name>.g.h, <Name>.enum.h) and references need no qualification.
        private static void CheckNamespaces(List<SchemaDef> defs, Diag diag)
        {
            List<SchemaDef> sorted = defs.OrderBy(m => m.FilePath, StringComparer.Ordinal).ToList();
            foreach (SchemaDef def in sorted)
            {
                foreach (string part in def.Namespace.Split('.'))
                {
                    if (CppKeywords.Contains(part) || CsKeywords.Contains(part))
                        diag.Error(def.Pos, Codes.BadNamespace, "namespace '" + def.Namespace + "' uses the keyword '" + part + "'");
                }
            }
            if (sorted.Count == 0)
                return;
            SchemaDef first = sorted[0];
            foreach (SchemaDef def in sorted.Skip(1))
            {
                if (def.Namespace != first.Namespace)
                    diag.Error(def.Pos, Codes.NamespaceMismatch, "namespace '" + def.Namespace + "' differs from '" + first.Namespace + "' (" + first.FileName + "). Generate each namespace in its own run and output folder");
            }
        }

        private static void CheckFields(MessageDef message, Dictionary<string, SchemaDef> byName, Diag diag)
        {
            HashSet<string> keys = new HashSet<string>();
            HashSet<string> csNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            HashSet<string> pyNames = new HashSet<string>();
            foreach (FieldDef field in message.Fields)
            {
                SourcePos pos = field.Pos;
                if (!keys.Add(field.Key))
                    diag.Error(pos, Codes.DupKey, "duplicate field key '" + field.Key + "'");
                else if (!pyNames.Add(PyEmitter.Name(field.Key)))
                    diag.Error(pos, Codes.NameClash, "Python attribute '" + PyEmitter.Name(field.Key) + "' clashes (a Python keyword gets a trailing '_')");
                CheckName(message, field, csNames, diag);
                if (!ResolveType(field.Type, field, byName, diag, Place.Field) || !CheckSelf(message, field, diag))
                    continue;
                if (field.Required && field.Type.IsFixed)
                    diag.Error(pos, Codes.RequiredFixed, "'" + field.Key + "' is a fixed field and is always present. required is not allowed (validate the value in receiving code)");
                CheckDefault(field, diag);
            }
        }

        private static void CheckName(MessageDef message, FieldDef field, HashSet<string> csNames, Diag diag)
        {
            if (field.Key.StartsWith("_"))
                diag.Error(field.Pos, Codes.Reserved, "keys starting with '_' are reserved");
            if (field.Key.StartsWith("get_") || field.Key.StartsWith("set_") || field.Key.StartsWith("mutable_") || field.Key.StartsWith("has_")
                || field.Key.StartsWith("clear_"))
                diag.Error(field.Pos, Codes.Reserved, "keys starting with 'get_', 'set_', 'mutable_', 'has_' or 'clear_' clash with generated accessors");
            if (GeneratedNames.Contains(field.Key))
                diag.Error(field.Pos, Codes.Reserved, "'" + field.Key + "' is reserved by generated code");
            if (CppKeywords.Contains(field.Key) || CsKeywords.Contains(field.Key))
                diag.Error(field.Pos, Codes.Reserved, "'" + field.Key + "' is a C++ or C# keyword");
            if (field.Key.Length > 255)
                diag.Error(field.Pos, Codes.Reserved, "key longer than 255 bytes");
            string csName = Naming.CsProperty(field.Key);
            if (csName == message.Name)
                diag.Error(field.Pos, Codes.NameClash, "C# property name '" + csName + "' equals the message name");
            else if (!csNames.Add(csName))
                diag.Error(field.Pos, Codes.NameClash, "C# property name '" + csName + "' clashes (keys differ only in case)");
        }

        // A message may refer to itself as list<Self> or as one Self field, which is optional (absent until set).
        // Not inside a map: libstdc++ std::unordered_map needs a complete value type. Never required.
        private static bool CheckSelf(MessageDef message, FieldDef field, Diag diag)
        {
            TypeRef type = field.Type;
            bool single = type.Kind == TypeKind.Message && type.Resolved == message;
            bool list = type.Kind == TypeKind.List && type.Element.Resolved == message;
            if (!single && !list)
            {
                if (!type.Named().Any(t => t.Resolved == message))
                    return true;
                diag.Error(field.Pos, Codes.Cycle, "'" + message.Name + "' can refer to itself only as list<" + message.Name + "> or a single "
                    + message.Name + " field, not " + type);
                return false;
            }
            if (field.Required)
                diag.Error(field.Pos, Codes.RequiredSelf, "'" + field.Key + "' has the type of its own message and cannot be required");
            field.Optional = single;
            return true;
        }

        // Named types become Message or Enum here. False if the type cannot be used.
        // list elements: no bool, string(N), list or map. map keys: int, int64, uint, uint64, string, enum.
        // map values: any type but string(N); a list or map value follows the same rules again.
        private static bool ResolveType(TypeRef type, FieldDef field, Dictionary<string, SchemaDef> byName, Diag diag, Place place)
        {
            if (type.Kind == TypeKind.Array)
            {
                if (place != Place.Field)
                {
                    diag.Error(field.Pos, Codes.BadElement, "a fixed-length array as a list element, map key or value, or array element is not supported. Wrap it in a message");
                    return false;
                }
                if (!ResolveType(type.Element, field, byName, diag, Place.ArrayElement))
                    return false;
                TypeKind element = type.Element.Kind;
                if (element == TypeKind.String || element == TypeKind.Bytes || element == TypeKind.List || element == TypeKind.Map
                    || (element == TypeKind.Message && !type.Element.Resolved.Fixed))
                {
                    diag.Error(field.Pos, Codes.BadElement, type.Element + "[N] is not supported. Allowed elements: bool, int, int64, uint, uint64, double, string(N), enum, "
                        + "and messages with fixed: true");
                    return false;
                }
                return ResolveLength(type, field, byName, diag);
            }
            if (type.Kind == TypeKind.List)
            {
                if (place == Place.Element || place == Place.MapKey)
                {
                    diag.Error(field.Pos, Codes.BadElement, (place == Place.Element ? "list of lists" : "list as a map key") + " is not supported. Wrap the list in a message");
                    return false;
                }
                TypeRef element = type.Element;
                if (element.Kind == TypeKind.Bool || element.Kind == TypeKind.FixedString)
                {
                    diag.Error(field.Pos, Codes.BadElement, "list<" + element + "> is not supported. Allowed elements: int, int64, uint, uint64, double, string, bytes, enum, message");
                    return false;
                }
                return ResolveType(element, field, byName, diag, Place.Element);
            }
            if (type.Kind == TypeKind.Map)
            {
                if (place == Place.Element || place == Place.MapKey)
                {
                    diag.Error(field.Pos, Codes.BadElement, (place == Place.Element ? "list of maps" : "map as a map key") + " is not supported. Wrap the map in a message");
                    return false;
                }
                if (!ResolveType(type.Key, field, byName, diag, Place.MapKey) || !ResolveType(type.Element, field, byName, diag, Place.MapValue))
                    return false;
                TypeKind key = type.Key.Kind;
                if (key != TypeKind.Int && key != TypeKind.Int64 && key != TypeKind.UInt && key != TypeKind.UInt64 && key != TypeKind.String && key != TypeKind.Enum)
                {
                    diag.Error(field.Pos, Codes.BadElement, "map key " + type.Key + " is not supported. Allowed keys: int, int64, uint, uint64, string, enum");
                    return false;
                }
                if (type.Element.Kind == TypeKind.FixedString)
                {
                    diag.Error(field.Pos, Codes.BadElement, "map value " + type.Element + " is not supported. Use string");
                    return false;
                }
                return true;
            }
            if (type.Kind != TypeKind.Message)
                return true;
            SchemaDef target;
            if (!byName.TryGetValue(type.TypeName, out target))
            {
                diag.Error(field.Pos, Codes.UnknownRef, "unknown type '" + type.TypeName + "'. Expected " + type.TypeName + ".yaml or "
                    + type.TypeName + Loader.EnumSuffix + " in an -I folder");
                return false;
            }
            EnumDef enumDef = target as EnumDef;
            if (enumDef != null)
            {
                type.Kind = TypeKind.Enum;
                type.ResolvedEnum = enumDef;
                return true;
            }
            if (target is ConstDef)
            {
                diag.Error(field.Pos, Codes.BadType, "'" + type.TypeName + "' is a const set, not a type");
                return false;
            }
            type.Resolved = (MessageDef)target;
            return true;
        }

        // N of T[N]: a number, Const.Key or Enum.Value. The element count is 1 or more and the array fits the fixed block limit.
        private static bool ResolveLength(TypeRef type, FieldDef field, Dictionary<string, SchemaDef> byName, Diag diag)
        {
            string text = type.LengthText;
            long length;
            if (Loader.IsIntText(text))
            {
                if (!long.TryParse(text, out length))
                    length = -1;
            }
            else
            {
                string[] parts = text.Split('.');
                SchemaDef target;
                byName.TryGetValue(parts[0], out target);
                ConstDef constDef = target as ConstDef;
                EnumDef enumDef = target as EnumDef;
                if (constDef == null && enumDef == null)
                {
                    diag.Error(field.Pos, Codes.UnknownRef, "unknown const or enum '" + parts[0] + "' in array length '" + type + "'. Expected "
                        + parts[0] + Loader.ConstSuffix + " or " + parts[0] + Loader.EnumSuffix + " in an -I folder");
                    return false;
                }
                ConstValueDef constValue = constDef?.Values.FirstOrDefault(v => v.Key == parts[1]);
                EnumValueDef enumValue = enumDef?.Values.FirstOrDefault(v => v.Key == parts[1]);
                if (constValue == null && enumValue == null)
                {
                    diag.Error(field.Pos, Codes.UnknownRef, "'" + parts[1] + "' is not a value of " + (constDef != null ? "const " : "enum ") + parts[0]);
                    return false;
                }
                length = constValue != null ? constValue.Number : enumValue.Number;
            }
            if (length < 1 || length * type.Element.FixedSize > MaxFixedBlock)
            {
                diag.Error(field.Pos, Codes.BadLength, "array length of '" + type + "' is " + length + ". It must be 1 or more, and the array at most " + MaxFixedBlock + " bytes");
                return false;
            }
            type.ArrayLength = (int)length;
            return true;
        }

        private static void CheckDefault(FieldDef field, Diag diag)
        {
            if (field.DefaultText == null)
                return;
            string text = field.DefaultText;
            bool ok;
            // An array default fills every element.
            TypeRef type = field.Type.Kind == TypeKind.Array ? field.Type.Element : field.Type;
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    ok = !field.DefaultQuoted && (text == "true" || text == "false");
                    break;
                case TypeKind.Int:
                    int i32;
                    ok = !field.DefaultQuoted && Loader.IsIntText(text) && int.TryParse(text, out i32);
                    break;
                case TypeKind.Int64:
                    long i64;
                    ok = !field.DefaultQuoted && Loader.IsIntText(text) && long.TryParse(text, out i64) && i64 != long.MinValue;
                    break;
                case TypeKind.UInt:
                    uint u32;
                    ok = !field.DefaultQuoted && Loader.IsIntText(text) && !text.StartsWith("-") && uint.TryParse(text, out u32);
                    break;
                case TypeKind.UInt64:
                    ulong u64;
                    ok = !field.DefaultQuoted && Loader.IsIntText(text) && !text.StartsWith("-") && ulong.TryParse(text, out u64);
                    break;
                case TypeKind.Double:
                    // Only forms usable verbatim as C++ and C# literals (no Infinity or NaN, no overflow to infinity).
                    double d;
                    ok = !field.DefaultQuoted && DoubleText.IsMatch(text)
                        && double.TryParse(text, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out d)
                        && !double.IsInfinity(d);
                    break;
                case TypeKind.Enum:
                    ok = !field.DefaultQuoted && type.ResolvedEnum.Alive.Exists(v => v.Key == text);
                    if (!ok)
                    {
                        diag.Error(field.Pos, Codes.BadDefault, "default '" + text + "' is not a value of enum " + type.TypeName + " (version "
                            + type.ResolvedEnum.Version + "). Write the value name without quotes");
                        return;
                    }
                    break;
                case TypeKind.String:
                case TypeKind.FixedString:
                    // Non-ASCII literal bytes depend on the compiler charset. ASCII only.
                    if (!Naming.IsAscii(text))
                    {
                        diag.Error(field.Pos, Codes.BadDefault, "string defaults must be ASCII");
                        return;
                    }
                    ok = type.Kind == TypeKind.String || text.Length <= type.FixedLength;
                    if (!ok)
                    {
                        diag.Error(field.Pos, Codes.BadDefault, "default is longer than string(" + type.FixedLength + ")");
                        return;
                    }
                    break;
                default:
                    diag.Error(field.Pos, Codes.BadDefault, "list, map, bytes and message fields cannot have a default (they start empty)");
                    return;
            }
            if (!ok)
                diag.Error(field.Pos, Codes.BadDefault, "default '" + text + "' does not match type " + field.Type + " (numbers and bools without quotes, integers without '+' or leading zeros)");
        }

        // Depth-first cycle search. Appends in completion order (dependencies first).
        private static List<MessageDef> TopoSort(List<MessageDef> messages, Diag diag)
        {
            List<MessageDef> order = new List<MessageDef>();
            Dictionary<MessageDef, int> state = new Dictionary<MessageDef, int>();   // 1 visiting, 2 done
            List<MessageDef> stack = new List<MessageDef>();
            bool ok = true;
            foreach (MessageDef message in messages.OrderBy(m => m.Name, StringComparer.Ordinal))
            {
                if (!Visit(message, state, stack, order, diag))
                    ok = false;
            }
            return ok ? order : null;
        }

        private static bool Visit(MessageDef message, Dictionary<MessageDef, int> state, List<MessageDef> stack, List<MessageDef> order, Diag diag)
        {
            int current;
            state.TryGetValue(message, out current);
            if (current == 2)
                return true;
            if (current == 1)
            {
                int start = stack.IndexOf(message);
                IEnumerable<string> path = stack.Skip(start).Select(m => m.Name).Concat(new[] { message.Name });
                diag.Error(message.Pos, Codes.Cycle, "reference cycle: " + string.Join(" -> ", path)
                    + ". Only a message that refers to itself is supported (list<" + message.Name + "> or a single " + message.Name + " field)");
                return false;
            }
            state[message] = 1;
            stack.Add(message);
            bool ok = true;
            foreach (MessageDef dep in DirectDependencies(message))
            {
                if (!Visit(dep, state, stack, order, diag))
                {
                    ok = false;
                    break;
                }
            }
            stack.RemoveAt(stack.Count - 1);
            state[message] = 2;
            message.Dependencies = DirectDependencies(message).ToList();
            message.EnumDependencies = EnumDependencies(message).ToList();
            order.Add(message);
            return ok;
        }

        private static IEnumerable<MessageDef> DirectDependencies(MessageDef message)
        {
            HashSet<MessageDef> seen = new HashSet<MessageDef>();
            foreach (FieldDef field in message.Fields)
            {
                foreach (TypeRef type in field.Type.Named())
                {
                    if (type.Resolved != null && type.Resolved != message && seen.Add(type.Resolved))
                        yield return type.Resolved;
                }
            }
        }

        private static IEnumerable<EnumDef> EnumDependencies(MessageDef message)
        {
            HashSet<EnumDef> seen = new HashSet<EnumDef>();
            foreach (FieldDef field in message.Fields)
            {
                foreach (TypeRef type in field.Type.Named())
                {
                    if (type.ResolvedEnum != null && seen.Add(type.ResolvedEnum))
                        yield return type.ResolvedEnum;
                }
            }
        }

        // Serialization order: added, then declaration. A later version only appends fixed fields, so the fixed block
        // of version k is a prefix of every newer one and an offset never changes. Removed fixed fields keep their bytes.
        // Layout changes are caught by the schema fingerprint.
        private static bool Layout(MessageDef message, Diag diag)
        {
            message.Physical = message.Fields.OrderBy(f => f.Added).ThenBy(f => f.Index).ToList();
            long offset = 0;
            foreach (FieldDef field in message.Physical.Where(f => f.Type.IsFixed))
            {
                field.FixedOffset = (int)offset;
                offset += field.Type.FixedSize;
                if (offset > MaxFixedBlock)
                {
                    diag.Error(message.Pos, Codes.BadLength, "the fixed block of '" + message.Name + "' passes " + MaxFixedBlock + " bytes at '" + field.Key + "'");
                    return false;
                }
            }
            message.Views.Clear();
            for (int version = message.MinVersion; version <= message.Version; ++version)
            {
                VersionView view = new VersionView { Version = version };
                view.Alive = message.Fields.Where(f => f.AliveAt(version)).ToList();
                view.FixedSlots = message.Physical.Where(f => f.Type.IsFixed && f.Added <= version).ToList();
                view.AliveVar = message.Physical.Where(f => !f.Type.IsFixed && f.AliveAt(version)).ToList();
                view.FixedSize = view.FixedSlots.Sum(f => f.Type.FixedSize);
                view.SchemaHash = SchemaFingerprint.Of(message, version);
                message.Views.Add(view);
            }
            return true;
        }
    }
}
