// SchemaFingerprint.cs. 32-bit fingerprint of one version of a message schema. Every frame header carries the
// fingerprints of the versions in its window; a receiver compares the one for its own version.
// Inserted, removed, reordered or retyped fields are caught even when the byte count stays the same.
// Canonical text: README.md "Schema fingerprint".
using System.Linq;
using System.Text;

namespace Protacoc
{
    internal static class SchemaFingerprint
    {
        // Referenced messages must already have their fingerprint (Linker walks dependencies first).
        public static uint Of(MessageDef message, int version)
        {
            return Fnv1a(Encoding.UTF8.GetBytes(CanonicalText(message, version)));
        }

        // protaco-schema-1|<name>|<version>|<ext 0/1>|<key>:<type>[!][=<default>];...
        // Fields in serialization order. A removed fixed field keeps its bytes and is written ~<key>:<type>;
        // a removed variable field is left out. added and removed themselves are not part of the text,
        // so adding a version leaves the fingerprints of older versions unchanged.
        // Namespace, comments, desc, owner and eq are not part of the contract on the wire.
        public static string CanonicalText(MessageDef message, int version)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("protaco-schema-1|").Append(message.Name).Append('|').Append(version).Append('|');
            sb.Append(message.AllowExtension ? '1' : '0').Append('|');
            foreach (FieldDef field in message.Physical.Where(f => f.Added <= version))
            {
                if (!field.AliveAt(version))
                {
                    if (field.Type.IsFixed)
                        sb.Append('~').Append(field.Key).Append(':').Append(TypeText(field.Type)).Append(';');
                    continue;
                }
                sb.Append(field.Key).Append(':').Append(TypeText(field.Type));
                if (field.Required)
                    sb.Append('!');
                if (field.DefaultText != null)
                    sb.Append('=').Append(field.DefaultText);
                sb.Append(';');
            }
            return sb.ToString();
        }

        private static string TypeText(TypeRef type)
        {
            switch (type.Kind)
            {
                case TypeKind.List:
                    return "list<" + TypeText(type.Element) + ">";
                case TypeKind.Message:
                    return type.MessageName + "@" + type.Resolved.SchemaHash.ToString("x8");
                default:
                    return type.ToString();
            }
        }

        private static uint Fnv1a(byte[] bytes)
        {
            uint hash = 2166136261u;
            foreach (byte b in bytes)
            {
                hash ^= b;
                hash = unchecked(hash * 16777619u);
            }
            return hash;
        }

        public static string Hex(uint hash)
        {
            return "0x" + hash.ToString("X8");
        }
    }
}
