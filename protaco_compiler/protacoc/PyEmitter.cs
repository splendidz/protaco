// PyEmitter.cs. One message -> <Name>_g.py, one enum -> <Name>_enum.py (Python 3.8 and later, standard library only).
// Files go to <py_out>/<namespace as folders>/ and import each other relatively; the runtime protaco.py goes to <py_out>.
// The latest version is <Name>. Older supported versions are v<N>.<Name> (class v<N> in the same module).
// Field attributes are the yaml keys; a Python keyword gets a trailing '_' (from -> from_). Same bytes as C++.
using System.Collections.Generic;
using System.Linq;

namespace Protacoc
{
    internal static class PyEmitter
    {
        public static readonly HashSet<string> Keywords = new HashSet<string>
        {
            "False", "None", "True", "and", "as", "assert", "async", "await", "break", "class", "continue", "def", "del",
            "elif", "else", "except", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
            "not", "or", "pass", "raise", "return", "try", "while", "with", "yield",
        };

        // Attribute or enum member name of a yaml key.
        public static string Name(string key)
        {
            return Keywords.Contains(key) ? key + "_" : key;
        }

        public static string Emit(MessageDef message)
        {
            CodeWriter w = new CodeWriter();
            w.Line("# " + message.Name + "_g.py. protacoc " + Naming.Version + ", source " + message.FileName + " (sha1 " + message.Sha1 + "). Do not edit.");
            w.Line("import protaco");
            foreach (EnumDef dep in message.EnumDependencies.OrderBy(d => d.Name, System.StringComparer.Ordinal))
                w.Line("from ." + dep.Name + "_enum import " + dep.Name + ", " + dep.Name + "Names");
            foreach (MessageDef dep in message.Dependencies.OrderBy(d => d.Name, System.StringComparer.Ordinal))
                w.Line("from ." + dep.Name + "_g import " + dep.Name);
            w.Line();
            w.Line();
            Class(w, message, message.Latest);
            w.Line();
            w.Line();
            Bind(w, message, message.Latest, message.Name);
            foreach (VersionView view in message.Views.Where(v => v != message.Latest))
            {
                w.Line();
                w.Line();
                w.Line("class v" + view.Version + ":");
                w.Indent();
                w.Line("\"\"\"Version " + view.Version + " of " + message.Name + ", for frames from peers on version " + view.Version + ".\"\"\"");
                w.Line();
                Class(w, message, view);
                w.Outdent();
                w.Line();
                w.Line();
                Bind(w, message, view, "v" + view.Version + "." + message.Name);
            }
            return w.ToString();
        }

        // enum.IntEnum <Name> (values of the latest version) and <Name>Names (Name with removed values, TryParse).
        public static string EmitEnum(EnumDef enumDef)
        {
            CodeWriter w = new CodeWriter();
            w.Line("# " + enumDef.Name + "_enum.py. protacoc " + Naming.Version + ", source " + enumDef.FileName + " (sha1 " + enumDef.Sha1 + "). Do not edit.");
            w.Line("import enum");
            w.Line();
            w.Line("import protaco");
            w.Line();
            w.Line();
            w.Line("class " + enumDef.Name + "(enum.IntEnum):");
            w.Indent();
            Doc(w, Naming.EnumDoc(enumDef));
            w.Line();
            foreach (EnumValueDef value in enumDef.Alive)
            {
                foreach (string line in Naming.EnumValueDoc(value))
                    w.Line("#: " + line);
                w.Line(Name(value.Key) + " = " + value.Number);
            }
            w.Outdent();
            w.Line();
            w.Line();
            w.Line("class " + enumDef.Name + "Names:");
            w.Indent();
            w.Line("\"\"\"Names of " + enumDef.Name + " values.\"\"\"");
            w.Line();
            w.Line("_NAMES = {" + string.Join(", ", enumDef.Values.Select(v => v.Number + ": " + Naming.CppString(v.Key))) + "}   # removed values included");
            w.Line("_VALUES = {" + string.Join(", ", enumDef.Alive.Select(v => Naming.CppString(v.Key) + ": " + enumDef.Name + "." + Name(v.Key))) + "}");
            w.Line();
            w.Line("@staticmethod");
            w.Line("def Name(value):");
            w.Line("    \"\"\"Name of a value, removed values included. None for a number never declared.\"\"\"");
            w.Line("    return " + enumDef.Name + "Names._NAMES.get(int(value))");
            w.Line();
            w.Line("@staticmethod");
            w.Line("def TryParse(name):");
            w.Line("    \"\"\"Value from its yaml name (values of version " + enumDef.Version + "). Result value: the " + enumDef.Name + ".\"\"\"");
            w.Line("    value = " + enumDef.Name + "Names._VALUES.get(name)");
            w.Line("    if value is None:");
            w.Line("        return protaco.Result(False, None, \"unknown " + enumDef.Name + " name: \" + str(name))");
            w.Line("    return protaco.Result(True, value)");
            w.Outdent();
            return w.ToString();
        }

        // class <Name> with int class attributes. A Python keyword key gets a trailing '_'.
        public static string EmitConst(ConstDef constDef)
        {
            CodeWriter w = new CodeWriter();
            w.Line("# " + constDef.Name + "_const.py. protacoc " + Naming.Version + ", source " + constDef.FileName + " (sha1 " + constDef.Sha1 + "). Do not edit.");
            w.Line();
            w.Line();
            w.Line("class " + constDef.Name + ":");
            w.Indent();
            Doc(w, Naming.ConstDoc(constDef));
            w.Line();
            foreach (ConstValueDef value in constDef.Values)
            {
                foreach (string line in Naming.ConstValueDoc(value))
                    w.Line("#: " + line);
                w.Line(Name(value.Key) + " = " + value.Number);
            }
            w.Outdent();
            return w.ToString();
        }

        private static void Doc(CodeWriter w, List<string> lines)
        {
            List<string> escaped = lines.Select(l => l.Replace("\\", "\\\\").Replace("\"\"\"", "\\\"\\\"\\\"")).ToList();
            w.Line("\"\"\"" + (escaped.Count == 0 ? "" : escaped[0]));
            foreach (string line in escaped.Skip(1))
                w.Line(line);
            w.Line("\"\"\"");
        }

        private static void Class(CodeWriter w, MessageDef message, VersionView view)
        {
            w.Line("class " + message.Name + "(protaco.Message):");
            w.Indent();
            Doc(w, Naming.MessageDoc(message, view));
            w.Line();
            List<string> attrs = view.Alive.Select(f => "\"" + Name(f.Key) + "\"").ToList();
            w.Line("__slots__ = (" + string.Join(", ", attrs) + (attrs.Count == 1 ? "," : "") + ")");
            w.Line("MsgName = \"" + message.Name + "\"");
            w.Line("MsgVersion = " + view.Version);
            w.Line("MsgMinVersion = " + message.MinVersion);
            w.Line("MsgSchemaHash = " + SchemaFingerprint.Hex(view.SchemaHash));
            w.Line();
            w.Line("def __init__(self, *, _stamp=None, **fields):");
            w.Indent();
            w.Line("\"\"\"Defaults; fields by key, for example " + message.Name + "(" + (view.Alive.Count == 0 ? "" : Name(view.Alive[0].Key) + "=...") + ").");
            if (message.AutoEnvelope)
            {
                w.Line("The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs).\"\"\"");
                w.Line("protaco.Message.__init__(self, _stamp)");
            }
            else
            {
                w.Line("The envelope stays 0 (auto_envelope false): set it with SetId, SetCorr and SetTimestamp.\"\"\"");
                w.Line("protaco.Message.__init__(self, protaco.NO_STAMP)");
            }
            foreach (FieldDef field in view.Alive)
            {
                foreach (string line in Naming.FieldDoc(field))
                    w.Line("#: " + line);
                w.Line("self." + Name(field.Key) + " = " + Default(field));
            }
            w.Line("if fields:");
            w.Line("    self._Assign(fields)");
            w.Outdent();
            w.Outdent();
        }

        private static void Bind(CodeWriter w, MessageDef message, VersionView view, string target)
        {
            w.Line(target + "._Bind(");
            w.Indent();
            List<string> hashes = message.Views.Where(v => v.Version <= view.Version).Select(v => SchemaFingerprint.Hex(v.SchemaHash)).ToList();
            w.Line("hashes=(" + string.Join(", ", hashes) + (hashes.Count == 1 ? "," : "") + "),");
            w.Line("allow_extension=" + (message.AllowExtension ? "True" : "False") + ",");
            List<string> keys = message.Fields.Select(f => "\"" + f.Key + "\"").ToList();
            w.Line("schema_keys=(" + string.Join(", ", keys) + (keys.Count == 1 ? "," : "") + "),");
            Tuple(w, "fixed", view.FixedSlots.Select(field => field.AliveAt(view.Version)
                ? "(\"" + field.Key + "\", \"" + Name(field.Key) + "\", " + Item(field.Type, true) + ", None)"
                : "(\"" + field.Key + "\", None, " + Item(field.Type, true) + ", " + RemovedDefault(field) + ")"));
            Tuple(w, "var", view.AliveVar.Select(field => "(\"" + field.Key + "\", \"" + Name(field.Key) + "\", " + Item(field.Type) + ", "
                + (field.Required ? "True" : "False") + ", " + (field.Optional ? "True" : "False") + ")"));
            List<string> text = view.Alive.Select(f => "\"" + f.Key + "\"").ToList();
            w.Line("text=(" + string.Join(", ", text) + (text.Count == 1 ? "," : "") + "),");
            w.Outdent();
            w.Line(")");
        }

        // name=(\n item,\n ...\n), or name=() when empty.
        private static void Tuple(CodeWriter w, string name, IEnumerable<string> items)
        {
            List<string> lines = items.ToList();
            if (lines.Count == 0)
            {
                w.Line(name + "=(),");
                return;
            }
            w.Line(name + "=(");
            foreach (string line in lines)
                w.Line("    " + line + ",");
            w.Line("),");
        }

        // Runtime descriptor (protaco.Items) of a type. inline: a fixed block slot, where a fixed: true message is inline.
        private static string Item(TypeRef type, bool inline = false)
        {
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    return "protaco.Items.Bool";
                case TypeKind.Int:
                    return "protaco.Items.Int";
                case TypeKind.Int64:
                    return "protaco.Items.Int64";
                case TypeKind.UInt:
                    return "protaco.Items.UInt";
                case TypeKind.UInt64:
                    return "protaco.Items.UInt64";
                case TypeKind.Double:
                    return "protaco.Items.Double";
                case TypeKind.String:
                    return "protaco.Items.String";
                case TypeKind.FixedString:
                    return "protaco.Items.Chars(" + type.FixedLength + ")";
                case TypeKind.Bytes:
                    return "protaco.Items.Bytes";
                case TypeKind.Enum:
                    return "protaco.Items.Enum(" + type.TypeName + ", " + type.TypeName + "Names.Name)";
                case TypeKind.List:
                    return "protaco.Items.List(" + Item(type.Element) + ")";
                case TypeKind.Map:
                    return "protaco.Items.Map(" + Item(type.Key) + ", " + Item(type.Element) + ")";
                case TypeKind.Array:
                    return "protaco.Items.Array(" + Item(type.Element, true) + ", " + type.ArrayLength + ")";
                default:
                    return (inline && type.Resolved.Fixed ? "protaco.Items.Fixed(" : "protaco.Items.Message(") + type.TypeName + ")";
            }
        }

        // An array starts as a list of N copies of the element default.
        private static string Default(FieldDef field)
        {
            if (field.Type.Kind == TypeKind.Array && field.Type.Element.Kind == TypeKind.Message)
                return "[" + DefaultOf(field.Type.Element, null, false) + " for _ in range(" + field.Type.ArrayLength + ")]";
            if (field.Type.Kind == TypeKind.Array)
                return "[" + DefaultOf(field.Type.Element, field.DefaultText, false) + "] * " + field.Type.ArrayLength;
            return DefaultOf(field.Type, field.DefaultText, field.Optional);
        }

        private static string DefaultOf(TypeRef type, string text, bool optional)
        {
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    return text == "true" ? "True" : "False";
                case TypeKind.Double:
                    return text == null ? "0.0" : FloatLiteral(text);
                case TypeKind.String:
                case TypeKind.FixedString:
                    return Naming.CppString(text ?? "");
                case TypeKind.Bytes:
                    return "b\"\"";
                case TypeKind.List:
                    return "[]";
                case TypeKind.Map:
                    return "{}";
                case TypeKind.Message:
                    return optional ? "None" : type.TypeName + "(_stamp=protaco.NO_STAMP)";
                case TypeKind.Enum:
                    return type.TypeName + "." + Name(text ?? type.ResolvedEnum.First.Key);
                default:
                    return text ?? "0";
            }
        }

        // Value packed into the slot of a removed fixed field: its default, which older readers get.
        private static string RemovedDefault(FieldDef field)
        {
            TypeRef type = field.Type.Kind == TypeKind.Array ? field.Type.Element : field.Type;
            if (type.Kind != TypeKind.Enum)
                return Default(field);
            string text = field.DefaultText;
            EnumValueDef value = text == null ? type.ResolvedEnum.First : type.ResolvedEnum.Values.First(v => v.Key == text);
            return field.Type.Kind == TypeKind.Array ? "[" + value.Number + "] * " + field.Type.ArrayLength : value.Number.ToString();
        }

        private static string FloatLiteral(string text)
        {
            return text.IndexOfAny(new[] { '.', 'e', 'E' }) < 0 ? text + ".0" : text;
        }
    }
}
