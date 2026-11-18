// NatvisEmitter.cs. Visual Studio debugger view (.natvis) of the generated C++ messages.
// Fixed fields live in the frame buffer, not in members, so the watch window cannot show them by name.
// The natvis file maps every field to an expression: fixed fields read the frame bytes at their offset, variable fields show their member.
// It changes nothing at run time (no members, no sizeof change). Add the file to the C++ project; the linker embeds it in the PDB.
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Protacoc
{
    internal static class NatvisEmitter
    {
        // MSVC std::vector<uint8_t> data pointer (VS2017 and later STL layout).
        private const string VectorData = "buf_._Mypair._Myval2._Myfirst";

        public static string Emit(List<MessageDef> messages)
        {
            StringBuilder s = new StringBuilder();
            s.Append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
            s.Append("<!-- protacoc " + Naming.Version + ". Debugger view of the generated messages. Do not edit. -->\n");
            s.Append("<AutoVisualizer xmlns=\"http://schemas.microsoft.com/vstudio/debugger/natvis/2010\">\n");
            foreach (MessageDef message in messages)
            {
                string ns = message.Namespace.Replace(".", "::");
                foreach (VersionView view in message.Views)
                {
                    string type = view == message.Latest ? ns + "::" + message.Name : ns + "::v" + view.Version + "::" + message.Name;
                    int hashCount = view.Version - message.MinVersion + 1;
                    int fixedOffset = CppEmitter.HeaderBytesWithoutName + message.Name.Length + 4 * hashCount + 4;
                    TypeEntry(s, type, message.Name + " v" + view.Version, view, "(" + VectorData + " + " + fixedOffset + ")", true);
                }
                if (message.Fixed)
                {
                    // Inline views of a fixed message point at its bytes inside another message.
                    TypeEntry(s, ns + "::" + message.Name + "::ConstRef", message.Name, message.Latest, "p_", false);
                    TypeEntry(s, ns + "::" + message.Name + "::Ref", message.Name, message.Latest, "p_", false);
                }
            }
            s.Append("</AutoVisualizer>\n");
            return s.ToString();
        }

        private static void TypeEntry(StringBuilder s, string type, string display, VersionView view, string block, bool members)
        {
            s.Append("  <Type Name=\"" + type + "\">\n");
            s.Append("    <DisplayString>" + display + "</DisplayString>\n");
            s.Append("    <Expand>\n");
            foreach (FieldDef field in view.Alive)
            {
                string value = field.Type.IsFixed ? FixedValue(field, "(unsigned char*)" + block + " + " + field.FixedOffset) : (members ? field.Key + "_" : null);
                if (value != null)
                    s.Append("      <Item Name=\"" + field.Key + "\">" + value + "</Item>\n");
            }
            s.Append("    </Expand>\n");
            s.Append("  </Type>\n");
        }

        // Expression of a fixed field at byte address at.
        private static string FixedValue(FieldDef field, string at)
        {
            TypeRef type = field.Type;
            string scalar = ScalarType(type);
            if (scalar != null)
                return "*(" + scalar + "*)(" + at + ")";
            switch (type.Kind)
            {
                case TypeKind.FixedString:
                    return "(char*)(" + at + "),[" + type.FixedLength + "]s8";
                case TypeKind.Array:
                    string element = ScalarType(type.Element);
                    if (element != null)
                        return "(" + element + "*)(" + at + "),[" + type.ArrayLength + "]";
                    return "(unsigned char*)(" + at + "),[" + type.FixedSize + "]";
                default:
                    // Inline fixed message: its bytes.
                    return "(unsigned char*)(" + at + "),[" + type.FixedSize + "]";
            }
        }

        private static string ScalarType(TypeRef type)
        {
            switch (type.Kind)
            {
                case TypeKind.Bool:
                    return "bool";
                case TypeKind.Int:
                    return "int";
                case TypeKind.UInt:
                    return "unsigned int";
                case TypeKind.Int64:
                    return "__int64";
                case TypeKind.UInt64:
                    return "unsigned __int64";
                case TypeKind.Double:
                    return "double";
                case TypeKind.Enum:
                    return type.ResolvedEnum != null ? type.ResolvedEnum.Namespace.Replace(".", "::") + "::" + type.ResolvedEnum.Name : "int";
                default:
                    return null;
            }
        }
    }
}
