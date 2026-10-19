// Diag.cs. Error collection. MSBuild format "file(line,col): error CODE: text", clickable in the VS output window.
using System;
using System.Collections.Generic;

namespace Protacoc
{
    internal sealed class Diag
    {
        private readonly List<string> errors_ = new List<string>();

        public int ErrorCount
        {
            get
            {
                return errors_.Count;
            }
        }

        public IReadOnlyList<string> Errors
        {
            get
            {
                return errors_;
            }
        }

        public void Error(SourcePos pos, string code, string message)
        {
            errors_.Add(string.Format("{0}({1},{2}): error {3}: {4}", pos.File, pos.Line, pos.Col, code, message));
        }

        public void Error(string code, string message)
        {
            errors_.Add(string.Format("protacoc: error {0}: {1}", code, message));
        }

        public void Print()
        {
            foreach (string line in errors_)
                Console.Error.WriteLine(line);
        }
    }

    // Error codes. List: docs/reference.md "Error codes".
    internal static class Codes
    {
        public const string Syntax = "PC101";          // unsupported YAML construct
        public const string Flow = "PC102";            // { } [ ] pairing, colon, quotes
        public const string UnknownTop = "PC201";      // unknown top-level key
        public const string MissingTop = "PC202";      // missing required top-level key
        public const string UnknownAttr = "PC203";     // unknown field attribute
        public const string BadValue = "PC204";        // value format (bool, int, name)
        public const string DupAttr = "PC205";         // duplicate attribute
        public const string BadNamespace = "PC206";    // malformed namespace or keyword segment
        public const string FileKind = "PC208";        // enum: outside <Name>.enum.yaml, or a .enum.yaml file without enum:
        public const string FileName = "PC301";        // message name differs from file name
        public const string DupKey = "PC302";          // duplicate field key
        public const string Reserved = "PC303";        // reserved word or generated member name
        public const string BadType = "PC304";         // malformed type
        public const string RequiredFixed = "PC305";   // required on a fixed field
        public const string RequiredSelf = "PC306";    // required on a field of the message's own type
        public const string BadDefault = "PC308";      // malformed default
        public const string BadLength = "PC310";       // N of string(N)
        public const string NameClash = "PC311";       // C# name clash
        public const string BadVersion = "PC312";      // min_version, added or removed out of range
        public const string DeadLine = "PC313";        // variable field removed at or below min_version
        public const string ReservedName = "PC314";    // message name v<N> (version namespace)
        public const string DupNumber = "PC318";       // two enum values with one number
        public const string DupMessage = "PC401";      // duplicate message name
        public const string UnknownRef = "PC402";      // unknown message reference
        public const string Cycle = "PC403";           // reference cycle, or a self reference other than list<Self> and Self
        public const string BadElement = "PC404";      // type not allowed as list element, map key or map value
        public const string NamespaceMismatch = "PC405"; // files of one run declare different namespaces
        public const string TooDeep = "PC406";         // message nesting deeper than the runtime limit
        public const string NestedVersion = "PC407";   // message used as a field type has version attributes
        public const string Io = "PC501";              // file I/O
        public const string Usage = "PC502";           // command line
    }
}
