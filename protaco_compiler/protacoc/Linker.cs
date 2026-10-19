// Linker.cs. Cross-message rules: name resolution, cycle check, topological sort, field semantics, fixed block layout.
// Input file order is irrelevant (all files are loaded first, then looked up by name).
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
            "gettimestamp", "settimestamp", "settimestampasnow", "trygettimefromtimestamp",
        };

        private static readonly System.Text.RegularExpressions.Regex VersionNamespace =
            new System.Text.RegularExpressions.Regex("^v[0-9]+$");

        // Must equal the runtime limit (Protaco.hpp kMaxDepth, Protaco.cs Codec.MaxDepth).
        public const int MaxDepth = 8;

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

        // Returns dependency order (referenced messages first), or null on error.
        public static List<MessageDef> Link(List<MessageDef> messages, Diag diag)
        {
            Dictionary<string, MessageDef> byName = new Dictionary<string, MessageDef>();
            foreach (MessageDef message in messages)
            {
                MessageDef other;
                if (byName.TryGetValue(message.Name, out other))
                {
                    diag.Error(message.Pos, Codes.DupMessage, "duplicate message name '" + message.Name + "' (first defined in " + other.FilePath + ")");
                    continue;
                }
                byName.Add(message.Name, message);
            }
            CheckNamespaces(messages, diag);
            foreach (MessageDef message in messages)
            {
                CheckFields(message, byName, diag);
                CheckVersions(message, diag);
            }
            CheckNestedVersions(messages, diag);
            if (diag.ErrorCount != 0)
                return null;
            List<MessageDef> order = TopoSort(messages, diag);
            if (order == null)
                return null;
            // Dependencies come first, so depth and fingerprints of referenced messages are ready.
            foreach (MessageDef message in order)
            {
                Layout(message);
                message.Depth = message.Dependencies.Count == 0 ? 0 : 1 + message.Dependencies.Max(d => d.Depth);
                if (message.Depth > MaxDepth)
                    diag.Error(message.Pos, Codes.TooDeep, "'" + message.Name + "' nests messages " + message.Depth + " levels deep. The maximum is " + MaxDepth);
            }
            return diag.ErrorCount == 0 ? order : null;
        }

        // Version rules. README.md "Versioning".
        private static void CheckVersions(MessageDef message, Diag diag)
        {
            if (VersionNamespace.IsMatch(message.Name))
                diag.Error(message.Pos, Codes.ReservedName, "message name '" + message.Name + "' is reserved for version namespaces (v1, v2, ...)");
            if (message.MinVersion > message.Version)
            {
                diag.Error(message.Pos, Codes.BadVersion, "min_version " + message.MinVersion + " is above version " + message.Version);
                return;
            }
            if (message.Version - message.MinVersion + 1 > MaxVersions)
                diag.Error(message.Pos, Codes.BadVersion, "min_version " + message.MinVersion + " to version " + message.Version + " spans more than " + MaxVersions + " versions. Raise min_version");
            foreach (FieldDef field in message.Fields)
            {
                if (field.Added > message.Version)
                    diag.Error(field.Pos, Codes.BadVersion, "'" + field.Key + "' added in " + field.Added + ", above version " + message.Version);
                if (field.Removed == 0)
                    continue;
                if (field.Removed > message.Version)
                    diag.Error(field.Pos, Codes.BadVersion, "'" + field.Key + "' removed in " + field.Removed + ", above version " + message.Version);
                else if (field.Removed <= field.Added)
                    diag.Error(field.Pos, Codes.BadVersion, "'" + field.Key + "' removed in " + field.Removed + ", not after added " + field.Added);
                else if (!field.Type.IsFixed && field.Removed <= message.MinVersion)
                    diag.Error(field.Pos, Codes.DeadLine, "'" + field.Key + "' removed in " + field.Removed + " and min_version is " + message.MinVersion + ": no supported version has it. Delete the line");
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
                    TypeRef type = field.Type.Kind == TypeKind.List ? field.Type.Element : field.Type;
                    MessageDef target = type.Resolved;
                    if (target == null || !target.UsesVersions || !reported.Add(target))
                        continue;
                    diag.Error(target.Pos, Codes.NestedVersion, "'" + target.Name + "' is used as a field type (" + message.Name + "." + field.Key
                        + ") and cannot use min_version, added or removed. Nested bodies carry no version: add a new message instead");
                }
            }
        }

        // One run = one namespace, so output files stay flat (<Name>.g.h) and references need no qualification.
        private static void CheckNamespaces(List<MessageDef> messages, Diag diag)
        {
            List<MessageDef> sorted = messages.OrderBy(m => m.FilePath, StringComparer.Ordinal).ToList();
            foreach (MessageDef message in sorted)
            {
                foreach (string part in message.Namespace.Split('.'))
                {
                    if (CppKeywords.Contains(part) || CsKeywords.Contains(part))
                        diag.Error(message.Pos, Codes.BadNamespace, "namespace '" + message.Namespace + "' uses the keyword '" + part + "'");
                }
            }
            if (sorted.Count == 0)
                return;
            MessageDef first = sorted[0];
            foreach (MessageDef message in sorted.Skip(1))
            {
                if (message.Namespace != first.Namespace)
                    diag.Error(message.Pos, Codes.NamespaceMismatch, "namespace '" + message.Namespace + "' differs from '" + first.Namespace + "' (" + first.FileName + "). Generate each namespace in its own run and output folder");
            }
        }

        private static void CheckFields(MessageDef message, Dictionary<string, MessageDef> byName, Diag diag)
        {
            HashSet<string> keys = new HashSet<string>();
            HashSet<string> csNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (FieldDef field in message.Fields)
            {
                SourcePos pos = field.Pos;
                if (!keys.Add(field.Key))
                    diag.Error(pos, Codes.DupKey, "duplicate field key '" + field.Key + "'");
                CheckName(message, field, csNames, diag);
                ResolveType(field.Type, field, byName, diag, false);
                if (field.Required && field.Type.IsFixed)
                    diag.Error(pos, Codes.RequiredFixed, "'" + field.Key + "' is a fixed field and is always present. required is not allowed (validate the value in receiving code)");
                CheckDefault(field, diag);
            }
        }

        private static void CheckName(MessageDef message, FieldDef field, HashSet<string> csNames, Diag diag)
        {
            if (field.Key.StartsWith("_"))
                diag.Error(field.Pos, Codes.Reserved, "keys starting with '_' are reserved");
            if (field.Key.StartsWith("get_") || field.Key.StartsWith("set_") || field.Key.StartsWith("mutable_"))
                diag.Error(field.Pos, Codes.Reserved, "keys starting with 'get_', 'set_' or 'mutable_' clash with generated accessors");
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

        private static void ResolveType(TypeRef type, FieldDef field, Dictionary<string, MessageDef> byName, Diag diag, bool isElement)
        {
            if (type.Kind == TypeKind.List)
            {
                if (isElement)
                {
                    diag.Error(field.Pos, Codes.BadElement, "list of lists is not supported. Wrap the element in a message");
                    return;
                }
                TypeRef element = type.Element;
                if (element.Kind == TypeKind.Bool || element.Kind == TypeKind.FixedString)
                {
                    diag.Error(field.Pos, Codes.BadElement, "list<" + element + "> is not supported. Allowed elements: int, int64, uint, uint64, double, string, bytes, message");
                    return;
                }
                ResolveType(element, field, byName, diag, true);
                return;
            }
            if (type.Kind != TypeKind.Message)
                return;
            MessageDef target;
            if (!byName.TryGetValue(type.MessageName, out target))
            {
                diag.Error(field.Pos, Codes.UnknownRef, "unknown message '" + type.MessageName + "'. Expected " + type.MessageName + ".yaml in an -I folder");
                return;
            }
            type.Resolved = target;
        }

        private static void CheckDefault(FieldDef field, Diag diag)
        {
            if (field.DefaultText == null)
                return;
            string text = field.DefaultText;
            bool ok;
            switch (field.Type.Kind)
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
                case TypeKind.String:
                case TypeKind.FixedString:
                    // Non-ASCII literal bytes depend on the compiler charset. ASCII only.
                    if (!Naming.IsAscii(text))
                    {
                        diag.Error(field.Pos, Codes.BadDefault, "string defaults must be ASCII");
                        return;
                    }
                    ok = field.Type.Kind == TypeKind.String || text.Length <= field.Type.FixedLength;
                    if (!ok)
                    {
                        diag.Error(field.Pos, Codes.BadDefault, "default is longer than string(" + field.Type.FixedLength + ")");
                        return;
                    }
                    break;
                default:
                    diag.Error(field.Pos, Codes.BadDefault, "list, bytes and message fields cannot have a default (they start empty)");
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
                diag.Error(message.Pos, Codes.Cycle, "reference cycle: " + string.Join(" -> ", path) + ". Recursive messages are not supported");
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
            order.Add(message);
            return ok;
        }

        private static IEnumerable<MessageDef> DirectDependencies(MessageDef message)
        {
            HashSet<MessageDef> seen = new HashSet<MessageDef>();
            foreach (FieldDef field in message.Fields)
            {
                TypeRef type = field.Type.Kind == TypeKind.List ? field.Type.Element : field.Type;
                if (type.Resolved != null && seen.Add(type.Resolved))
                    yield return type.Resolved;
            }
        }

        // Serialization order: added, then declaration. A later version only appends fixed fields, so the fixed block
        // of version k is a prefix of every newer one and an offset never changes. Removed fixed fields keep their bytes.
        // Layout changes are caught by the schema fingerprint.
        private static void Layout(MessageDef message)
        {
            message.Physical = message.Fields.OrderBy(f => f.Added).ThenBy(f => f.Index).ToList();
            int offset = 0;
            foreach (FieldDef field in message.Physical.Where(f => f.Type.IsFixed))
            {
                field.FixedOffset = offset;
                offset += field.Type.FixedSize;
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
        }
    }
}
