// CppEmitter.cs. One message -> <Name>.g.h, one enum -> <Name>.enum.h (C++14 compatible, header-only).
// The latest version is <ns>::<Name>. Older supported versions are <ns>::v<N>::<Name>.
// Public: user API only (constructor, get_/set_/mutable_ field accessors, GetExt/GetMutableExt when allowed, envelope,
// Serialize, ToString). Field accessors are snake_case get_<key>; everything else is PascalCase.
// Buffer, visitors and constants are private; the runtime gets in as a friend.
using System;
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
            foreach (EnumDef dep in message.EnumDependencies.OrderBy(d => d.Name, System.StringComparer.Ordinal))
                w.Line("#include \"" + dep.Name + ".enum.h\"");
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

        // enum class <Name> : int32_t, EnumName (removed values included, for old frames) and TryParseEnum (latest values).
        public static string EmitEnum(EnumDef enumDef, string[] ns)
        {
            CodeWriter w = new CodeWriter();
            w.Line("// " + enumDef.Name + ".enum.h. protacoc " + Naming.Version + ", source " + enumDef.FileName + " (sha1 " + enumDef.Sha1 + "). Do not edit.");
            w.Line("#pragma once");
            w.Line();
            w.Line("#include <cstdint>");
            w.Line("#include <string>");
            w.Line();
            foreach (string part in ns)
                w.Line("namespace " + part + "\n{");
            w.Line();
            Doc(w, Naming.EnumDoc(enumDef));
            w.Line("enum class " + enumDef.Name + " : int32_t");
            w.Line("{");
            w.Indent();
            foreach (EnumValueDef value in enumDef.Alive)
            {
                Doc(w, Naming.EnumValueDoc(value));
                w.Line(value.Key + " = " + value.Number + ",");
            }
            w.Outdent();
            w.Line("};");
            w.Line();
            w.Line("/// Name of a value, removed values included. nullptr for a number never declared.");
            w.Line("inline const char* EnumName(" + enumDef.Name + " value)");
            w.Line("{");
            w.Line("    switch (static_cast<int32_t>(value))");
            w.Line("    {");
            foreach (EnumValueDef value in enumDef.Values)
            {
                w.Line("    case " + value.Number + ":");
                w.Line("        return \"" + value.Key + "\";");
            }
            w.Line("    default:");
            w.Line("        return nullptr;");
            w.Line("    }");
            w.Line("}");
            w.Line();
            w.Line("/// Value from its name (values of version " + enumDef.Version + "). False, out unchanged and error set for another name.");
            w.Line("inline bool TryParseEnum(const std::string& name, " + enumDef.Name + "* out, std::string* error = nullptr)");
            w.Line("{");
            w.Indent();
            foreach (EnumValueDef value in enumDef.Alive)
            {
                w.Line("if (name == \"" + value.Key + "\")");
                w.Line("{");
                w.Line("    *out = " + enumDef.Name + "::" + value.Key + ";");
                w.Line("    return true;");
                w.Line("}");
            }
            w.Line("if (error != nullptr)");
            w.Line("    *error = \"unknown " + enumDef.Name + " name: \" + name;");
            w.Line("return false;");
            w.Outdent();
            w.Line("}");
            w.Line();
            for (int i = ns.Length - 1; i >= 0; --i)
                w.Line("}  // namespace " + ns[i]);
            return w.ToString();
        }

        // namespace <Name> { constexpr int32_t <Key> = <value>; }. Namespace-scope constexpr has internal linkage, so the
        // values work in C++14 without out-of-line definitions.
        public static string EmitConst(ConstDef constDef, string[] ns)
        {
            CodeWriter w = new CodeWriter();
            w.Line("// " + constDef.Name + ".const.h. protacoc " + Naming.Version + ", source " + constDef.FileName + " (sha1 " + constDef.Sha1 + "). Do not edit.");
            w.Line("#pragma once");
            w.Line();
            w.Line("#include <cstdint>");
            w.Line();
            foreach (string part in ns)
                w.Line("namespace " + part + "\n{");
            w.Line();
            Doc(w, Naming.ConstDoc(constDef));
            w.Line("namespace " + constDef.Name);
            w.Line("{");
            foreach (ConstValueDef value in constDef.Values)
            {
                Doc(w, Naming.ConstValueDoc(value));
                w.Line("constexpr int32_t " + value.Key + " = " + value.Number + ";");
            }
            w.Line("}  // namespace " + constDef.Name);
            w.Line();
            for (int i = ns.Length - 1; i >= 0; --i)
                w.Line("}  // namespace " + ns[i]);
            return w.ToString();
        }

        private static void Class(CodeWriter w, MessageDef message, VersionView view)
        {
            Doc(w, Naming.MessageDoc(message, view));
            w.Line("class " + message.Name + " : public protaco::CMessage<" + message.Name + ">");
            w.Line("{");
            w.Line("public:");
            w.Indent();
            Constructor(w, message, view);
            StaticInfo(w, message, view);
            foreach (FieldDef field in view.Alive)
                Accessors(w, field);
            Common(w, message, view);
            if (view.AliveVar.Count == 0)
                View(w, message, view);
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
                case TypeKind.UInt:
                    return "uint32_t";
                case TypeKind.UInt64:
                    return "uint64_t";
                case TypeKind.Double:
                    return "double";
                case TypeKind.String:
                case TypeKind.FixedString:
                    return "std::string";
                case TypeKind.Bytes:
                    return "std::vector<uint8_t>";
                case TypeKind.List:
                    return "std::vector<" + CppType(type.Element) + ">";
                case TypeKind.Map:
                    return "std::unordered_map<" + CppType(type.Key) + ", " + CppType(type.Element) + ">";
                case TypeKind.Array:
                    return "std::array<" + CppType(type.Element) + ", " + type.ArrayLength + ">";
                default:
                    return type.TypeName;
            }
        }

        private static string FixedAt(FieldDef field)
        {
            return "buf_.data() + kFixedOffset + " + field.FixedOffset;
        }

        private static void Constructor(CodeWriter w, MessageDef message, VersionView view)
        {
            w.Line(message.AutoEnvelope
                ? "/// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs)."
                : "/// Initialized to defaults. The envelope stays 0 (auto_envelope false): set it with SetId, SetCorr and SetTimestamp.");
            w.Line(message.Name + "()");
            w.Line("    : " + message.Name + "(protaco::detail::NoStampTag())");
            w.Line("{");
            if (message.AutoEnvelope)
                w.Line("    protaco::detail::StampEnvelope(buf_);");
            w.Line("}");
            w.Line();
            w.Line("/// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.");
            w.Line("explicit " + message.Name + "(protaco::detail::NoStampTag)");
            w.Line("{");
            w.Indent();
            w.Line("protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);");
            w.Line("protaco::detail::Access::BindExt(ext_, &" + message.Name + "::IsSchemaKey);");
            // Removed fixed fields keep their bytes; their default is what older readers get. An array default fills every element.
            foreach (FieldDef field in view.FixedSlots)
            {
                bool array = field.Type.Kind == TypeKind.Array;
                TypeRef type = array ? field.Type.Element : field.Type;
                string literal = FixedDefault(type, field.DefaultText);
                if (literal == null)
                    continue;
                if (!array)
                {
                    w.Line(StoreDefault(type, FixedAt(field), literal, field.Key));
                    continue;
                }
                w.Line("for (size_t i = 0; i < " + field.Type.ArrayLength + "; ++i)");
                w.Line("    " + StoreDefault(type, FixedAt(field) + " + " + type.FixedSize + " * i", literal, field.Key));
            }
            w.Outdent();
            w.Line("}");
            w.Line();
        }

        private static string StoreDefault(TypeRef type, string at, string literal, string key)
        {
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    return "protaco::detail::StoreBool(" + at + ", true);";
                case TypeKind.FixedString:
                    return "protaco::detail::StoreChars(" + at + ", " + type.FixedLength + ", " + literal + ", \"" + key + "\", nullptr);";
                case TypeKind.Enum:
                    return "protaco::detail::Store<int32_t>(" + at + ", static_cast<int32_t>(" + literal + "));";
                default:
                    return "protaco::detail::Store<" + CppType(type) + ">(" + at + ", " + literal + ");";
            }
        }

        // Writes only defaults other than 0, false and "" (the buffer starts zeroed).
        // An enum field without default starts at the first value of the latest version.
        private static string FixedDefault(TypeRef type, string text)
        {
            if (type.Kind == TypeKind.Enum)
            {
                EnumValueDef value = text == null ? type.ResolvedEnum.First : type.ResolvedEnum.Values.First(v => v.Key == text);
                return value.Number == 0 ? null : type.TypeName + "::" + value.Key;
            }
            if (text == null)
                return null;
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    return text == "true" ? "true" : null;
                case TypeKind.FixedString:
                    return text.Length == 0 ? null : Naming.CppString(text);
                case TypeKind.Double:
                    return double.Parse(text, System.Globalization.CultureInfo.InvariantCulture) == 0 ? null : text;
                case TypeKind.Int64:
                    return text == "0" ? null : text + "ll";
                case TypeKind.UInt:
                    return text == "0" ? null : text + "u";
                case TypeKind.UInt64:
                    return text == "0" ? null : text + "ull";
                default:
                    return text == "0" ? null : text;
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
            if (field.Type.Kind == TypeKind.Array)
            {
                ArrayAccessors(w, field, FixedAt(field), true);
                return;
            }
            if (field.Type.IsFixed)
            {
                string at = FixedAt(field);
                if (field.Type.Kind == TypeKind.Enum)
                    w.Line("/// Numbers this build does not know are kept as they are (ToString shows ?(n)).");
                Method(w, type + " get_" + key + "() const", "return " + FixedGet(field, at) + ";");
                switch (field.Type.Kind)
                {
                    case TypeKind.Bool:
                        Method(w, "void set_" + key + "(bool value)", "protaco::detail::StoreBool(" + at + ", value);");
                        break;
                    case TypeKind.Enum:
                        Method(w, "void set_" + key + "(" + type + " value)", "protaco::detail::Store<int32_t>(" + at + ", static_cast<int32_t>(value));");
                        break;
                    case TypeKind.FixedString:
                        w.Line("/// Returns false, keeps the old value and writes the reason to error if longer than " + field.Type.FixedLength + " bytes.");
                        Method(w, "bool set_" + key + "(const std::string& value, std::string* error = nullptr)",
                            "return protaco::detail::StoreChars(" + at + ", " + field.Type.FixedLength + ", value, \"" + key + "\", error);");
                        break;
                    default:
                        Method(w, "void set_" + key + "(" + type + " value)", "protaco::detail::Store<" + type + ">(" + at + ", value);");
                        break;
                }
                return;
            }
            if (field.Optional)
            {
                w.Line("/// get_ returns an empty " + type + " while absent; mutable_ creates it.");
                Method(w, "bool has_" + key + "() const", "return " + key + "_.Has();");
                Method(w, "const " + type + "& get_" + key + "() const", "return " + key + "_.Get();");
                Method(w, type + "& mutable_" + key + "()", "cache_.exposed = true;", "return " + key + "_.Mutable();");
                Method(w, "void set_" + key + "(const " + type + "& value)", key + "_.Set(value);", "cache_.valid = false;");
                Method(w, "void clear_" + key + "()", key + "_.Clear();", "cache_.valid = false;");
                return;
            }
            Method(w, "const " + type + "& get_" + key + "() const", "return " + key + "_;");
            if (field.Type.Kind != TypeKind.String)
                Method(w, type + "& mutable_" + key + "()", "cache_.exposed = true;", "return " + key + "_;");
            Method(w, "void set_" + key + "(const " + type + "& value)", key + "_ = value;", "cache_.valid = false;");
        }

        // Read of a fixed field at a pointer expression. Shared by the message and its View.
        private static string FixedGet(FieldDef field, string at)
        {
            return FixedGet(field.Type, at);
        }

        private static string FixedGet(TypeRef type, string at)
        {
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    return "protaco::detail::LoadBool(" + at + ")";
                case TypeKind.Enum:
                    return "static_cast<" + CppType(type) + ">(protaco::detail::Load<int32_t>(" + at + "))";
                case TypeKind.FixedString:
                    return "protaco::detail::LoadChars(" + at + ", " + type.FixedLength + ")";
                default:
                    return "protaco::detail::Load<" + CppType(type) + ">(" + at + ")";
            }
        }

        // T[N] at a pointer expression: size, element get and set in place, and whole-array copies.
        // Element get asserts the index (debug builds); element set checks it and returns false with the reason.
        private static void ArrayAccessors(CodeWriter w, FieldDef field, string at, bool write)
        {
            TypeRef element = field.Type.Element;
            string key = field.Key;
            int count = field.Type.ArrayLength;
            int size = element.FixedSize;
            string type = CppType(element);
            string all = CppType(field.Type);
            Func<string, string> item = index => at + " + " + size + " * " + index;
            w.Line("/// Element count.");
            Method(w, "static constexpr size_t " + key + "_size()", "return " + count + ";");
            w.Line("/// Element read in place. index below " + key + "_size() (asserted in debug builds).");
            Method(w, type + " get_" + key + "(size_t index) const", "return " + FixedGet(element, item("protaco::detail::AssertIndex(index, " + count + ")")) + ";");
            w.Line("/// All elements, copied.");
            Method(w, all + " " + "get_" + key + "() const",
                all + " values;",
                "for (size_t i = 0; i < " + count + "; ++i)",
                "    values[i] = " + FixedGet(element, item("i")) + ";",
                "return values;");
            if (!write)
                return;
            string check = "if (index >= " + count + ")";
            string fail = "    return protaco::detail::IndexError(error, \"" + key + "\", index, " + count + ");";
            if (element.Kind == TypeKind.FixedString)
            {
                w.Line("/// Element write in place. False, the old value kept and the reason in error for an index out of range or a value longer than "
                    + element.FixedLength + " bytes.");
                Method(w, "bool set_" + key + "(size_t index, const std::string& value, std::string* error = nullptr)",
                    check, fail,
                    "return protaco::detail::StoreCharsAt(" + item("index") + ", " + element.FixedLength + ", value, \"" + key + "\", index, error);");
                w.Line("/// All elements. False and nothing written if any value is longer than " + element.FixedLength + " bytes.");
                Method(w, "bool set_" + key + "(const " + all + "& values, std::string* error = nullptr)",
                    "for (size_t i = 0; i < " + count + "; ++i)",
                    "{",
                    "    if (values[i].size() > " + element.FixedLength + ")",
                    "        return protaco::detail::StoreCharsAt(" + item("i") + ", " + element.FixedLength + ", values[i], \"" + key + "\", i, error);",
                    "}",
                    "for (size_t i = 0; i < " + count + "; ++i)",
                    "    protaco::detail::StoreChars(" + item("i") + ", " + element.FixedLength + ", values[i], \"" + key + "\", nullptr);",
                    "return true;");
                return;
            }
            string store = element.Kind == TypeKind.Bool ? "protaco::detail::StoreBool({0}, {1});"
                : element.Kind == TypeKind.Enum ? "protaco::detail::Store<int32_t>({0}, static_cast<int32_t>({1}));"
                : "protaco::detail::Store<" + type + ">({0}, {1});";
            w.Line("/// Element write in place. False and the reason in error for an index out of range.");
            Method(w, "bool set_" + key + "(size_t index, " + type + " value, std::string* error = nullptr)",
                check, fail,
                string.Format(store, item("index"), "value"),
                "return true;");
            w.Line("/// All elements.");
            Method(w, "void set_" + key + "(const " + all + "& values)",
                "for (size_t i = 0; i < " + count + "; ++i)",
                "    " + string.Format(store, item("i"), "values[i]"));
        }

        // Fixed-only versions: read-only access to received bytes (protaco::TryView).
        private static void View(CodeWriter w, MessageDef message, VersionView view)
        {
            w.Line();
            w.Line("/// Fixed fields read in place from received bytes (protaco::TryView<" + message.Name + ">): the bytes are not copied.");
            w.Line("/// Valid after a successful TryView while the bytes live and stay unchanged.");
            w.Line("class View : public protaco::CView");
            w.Line("{");
            w.Line("public:");
            w.Indent();
            for (int i = 0; i < view.Alive.Count; ++i)
            {
                FieldDef field = view.Alive[i];
                Doc(w, Naming.FieldDoc(field));
                if (field.Type.Kind == TypeKind.Array)
                {
                    ArrayAccessors(w, field, "Fixed() + " + field.FixedOffset, false);
                    continue;
                }
                w.Line(CppType(field.Type) + " get_" + field.Key + "() const");
                w.Line("{");
                w.Line("    return " + FixedGet(field, "Fixed() + " + field.FixedOffset) + ";");
                w.Line("}");
                if (i + 1 < view.Alive.Count)
                    w.Line();
            }
            w.Outdent();
            w.Line("};");
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
            w.Line("/// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.");
            if (view.AliveVar.Count == 0)
                w.Line("/// Fixed fields only. " + (message.AllowExtension ? "With an empty ext, serializing" : "Serializing") + " costs nothing.");
            w.Line("/// Unchanged variable fields and ext reuse the last bytes. After a mutable_ call the variable block is rebuilt every time.");
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
            w.Line("friend class protaco::CMessage<" + message.Name + ">;");
            w.Line();
            w.Line("// Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).");
            w.Line(message.Name + "(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)");
            w.Line("    : buf_(std::move(frame))");
            w.Line("{");
            w.Line("    protaco::detail::Access::BindExt(ext_, &" + message.Name + "::IsSchemaKey);");
            w.Line("}");
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
                if (field.Optional)
                {
                    w.Line("protaco::detail::Box<" + CppType(field.Type) + "> " + field.Key + "_;");
                    continue;
                }
                string init = field.Type.Kind == TypeKind.String && !string.IsNullOrEmpty(field.DefaultText) ? " = " + Naming.CppString(field.DefaultText)
                    : field.Type.Kind == TypeKind.Message ? "{protaco::detail::NoStampTag()}" : "";
                w.Line(CppType(field.Type) + " " + field.Key + "_" + init + ";");
            }
            w.Line("protaco::CExt ext_;" + (message.AllowExtension ? "" : "   // stays empty: allow_extension false"));
            w.Line("mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block");
        }
    }
}
