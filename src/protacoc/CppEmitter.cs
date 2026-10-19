// CppEmitter.cs. One message -> <Name>.g.h (C++14 compatible, header-only).
// The latest version is <ns>::<Name>. Older supported versions are <ns>::v<N>::<Name>.
// Public: user API only (constructor, get_/set_/get_mutable_ field accessors, GetExt/GetMutableExt when allowed, envelope,
// Serialize, ToString). Field accessors are snake_case get_<key>; everything else is PascalCase.
// Buffer, visitors and constants are private; the runtime gets in as a friend.
using System.Collections.Generic;
using System.Linq;

namespace Protacoc
{
    internal static class CppEmitter
    {
        // Frame header bytes before the fingerprints: magic 2, frame version 1, name length 1, name, version 2,
        // min version 2, id 4, corr 4, ts 8, BodyLen 4. Then 4 bytes per fingerprint.
        public const int HeaderBytesWithoutName = 28;

        public static string Emit(MessageDef message, string[] ns)
        {
            CodeWriter w = new CodeWriter();
            w.Line("// " + message.Name + ".g.h. protacoc " + Naming.Version + ", source " + message.FileName + " (sha1 " + message.Sha1 + "). Do not edit.");
            w.Line("#pragma once");
            w.Line();
            w.Line("#include \"Protaco.hpp\"");
            foreach (MessageDef dep in message.Dependencies.OrderBy(d => d.Name, System.StringComparer.Ordinal))
                w.Line("#include \"" + dep.Name + ".g.h\"");
            w.Line();
            foreach (string part in ns)
                w.Line("namespace " + part + "\n{");
            w.Line();
            Class(w, message, message.Latest);
            foreach (VersionView view in message.Views.Where(v => v != message.Latest))
            {
                w.Line();
                w.Line("namespace v" + view.Version + "\n{");
                w.Line();
                Class(w, message, view);
                w.Line();
                w.Line("}  // namespace v" + view.Version);
            }
            w.Line();
            for (int i = ns.Length - 1; i >= 0; --i)
                w.Line("}  // namespace " + ns[i]);
            return w.ToString();
        }

        private static void Class(CodeWriter w, MessageDef message, VersionView view)
        {
            Doc(w, Naming.MessageDoc(message, view));
            w.Line("class " + message.Name);
            w.Line("{");
            w.Line("public:");
            w.Indent();
            Constructor(w, message, view);
            StaticInfo(w, message, view);
            foreach (FieldDef field in view.Alive)
                Accessors(w, field);
            Common(w, message, view);
            w.Outdent();
            w.Line();
            w.Line("private:");
            w.Indent();
            Private(w, message, view);
            w.Outdent();
            w.Line("};");
        }

        private static void Doc(CodeWriter w, List<string> lines)
        {
            foreach (string line in lines)
                w.Line(line.Length == 0 ? "///" : "/// " + line);
        }

        public static string CppType(TypeRef type)
        {
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    return "bool";
                case TypeKind.Int:
                    return "int32_t";
                case TypeKind.Int64:
                    return "int64_t";
                case TypeKind.Double:
                    return "double";
                case TypeKind.String:
                case TypeKind.FixedString:
                    return "std::string";
                case TypeKind.Bytes:
                    return "std::vector<uint8_t>";
                case TypeKind.List:
                    return "std::vector<" + CppType(type.Element) + ">";
                default:
                    return type.MessageName;
            }
        }

        private static string FixedAt(FieldDef field)
        {
            return "buf_.data() + kFixedOffset + " + field.FixedOffset;
        }

        private static void Constructor(CodeWriter w, MessageDef message, VersionView view)
        {
            w.Line("/// Initialized to defaults. Fixed fields live in the frame buffer.");
            w.Line(message.Name + "()");
            w.Line("{");
            w.Indent();
            w.Line("protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);");
            w.Line("protaco::detail::Access::BindExt(ext_, &" + message.Name + "::IsSchemaKey);");
            // Removed fixed fields keep their bytes; their default is what older readers get.
            foreach (FieldDef field in view.FixedSlots)
            {
                string literal = FixedDefault(field);
                if (literal == null)
                    continue;
                switch (field.Type.Kind)
                {
                    case TypeKind.Bool:
                        w.Line("protaco::detail::StoreBool(" + FixedAt(field) + ", true);");
                        break;
                    case TypeKind.FixedString:
                        w.Line("protaco::detail::StoreChars(" + FixedAt(field) + ", " + field.Type.FixedLength + ", " + literal + ");");
                        break;
                    default:
                        w.Line("protaco::detail::Store<" + CppType(field.Type) + ">(" + FixedAt(field) + ", " + literal + ");");
                        break;
                }
            }
            w.Outdent();
            w.Line("}");
            w.Line();
        }

        // Writes only defaults other than 0, false and "" (the buffer starts zeroed).
        private static string FixedDefault(FieldDef field)
        {
            string text = field.DefaultText;
            if (text == null)
                return null;
            switch (field.Type.Kind)
            {
                case TypeKind.Bool:
                    return text == "true" ? "true" : null;
                case TypeKind.FixedString:
                    return text.Length == 0 ? null : Naming.CppString(text);
                case TypeKind.Double:
                    return double.Parse(text, System.Globalization.CultureInfo.InvariantCulture) == 0 ? null : text;
                default:
                    return text == "0" || text == "-0" ? null : text;
            }
        }

        private static void StaticInfo(CodeWriter w, MessageDef message, VersionView view)
        {
            w.Line("/// Message name. Stored in the frame header and checked by ToObject.");
            Method(w, "static const char* MsgName()", "return \"" + message.Name + "\";");
            w.Line("/// Version of this type. ToObject reads frames of this version or newer as this version.");
            Method(w, "static uint16_t MsgVersion()", "return " + view.Version + ";");
            w.Line("/// Oldest supported version (min_version). Frames carry one schema fingerprint per version from here up.");
            Method(w, "static uint16_t MsgMinVersion()", "return " + message.MinVersion + ";");
            w.Line("/// Schema fingerprint of this version. A frame whose fingerprint for this version differs is rejected by ToObject.");
            Method(w, "static uint32_t MsgSchemaHash()", "return " + SchemaFingerprint.Hex(view.SchemaHash) + "u;");
        }

        private static void Accessors(CodeWriter w, FieldDef field)
        {
            string key = field.Key;
            string type = CppType(field.Type);
            List<string> doc = Naming.FieldDoc(field);
            Doc(w, doc);
            if (field.Type.IsFixed)
            {
                string at = FixedAt(field);
                switch (field.Type.Kind)
                {
                    case TypeKind.Bool:
                        Method(w, "bool get_" + key + "() const", "return protaco::detail::LoadBool(" + at + ");");
                        Method(w, "void set_" + key + "(bool value)", "protaco::detail::StoreBool(" + at + ", value);");
                        break;
                    case TypeKind.FixedString:
                        Method(w, "std::string get_" + key + "() const", "return protaco::detail::LoadChars(" + at + ", " + field.Type.FixedLength + ");");
                        w.Line("/// Returns false and keeps the old value if longer than " + field.Type.FixedLength + " bytes.");
                        Method(w, "bool set_" + key + "(const std::string& value)", "return protaco::detail::StoreChars(" + at + ", " + field.Type.FixedLength + ", value);");
                        break;
                    default:
                        Method(w, type + " get_" + key + "() const", "return protaco::detail::Load<" + type + ">(" + at + ");");
                        Method(w, "void set_" + key + "(" + type + " value)", "protaco::detail::Store<" + type + ">(" + at + ", value);");
                        break;
                }
                return;
            }
            Method(w, "const " + type + "& get_" + key + "() const", "return " + key + "_;");
            if (field.Type.Kind != TypeKind.String)
                Method(w, type + "& get_mutable_" + key + "()", "cache_.exposed = true;", "return " + key + "_;");
            Method(w, "void set_" + key + "(const " + type + "& value)", key + "_ = value;", "cache_.valid = false;");
        }

        private static void Method(CodeWriter w, string signature, params string[] body)
        {
            w.Line(signature);
            w.Line("{");
            foreach (string line in body)
                w.Line("    " + line);
            w.Line("}");
            w.Line();
        }

        private static void Common(CodeWriter w, MessageDef message, VersionView view)
        {
            if (message.AllowExtension)
            {
                w.Line("/// Extra key-value data not in the yaml. Unknown keys of received messages are kept here.");
                Method(w, "const protaco::CExt& GetExt() const", "return ext_;");
                Method(w, "protaco::CExt& GetMutableExt()", "return ext_;");
            }
            w.Line("/// Writes the envelope (id, corr, ts) straight into the frame header.");
            Method(w, "void SetEnvelope(const protaco::Envelope& env)", "protaco::detail::StoreEnvelope(buf_, env);");
            Method(w, "protaco::Envelope GetEnvelope() const", "return protaco::detail::LoadEnvelope(buf_.data());");
            w.Line("/// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.");
            if (view.AliveVar.Count == 0)
                w.Line("/// Fixed fields only. " + (message.AllowExtension ? "With an empty ext, serializing" : "Serializing") + " costs nothing.");
            w.Line("/// Unchanged variable fields and ext reuse the last bytes. After a get_mutable_ call the variable block is rebuilt every time.");
            w.Line("/// The returned reference is valid until this object is modified.");
            Method(w, "const std::vector<uint8_t>& Serialize() const", "return protaco::detail::Access::Serialize(*this);");
            w.Line("/// Text for logs: field names and values" + (message.AllowExtension ? ", ext entries under ext{...}" : "") + ". protaco::TextStyle::Pretty() for one field per line.");
            w.Line("std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const");
            w.Line("{");
            w.Line("    return protaco::detail::Access::ToText(*this, style);");
            w.Line("}");
        }

        private static void Private(CodeWriter w, MessageDef message, VersionView view)
        {
            int hashCount = view.Version - message.MinVersion + 1;
            w.Line("friend struct protaco::detail::Access;");
            w.Line();
            w.Line("enum : uint32_t");
            w.Line("{");
            w.Line("    kFixedOffset = " + (HeaderBytesWithoutName + message.Name.Length + 4 * hashCount + 4) + ",   // frame header (28 + name " + message.Name.Length + " + fingerprints 4 x " + hashCount + ") + FixedLen (4)");
            w.Line("    kFixedSize = " + view.FixedSize + ",");
            w.Line("    kAllowExtension = " + (message.AllowExtension ? "1" : "0") + ",");
            w.Line("};");
            w.Line();
            w.Line("// Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.");
            w.Line("static const uint32_t* SchemaHashes()");
            w.Line("{");
            w.Line("    static const uint32_t kHashes[] = { " + string.Join(", ", message.Views.Where(v => v.Version <= view.Version).Select(v => SchemaFingerprint.Hex(v.SchemaHash) + "u")) + " };");
            w.Line("    return kHashes;");
            w.Line("}");
            w.Line();
            List<string> keys = message.Fields.Select(f => f.Key).ToList();
            w.Line("// Keys not allowed in ext: schema keys of every version");
            if (keys.Count == 0)
            {
                Method(w, "static bool IsSchemaKey(const char*)", "return false;");
            }
            else
            {
                w.Line("static bool IsSchemaKey(const char* key)");
                w.Line("{");
                w.Line("    static const char* const kKeys[] = { " + string.Join(", ", keys.Select(k => "\"" + k + "\"")) + " };");
                w.Line("    return protaco::detail::InList(key, kKeys, " + keys.Count + ");");
                w.Line("}");
                w.Line();
            }
            w.Line("// The single list of variable fields. Shared by size, write and read.");
            w.Line("template <class TSelf, class TVisitor>");
            if (view.AliveVar.Count == 0)
            {
                w.Line("static void VisitVar(TSelf&, TVisitor&)");
                w.Line("{");
                w.Line("}");
            }
            else
            {
                w.Line("static void VisitVar(TSelf& self, TVisitor& visitor)");
                w.Line("{");
                foreach (FieldDef field in view.AliveVar)
                    w.Line("    visitor.Field(\"" + field.Key + "\", self." + field.Key + "_, " + (field.Required ? "true" : "false") + ");");
                w.Line("}");
            }
            w.Line();
            w.Line("// All fields in yaml order, for ToString.");
            w.Line("template <class TWriter>");
            if (view.Alive.Count == 0)
            {
                w.Line("static void VisitText(const " + message.Name + "&, TWriter&)");
                w.Line("{");
                w.Line("}");
            }
            else
            {
                w.Line("static void VisitText(const " + message.Name + "& self, TWriter& writer)");
                w.Line("{");
                foreach (FieldDef field in view.Alive)
                {
                    string value = field.Type.IsFixed ? "self.get_" + field.Key + "()" : "self." + field.Key + "_";
                    w.Line("    writer.Field(\"" + field.Key + "\", " + value + ");");
                }
                w.Line("}");
            }
            w.Line();
            w.Line("mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)");
            foreach (FieldDef field in view.AliveVar.OrderBy(f => f.Index))
            {
                string init = field.Type.Kind == TypeKind.String && !string.IsNullOrEmpty(field.DefaultText) ? " = " + Naming.CppString(field.DefaultText) : "";
                w.Line(CppType(field.Type) + " " + field.Key + "_" + init + ";");
            }
            w.Line("protaco::CExt ext_;" + (message.AllowExtension ? "" : "   // stays empty: allow_extension false"));
            w.Line("mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block");
        }
    }
}
