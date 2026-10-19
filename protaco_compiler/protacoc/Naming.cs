// Naming.cs. Name conversion, literal escaping, docstring assembly, output files (write-if-changed).
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Protacoc
{
    internal static class Naming
    {
        public const string Version = "0.5.0";

        // C# property name: first letter upper-cased (prevState -> PrevState).
        public static string CsProperty(string key)
        {
            return char.ToUpperInvariant(key[0]) + key.Substring(1);
        }

        // String bytes use the ANSI code page. C# uses Encoding.Default.
        public static int AcpLength(string text)
        {
            return Encoding.Default.GetByteCount(text);
        }

        public static bool IsAscii(string text)
        {
            return text.All(c => c < 0x80);
        }

        public static string CppString(string text)
        {
            return "\"" + Escape(text) + "\"";
        }

        public static string CsString(string text)
        {
            return "\"" + Escape(text) + "\"";
        }

        private static string Escape(string text)
        {
            StringBuilder sb = new StringBuilder();
            foreach (char c in text)
            {
                switch (c)
                {
                    case '\\':
                        sb.Append("\\\\");
                        break;
                    case '"':
                        sb.Append("\\\"");
                        break;
                    case '\n':
                        sb.Append("\\n");
                        break;
                    case '\t':
                        sb.Append("\\t");
                        break;
                    default:
                        sb.Append(c);
                        break;
                }
            }
            return sb.ToString();
        }

        // yaml comments + desc + attribute summary -> docstring lines. Emitters add the language prefix.
        public static List<string> FieldDoc(FieldDef field)
        {
            List<string> lines = new List<string>(field.Doc);
            if (!string.IsNullOrEmpty(field.Desc))
                lines.Add(field.Desc);
            List<string> tags = new List<string>();
            tags.Add("type " + field.Type);
            if (field.Type.Kind == TypeKind.FixedString)
                tags.Add("fixed " + field.Type.FixedLength + " bytes, set rejects longer values");
            if (field.Required)
                tags.Add("required, decoding fails if missing");
            if (field.DefaultText != null)
                tags.Add("default " + (field.DefaultQuoted || field.Type.Kind == TypeKind.String || field.Type.Kind == TypeKind.FixedString ? "\"" + field.DefaultText + "\"" : field.DefaultText));
            if (field.Added != 1)
                tags.Add("added in " + field.Added);
            if (field.Removed != 0)
                tags.Add("removed in " + field.Removed);
            lines.Add("[" + string.Join("; ", tags) + "]");
            return lines;
        }

        public static List<string> MessageDoc(MessageDef message, VersionView view)
        {
            List<string> lines = new List<string>(message.Doc);
            if (!string.IsNullOrEmpty(message.Desc))
                lines.Add(message.Desc);
            if (view != message.Latest)
                lines.Add("Version " + view.Version + " layout, for frames from peers on version " + view.Version + ".");
            List<string> tags = new List<string> { "version " + view.Version, "schema " + SchemaFingerprint.Hex(view.SchemaHash) };
            if (message.MinVersion < message.Version)
                tags.Add("supports " + message.MinVersion + ".." + message.Version);
            if (!message.AllowExtension)
                tags.Add("allow_extension false");
            tags.Add("source " + message.FileName);
            lines.Add("[" + string.Join("; ", tags) + "]");
            return lines;
        }

        public static string XmlEscape(string text)
        {
            return text.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;");
        }
    }

    // Generated files: UTF-8 with BOM, CRLF. Unchanged content is not rewritten (no needless recompiles).
    internal sealed class OutputSet
    {
        private readonly bool check_;
        private readonly Diag diag_;
        private int written_;
        private int unchanged_;
        private readonly List<string> stale_ = new List<string>();

        public OutputSet(bool check, Diag diag)
        {
            check_ = check;
            diag_ = diag;
        }

        public IReadOnlyList<string> Stale
        {
            get
            {
                return stale_;
            }
        }

        public void Write(string path, string content)
        {
            string normalized = content.Replace("\r\n", "\n").Replace("\n", "\r\n");
            byte[] bytes = new UTF8Encoding(true).GetPreamble().Concat(new UTF8Encoding(false).GetBytes(normalized)).ToArray();
            if (File.Exists(path) && File.ReadAllBytes(path).SequenceEqual(bytes))
            {
                ++unchanged_;
                return;
            }
            if (check_)
            {
                stale_.Add(path);
                return;
            }
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path)));
                File.WriteAllBytes(path, bytes);
                ++written_;
            }
            catch (Exception e)
            {
                diag_.Error(Codes.Io, path + ": " + e.Message);
            }
        }

        public string Summary()
        {
            return check_
                ? string.Format("protacoc: checked, {0} up to date, {1} out of date", unchanged_, stale_.Count)
                : string.Format("protacoc: {0} written, {1} unchanged", written_, unchanged_);
        }
    }

    // Line writer with indentation.
    internal sealed class CodeWriter
    {
        private readonly StringBuilder sb_ = new StringBuilder();
        private int indent_;

        public void Line(string text = "")
        {
            if (text.Length == 0)
                sb_.Append('\n');
            else
                sb_.Append(new string(' ', indent_ * 4)).Append(text).Append('\n');
        }

        public void Open(string text)
        {
            Line(text);
            Line("{");
            ++indent_;
        }

        public void Close(string suffix = "")
        {
            --indent_;
            Line("}" + suffix);
        }

        public void Indent()
        {
            ++indent_;
        }

        public void Outdent()
        {
            --indent_;
        }

        public override string ToString()
        {
            return sb_.ToString();
        }
    }
}
