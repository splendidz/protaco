// Model.cs. Message definitions read from yaml. Shared by all generator stages.
using System.Collections.Generic;

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
        Message,
    }

    internal sealed class TypeRef
    {
        public TypeKind Kind;
        public int FixedLength;          // byte count of FixedString
        public TypeRef Element;          // element of List
        public string MessageName;       // referenced message name
        public MessageDef Resolved;      // set by the linker

        // Whether the type lives in the fixed block. Only scalars and string(N).
        public bool IsFixed
        {
            get
            {
                return Kind == TypeKind.Bool || Kind == TypeKind.Int || Kind == TypeKind.Int64 || Kind == TypeKind.UInt
                    || Kind == TypeKind.UInt64 || Kind == TypeKind.Double || Kind == TypeKind.FixedString;
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
                        return 4;
                    case TypeKind.Int64:
                    case TypeKind.UInt64:
                    case TypeKind.Double:
                        return 8;
                    case TypeKind.FixedString:
                        return FixedLength;
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
                default:
                    return MessageName;
            }
        }
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

    internal sealed class MessageDef
    {
        public string Name;
        public int Version;
        public int MinVersion;            // oldest supported version; equals Version without min_version
        public string Namespace;          // "shop" or dotted "shop.orders"
        public bool AllowExtension = true;
        public string Desc;
        public List<string> Doc = new List<string>();
        public List<FieldDef> Fields = new List<FieldDef>();
        public string FilePath;
        public string FileName;
        public string Sha1;
        public SourcePos Pos;

        // Set by the linker.
        public List<FieldDef> Physical = new List<FieldDef>();      // serialization order: added, then declaration
        public List<VersionView> Views = new List<VersionView>();   // MinVersion..Version
        public List<MessageDef> Dependencies = new List<MessageDef>();
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
