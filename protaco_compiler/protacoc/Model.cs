// Model.cs. Message and enum definitions read from yaml. Shared by all generator stages.
using System.Collections.Generic;
using System.Linq;

namespace Protacoc
{
    internal enum TypeKind
    {
        Bool,
        Int,
        Int64,
        UInt,
        UInt64,
        Double,
        String,
        FixedString,
        Bytes,
        List,
        Map,
        Message,
        Enum,
        Array,
    }

    internal sealed class TypeRef
    {
        public TypeKind Kind;
        public int FixedLength;          // byte count of FixedString
        public TypeRef Element;          // element of List and Array, value of Map
        public TypeRef Key;              // key of Map
        public string TypeName;          // referenced message or enum name
        public MessageDef Resolved;      // referenced message, set by the linker
        public EnumDef ResolvedEnum;     // referenced enum, set by the linker
        public string LengthText;        // N of T[N] as written: a number, Const.Key or Enum.Value
        public int ArrayLength;          // element count of Array, set by the linker

        // Whether the type lives in the fixed block. Scalars, enums, string(N), fixed messages (inline) and T[N] of those.
        public bool IsFixed
        {
            get
            {
                return Kind == TypeKind.Bool || Kind == TypeKind.Int || Kind == TypeKind.Int64 || Kind == TypeKind.UInt
                    || Kind == TypeKind.UInt64 || Kind == TypeKind.Double || Kind == TypeKind.FixedString || Kind == TypeKind.Enum
                    || Kind == TypeKind.Array || (Kind == TypeKind.Message && Resolved != null && Resolved.Fixed);
            }
        }

        public int FixedSize
        {
            get
            {
                switch (Kind)
                {
                    case TypeKind.Bool:
                        return 1;
                    case TypeKind.Int:
                    case TypeKind.UInt:
                    case TypeKind.Enum:
                        return 4;
                    case TypeKind.Int64:
                    case TypeKind.UInt64:
                    case TypeKind.Double:
                        return 8;
                    case TypeKind.FixedString:
                        return FixedLength;
                    case TypeKind.Array:
                        return (int)System.Math.Min((long)Element.FixedSize * ArrayLength, int.MaxValue);
                    case TypeKind.Message:
                        // An inline fixed message: its fixed block, known once the linker laid it out.
                        return Resolved != null && Resolved.Fixed && Resolved.Views.Count != 0 ? Resolved.Latest.FixedSize : 0;
                    default:
                        return 0;
                }
            }
        }

        public override string ToString()
        {
            switch (Kind)
            {
                case TypeKind.Bool:
                    return "bool";
                case TypeKind.Int:
                    return "int";
                case TypeKind.Int64:
                    return "int64";
                case TypeKind.UInt:
                    return "uint";
                case TypeKind.UInt64:
                    return "uint64";
                case TypeKind.Double:
                    return "double";
                case TypeKind.String:
                    return "string";
                case TypeKind.FixedString:
                    return "string(" + FixedLength + ")";
                case TypeKind.Bytes:
                    return "bytes";
                case TypeKind.List:
                    return "list<" + Element + ">";
                case TypeKind.Map:
                    return "map<" + Key + ", " + Element + ">";
                case TypeKind.Array:
                    return Element + "[" + LengthText + "]";
                default:
                    return TypeName;
            }
        }

        // Message and enum types inside this type: itself, list elements, map keys and values.
        public IEnumerable<TypeRef> Named()
        {
            if (Kind == TypeKind.Message || Kind == TypeKind.Enum)
                yield return this;
            if (Key != null)
            {
                foreach (TypeRef inner in Key.Named())
                    yield return inner;
            }
            if (Element != null)
            {
                foreach (TypeRef inner in Element.Named())
                    yield return inner;
            }
        }
    }

    // Common to message and enum files: one definition per file, file name = definition name.
    internal abstract class SchemaDef
    {
        public string Name;
        public int Version;
        public int MinVersion;            // oldest supported version; equals Version without min_version
        public string Namespace;          // "shop" or dotted "shop.orders"
        public string Desc;
        public List<string> Doc = new List<string>();
        public string FilePath;
        public string FileName;
        public string Sha1;
        public SourcePos Pos;
    }

    internal sealed class FieldDef
    {
        public string Key;
        public TypeRef Type;
        public bool Required;
        public string DefaultText;       // null means no default
        public bool DefaultQuoted;
        public string Desc;
        public List<string> Doc = new List<string>();   // yaml comments -> docstring
        public SourcePos Pos;
        public int Added = 1;            // first version with this field
        public int Removed;              // first version without it; 0 = never removed
        public int Index;                // declaration index
        public bool Optional;            // a single field of its own message type: absent until set (set by the linker)

        public int FixedOffset = -1;     // offset in the fixed block, same in every version (set by the linker)

        public bool AliveAt(int version)
        {
            return Added <= version && (Removed == 0 || version < Removed);
        }
    }

    // One version of a message: the layout a frame of that version has.
    internal sealed class VersionView
    {
        public int Version;
        public List<FieldDef> Alive = new List<FieldDef>();       // fields of this version, yaml order (accessors, ToString)
        public List<FieldDef> FixedSlots = new List<FieldDef>();  // fixed block of this version, removed fields included
        public List<FieldDef> AliveVar = new List<FieldDef>();    // variable fields of this version, serialization order
        public int FixedSize;
        public uint SchemaHash;           // FNV-1a 32 of the canonical schema text (docs/reference.md "Schema fingerprint")
    }

    internal sealed class MessageDef : SchemaDef
    {
        public bool AllowExtension = true;
        public bool AllowExtensionSet;    // allow_extension written in the yaml
        public bool AutoEnvelope = true;  // false: new objects keep id 0 and ts 0. Not part of the fingerprint
        public bool Fixed;                // fixed: true. Fixed fields only; used as a field type it is inline in the fixed block
        public List<FieldDef> Fields = new List<FieldDef>();

        // Set by the linker.
        public List<FieldDef> Physical = new List<FieldDef>();      // serialization order: added, then declaration
        public List<VersionView> Views = new List<VersionView>();   // MinVersion..Version
        public List<MessageDef> Dependencies = new List<MessageDef>();
        public List<EnumDef> EnumDependencies = new List<EnumDef>();
        public int Depth;                 // nested message levels below this message

        public VersionView Latest
        {
            get
            {
                return Views[Views.Count - 1];
            }
        }

        public uint SchemaHash
        {
            get
            {
                return Latest.SchemaHash;
            }
        }

        // More than one supported version, or version attributes on fields.
        public bool UsesVersions
        {
            get
            {
                return MinVersion < Version || Fields.Exists(f => f.Added != 1 || f.Removed != 0);
            }
        }
    }

    internal sealed class EnumValueDef
    {
        public string Key;
        public int Number;
        public string Desc;
        public List<string> Doc = new List<string>();
        public SourcePos Pos;
        public int Added = 1;
        public int Removed;

        public bool AliveAt(int version)
        {
            return Added <= version && (Removed == 0 || version < Removed);
        }
    }

    // Enum file (<Name>.enum.yaml). Values are int32. Removed values keep their names for ToString of old frames.
    internal sealed class EnumDef : SchemaDef
    {
        public List<EnumValueDef> Values = new List<EnumValueDef>();

        public List<EnumValueDef> Alive
        {
            get
            {
                return Values.Where(v => v.AliveAt(Version)).ToList();
            }
        }

        // Default of a field of this enum without default: the first value of the latest version.
        public EnumValueDef First
        {
            get
            {
                return Values.FirstOrDefault(v => v.AliveAt(Version));
            }
        }
    }

    internal sealed class ConstValueDef
    {
        public string Key;
        public int Number;
        public string Desc;
        public List<string> Doc = new List<string>();
        public SourcePos Pos;
    }

    // Const file (<Name>.const.yaml). Named int32 values for code and array lengths. Not serialized, so no version.
    internal sealed class ConstDef : SchemaDef
    {
        public List<ConstValueDef> Values = new List<ConstValueDef>();
    }

    internal struct SourcePos
    {
        public string File;
        public int Line;
        public int Col;

        public SourcePos(string file, int line, int col)
        {
            File = file;
            Line = line;
            Col = col;
        }
    }
}
