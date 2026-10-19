// YamlLite.cs. YAML subset reader for message files. Line based, no external library.
//
// Accepted forms (anything else is PC101):
//   # comment                    -> before the first top-level key: message docstring; right above an item: item docstring
//   key: value                   -> top-level scalar (quotes allowed, trailing # comment allowed)
//   key:                         -> followed by "  - {k: v, ...}" items, one item per line
// All values are read as text. Loader interprets bool, int and types (no implicit YAML typing).
using System.Collections.Generic;
using System.Text;

namespace Protacoc
{
    internal sealed class YScalar
    {
        public string Text;
        public bool Quoted;
        public SourcePos Pos;
    }

    internal sealed class YMap
    {
        public List<KeyValuePair<YScalar, object>> Entries = new List<KeyValuePair<YScalar, object>>();   // value: YScalar
    }

    internal sealed class YItem
    {
        public YMap Map;
        public List<string> Doc = new List<string>();
        public SourcePos Pos;
    }

    internal sealed class YTop
    {
        public YScalar Key;
        public object Value;                       // YScalar | List<YItem>
    }

    internal sealed class YDoc
    {
        public List<YTop> Entries = new List<YTop>();
        public List<string> HeaderDoc = new List<string>();
    }

    internal static class YamlLite
    {
        public static YDoc Parse(string text, string file, Diag diag)
        {
            YDoc doc = new YDoc();
            List<string> pending = new List<string>();
            YTop openList = null;                 // top-level key whose items are being collected
            string[] lines = text.Replace("\r\n", "\n").Split('\n');
            for (int i = 0; i < lines.Length; ++i)
            {
                int lineNo = i + 1;
                string line = lines[i].TrimEnd();
                if (lineNo == 1 && line.Length > 0 && line[0] == '﻿')
                    line = line.Substring(1);
                if (line.IndexOf('\t') >= 0)
                {
                    diag.Error(new SourcePos(file, lineNo, line.IndexOf('\t') + 1), Codes.Syntax, "tabs are not allowed. Indent with spaces");
                    continue;
                }
                string trimmed = line.TrimStart();
                int indent = line.Length - trimmed.Length;
                if (trimmed.Length == 0)
                {
                    // A blank line detaches pending item comments. In the header comment it is kept as a paragraph break.
                    if (doc.Entries.Count == 0)
                    {
                        if (doc.HeaderDoc.Count > 0 || pending.Count > 0)
                            pending.Add("");
                    }
                    else
                    {
                        pending.Clear();
                    }
                    continue;
                }
                if (trimmed[0] == '#')
                {
                    pending.Add(CommentText(trimmed));
                    continue;
                }
                if (indent == 0)
                {
                    if (doc.Entries.Count == 0)
                        doc.HeaderDoc.AddRange(TrimBlank(pending));
                    pending.Clear();
                    openList = ParseTop(line, lineNo, file, diag, doc);
                    continue;
                }
                if (trimmed.StartsWith("- ") || trimmed == "-")
                {
                    if (openList == null)
                    {
                        diag.Error(new SourcePos(file, lineNo, indent + 1), Codes.Syntax, "list item out of place. Items go right below a 'key:' line");
                        pending.Clear();
                        continue;
                    }
                    YItem item = ParseItem(line, indent, lineNo, file, diag);
                    if (item != null)
                    {
                        item.Doc.AddRange(TrimBlank(pending));
                        ((List<YItem>)openList.Value).Add(item);
                    }
                    pending.Clear();
                    continue;
                }
                diag.Error(new SourcePos(file, lineNo, indent + 1), Codes.Syntax,
                    "unsupported YAML. Write each item on one line as '- {key: ..., type: ...}'");
                pending.Clear();
            }
            return doc;
        }

        private static string CommentText(string trimmed)
        {
            string body = trimmed.Substring(1);
            return body.StartsWith(" ") ? body.Substring(1) : body;
        }

        private static List<string> TrimBlank(List<string> lines)
        {
            List<string> result = new List<string>(lines);
            while (result.Count > 0 && result[result.Count - 1].Length == 0)
                result.RemoveAt(result.Count - 1);
            while (result.Count > 0 && result[0].Length == 0)
                result.RemoveAt(0);
            return result;
        }

        // Top-level "key: value". An empty value starts an item list on the following lines (returned).
        private static YTop ParseTop(string line, int lineNo, string file, Diag diag, YDoc doc)
        {
            int colon = line.IndexOf(':');
            if (colon <= 0)
            {
                diag.Error(new SourcePos(file, lineNo, 1), Codes.Syntax, "bad top-level line. Expected 'key: value'");
                return null;
            }
            YTop top = new YTop();
            top.Key = new YScalar { Text = line.Substring(0, colon).Trim(), Pos = new SourcePos(file, lineNo, 1) };
            Cursor cursor = new Cursor(line, colon + 1, lineNo, file, diag);
            cursor.SkipSpaces();
            if (cursor.AtEndOrComment())
            {
                top.Value = new List<YItem>();
                doc.Entries.Add(top);
                return top;
            }
            YScalar value = cursor.ReadScalar(false);
            cursor.ExpectEnd();
            top.Value = value;
            doc.Entries.Add(top);
            return null;
        }

        private static YItem ParseItem(string line, int indent, int lineNo, string file, Diag diag)
        {
            Cursor cursor = new Cursor(line, indent + 1, lineNo, file, diag);
            cursor.SkipSpaces();
            if (cursor.Peek() != '{')
            {
                diag.Error(cursor.Pos(), Codes.Syntax, "bad item. Expected a one-line flow mapping '{k: v, ...}'");
                return null;
            }
            YItem item = new YItem { Pos = new SourcePos(file, lineNo, indent + 1) };
            item.Map = cursor.ReadMap();
            if (item.Map == null)
                return null;
            // A trailing comment is appended to the docstring.
            string trailing = cursor.ReadTrailingComment();
            cursor.ExpectEnd();
            if (trailing != null)
                item.Doc.Add(trailing);
            return item;
        }

        // Forward cursor within one line.
        private sealed class Cursor
        {
            private readonly string line_;
            private readonly int lineNo_;
            private readonly string file_;
            private readonly Diag diag_;
            private int pos_;
            private bool failed_;

            public Cursor(string line, int start, int lineNo, string file, Diag diag)
            {
                line_ = line;
                pos_ = start;
                lineNo_ = lineNo;
                file_ = file;
                diag_ = diag;
            }

            public SourcePos Pos()
            {
                return new SourcePos(file_, lineNo_, pos_ + 1);
            }

            public char Peek()
            {
                return pos_ < line_.Length ? line_[pos_] : '\0';
            }

            public void SkipSpaces()
            {
                while (pos_ < line_.Length && line_[pos_] == ' ')
                    ++pos_;
            }

            public bool AtEndOrComment()
            {
                return pos_ >= line_.Length || line_[pos_] == '#';
            }

            public string ReadTrailingComment()
            {
                SkipSpaces();
                if (pos_ < line_.Length && line_[pos_] == '#')
                {
                    string text = CommentText(line_.Substring(pos_));
                    pos_ = line_.Length;
                    return text;
                }
                return null;
            }

            public void ExpectEnd()
            {
                if (failed_)
                    return;
                SkipSpaces();
                if (pos_ < line_.Length && line_[pos_] == '#')
                    return;
                if (pos_ < line_.Length)
                    Fail("unexpected text at end of line: '" + line_.Substring(pos_) + "'");
            }

            public YMap ReadMap()
            {
                YMap map = new YMap();
                ++pos_;   // '{'
                SkipSpaces();
                if (Peek() == '}')
                {
                    ++pos_;
                    return map;
                }
                while (!failed_)
                {
                    SkipSpaces();
                    SourcePos keyPos = Pos();
                    int colon = line_.IndexOf(':', pos_);
                    if (colon < 0)
                    {
                        Fail("missing colon in 'key: value'. Quote values that contain ',' with \"...\"");
                        return null;
                    }
                    YScalar key = new YScalar { Text = line_.Substring(pos_, colon - pos_).Trim(), Pos = keyPos };
                    pos_ = colon + 1;
                    SkipSpaces();
                    YScalar value = ReadScalar(true);
                    if (failed_)
                        return null;
                    // The ',' of map<K, V> ends a plain value inside {...}.
                    if (!value.Quoted && value.Text.StartsWith("map<", System.StringComparison.Ordinal) && !value.Text.EndsWith(">", System.StringComparison.Ordinal))
                    {
                        Fail("map types contain ',': quote them inside {...}, for example type: \"map<string, int>\"");
                        return null;
                    }
                    map.Entries.Add(new KeyValuePair<YScalar, object>(key, value));
                    SkipSpaces();
                    char c = Peek();
                    ++pos_;
                    if (c == '}')
                        return map;
                    if (c != ',')
                    {
                        Fail("expected ',' or '}'");
                        return null;
                    }
                }
                return null;
            }

            // Inside flow, a value ends at , or }. " #" starts a comment.
            public YScalar ReadScalar(bool inFlow)
            {
                YScalar scalar = new YScalar { Pos = Pos() };
                char c = Peek();
                if (c == '[')
                {
                    Fail("flow lists '[...]' are not supported. Quote the value with \"...\"");
                    return scalar;
                }
                if (c == '"' || c == '\'')
                {
                    scalar.Quoted = true;
                    scalar.Text = ReadQuoted(c);
                    return scalar;
                }
                int start = pos_;
                while (pos_ < line_.Length)
                {
                    char ch = line_[pos_];
                    if (inFlow && (ch == ',' || ch == '}'))
                        break;
                    if (ch == '#' && pos_ > start && line_[pos_ - 1] == ' ')
                        break;
                    if (ch == ':' && pos_ + 1 < line_.Length && line_[pos_ + 1] == ' ')
                    {
                        Fail("unquoted value contains ': '. Quote it with \"...\"");
                        break;
                    }
                    ++pos_;
                }
                scalar.Text = line_.Substring(start, pos_ - start).Trim();
                return scalar;
            }

            private string ReadQuoted(char quote)
            {
                StringBuilder sb = new StringBuilder();
                ++pos_;
                while (pos_ < line_.Length)
                {
                    char ch = line_[pos_];
                    if (quote == '\'' && ch == '\'')
                    {
                        if (pos_ + 1 < line_.Length && line_[pos_ + 1] == '\'')
                        {
                            sb.Append('\'');
                            pos_ += 2;
                            continue;
                        }
                        ++pos_;
                        return sb.ToString();
                    }
                    if (quote == '"' && ch == '"')
                    {
                        ++pos_;
                        return sb.ToString();
                    }
                    if (quote == '"' && ch == '\\' && pos_ + 1 < line_.Length)
                    {
                        char next = line_[pos_ + 1];
                        sb.Append(next == 'n' ? '\n' : next == 't' ? '\t' : next);
                        pos_ += 2;
                        continue;
                    }
                    sb.Append(ch);
                    ++pos_;
                }
                Fail("unterminated quote");
                return sb.ToString();
            }

            private void Fail(string message)
            {
                if (failed_)
                    return;
                failed_ = true;
                diag_.Error(Pos(), Codes.Flow, message);
                pos_ = line_.Length;
            }
        }
    }
}
