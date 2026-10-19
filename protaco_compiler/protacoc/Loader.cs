// Loader.cs. YDoc -> MessageDef. Checks key and value formats and per-field rules.
// Cross-message rules (references, cycles, layout) belong to Linker.
using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

namespace Protacoc
{
    internal static class Loader
    {
        private static readonly Regex Ident = new Regex("^[A-Za-z_][A-Za-z0-9_]*$");
        private static readonly Regex IntText = new Regex("^-?(0|[1-9][0-9]*)$");
        private static readonly Regex FixedStringType = new Regex("^string\\(([0-9]+)\\)$");
        private static readonly Regex ListType = new Regex("^list<(.+)>$");

        private static readonly HashSet<string> TopKeys = new HashSet<string> { "message", "version", "min_version", "namespace", "allow_extension", "desc", "fields" };
        private static readonly HashSet<string> FieldAttrs = new HashSet<string> { "key", "type", "required", "default", "added", "removed", "desc" };

        // Integer text the same in C++ and C#: no sign '+', no leading zero (C++ reads 010 as octal 8).
        public static bool IsIntText(string text)
        {
            return IntText.IsMatch(text);
        }

        public const int MaxFixedLength = 4096;

        public static MessageDef Load(string path, Diag diag)
        {
            byte[] bytes;
            try
            {
                bytes = File.ReadAllBytes(path);
            }
            catch (Exception e)
            {
                diag.Error(Codes.Io, path + ": " + e.Message);
                return null;
            }
            string text = new UTF8Encoding(false).GetString(bytes);
            int before = diag.ErrorCount;
            YDoc doc = YamlLite.Parse(text, path, diag);
            if (diag.ErrorCount != before)
                return null;
            MessageDef message = new MessageDef
            {
                FilePath = path,
                FileName = Path.GetFileName(path),
                Sha1 = Sha1Hex(bytes),
                Pos = new SourcePos(path, 1, 1),
            };
            message.Doc.AddRange(doc.HeaderDoc);
            ReadTop(doc, message, diag);
            return diag.ErrorCount == before ? message : null;
        }

        private static string Sha1Hex(byte[] bytes)
        {
            using (SHA1 sha = SHA1.Create())
            {
                byte[] hash = sha.ComputeHash(bytes);
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < 6; ++i)
                    sb.Append(hash[i].ToString("x2"));
                return sb.ToString();
            }
        }

        private static void ReadTop(YDoc doc, MessageDef message, Diag diag)
        {
            HashSet<string> seen = new HashSet<string>();
            foreach (YTop top in doc.Entries)
            {
                string key = top.Key.Text;
                if (!TopKeys.Contains(key))
                {
                    diag.Error(top.Key.Pos, Codes.UnknownTop, "unknown top-level key '" + key + "'. Allowed: " + string.Join(", ", TopKeys));
                    continue;
                }
                if (!seen.Add(key))
                {
                    diag.Error(top.Key.Pos, Codes.DupAttr, "duplicate top-level key '" + key + "'");
                    continue;
                }
                switch (key)
                {
                    case "message":
                        message.Name = ReadIdent(top.Value, top.Key.Pos, diag);
                        message.Pos = top.Key.Pos;
                        break;
                    case "version":
                        message.Version = ReadInt(top.Value, top.Key.Pos, 1, 65535, diag);
                        break;
                    case "min_version":
                        message.MinVersion = ReadInt(top.Value, top.Key.Pos, 1, 65535, diag);
                        break;
                    case "namespace":
                        message.Namespace = ReadNamespace(top.Value, top.Key.Pos, diag);
                        break;
                    case "allow_extension":
                        message.AllowExtension = ReadBool(top.Value, top.Key.Pos, diag);
                        break;
                    case "desc":
                        message.Desc = ReadText(top.Value, top.Key.Pos, diag);
                        break;
                    case "fields":
                        ReadFields(top, message, diag);
                        break;
                }
            }
            if (message.Name == null)
                diag.Error(message.Pos, Codes.MissingTop, "missing 'message:'");
            if (!seen.Contains("version"))
                diag.Error(message.Pos, Codes.MissingTop, "missing 'version:'");
            if (!seen.Contains("namespace"))
                diag.Error(message.Pos, Codes.MissingTop, "missing 'namespace:' (for example 'namespace: app.proto')");
            if (!seen.Contains("fields"))
                diag.Error(message.Pos, Codes.MissingTop, "missing 'fields:'");
            if (!seen.Contains("min_version"))
                message.MinVersion = message.Version;
            string stem = Path.GetFileNameWithoutExtension(message.FilePath);
            if (message.Name != null && message.Name != stem)
                diag.Error(message.Pos, Codes.FileName, "message name '" + message.Name + "' does not match file name '" + stem + "'. One message per file, file name = message name");
        }

        private static List<YItem> Items(YTop top, Diag diag)
        {
            List<YItem> items = top.Value as List<YItem>;
            if (items == null)
                diag.Error(top.Key.Pos, Codes.Syntax, "expected '- {...}' items on the lines after '" + top.Key.Text + ":'");
            return items ?? new List<YItem>();
        }

        private static void ReadFields(YTop top, MessageDef message, Diag diag)
        {
            foreach (YItem item in Items(top, diag))
            {
                FieldDef field = new FieldDef { Pos = item.Pos };
                field.Doc.AddRange(item.Doc);
                HashSet<string> seen = new HashSet<string>();
                foreach (KeyValuePair<YScalar, object> entry in item.Map.Entries)
                {
                    string attr = entry.Key.Text;
                    SourcePos pos = entry.Key.Pos;
                    if (!FieldAttrs.Contains(attr))
                    {
                        diag.Error(pos, Codes.UnknownAttr, "unknown field attribute '" + attr + "'. Allowed: " + string.Join(", ", FieldAttrs));
                        continue;
                    }
                    if (!seen.Add(attr))
                    {
                        diag.Error(pos, Codes.DupAttr, "duplicate attribute '" + attr + "'");
                        continue;
                    }
                    switch (attr)
                    {
                        case "key":
                            field.Key = ReadIdent(entry.Value, pos, diag);
                            break;
                        case "type":
                            field.Type = ParseType(ReadText(entry.Value, pos, diag), pos, diag);
                            break;
                        case "required":
                            field.Required = ReadBool(entry.Value, pos, diag);
                            break;
                        case "added":
                            field.Added = ReadInt(entry.Value, pos, 1, 65535, diag);
                            break;
                        case "removed":
                            field.Removed = ReadInt(entry.Value, pos, 2, 65535, diag);
                            break;
                        case "default":
                            YScalar scalar = entry.Value as YScalar;
                            if (scalar == null)
                            {
                                diag.Error(pos, Codes.BadDefault, "default must be a single scalar");
                                break;
                            }
                            field.DefaultText = scalar.Text;
                            field.DefaultQuoted = scalar.Quoted;
                            break;
                        case "desc":
                            field.Desc = ReadText(entry.Value, pos, diag);
                            break;
                    }
                }
                if (field.Key == null || field.Type == null)
                {
                    diag.Error(item.Pos, Codes.MissingTop, "a field needs key and type");
                    continue;
                }
                field.Index = message.Fields.Count;
                message.Fields.Add(field);
            }
        }

        // Dotted identifiers: "sched.proto" -> C++ sched::proto, C# sched.proto. Keywords are checked by Linker.
        private static string ReadNamespace(object value, SourcePos pos, Diag diag)
        {
            string text = ReadText(value, pos, diag);
            if (text == null)
                return null;
            foreach (string part in text.Split('.'))
            {
                if (!Ident.IsMatch(part))
                {
                    diag.Error(pos, Codes.BadNamespace, "bad namespace '" + text + "'. Use dotted identifiers, for example 'app.proto'");
                    return null;
                }
            }
            return text;
        }

        // Type syntax: bool | int | int64 | uint | uint64 | double | string | string(N) | bytes | list<T> | message name
        public static TypeRef ParseType(string text, SourcePos pos, Diag diag)
        {
            if (text == null)
                return null;
            switch (text)
            {
                case "bool":
                    return new TypeRef { Kind = TypeKind.Bool };
                case "int":
                    return new TypeRef { Kind = TypeKind.Int };
                case "int64":
                    return new TypeRef { Kind = TypeKind.Int64 };
                case "uint":
                    return new TypeRef { Kind = TypeKind.UInt };
                case "uint64":
                    return new TypeRef { Kind = TypeKind.UInt64 };
                case "double":
                    return new TypeRef { Kind = TypeKind.Double };
                case "string":
                    return new TypeRef { Kind = TypeKind.String };
                case "bytes":
                    return new TypeRef { Kind = TypeKind.Bytes };
            }
            Match fixedMatch = FixedStringType.Match(text);
            if (fixedMatch.Success)
            {
                int length;
                if (!int.TryParse(fixedMatch.Groups[1].Value, out length) || length < 1 || length > MaxFixedLength)
                {
                    diag.Error(pos, Codes.BadLength, "N of string(N) must be 1.." + MaxFixedLength + ": '" + text + "'");
                    return null;
                }
                return new TypeRef { Kind = TypeKind.FixedString, FixedLength = length };
            }
            Match listMatch = ListType.Match(text);
            if (listMatch.Success)
            {
                TypeRef element = ParseType(listMatch.Groups[1].Value.Trim(), pos, diag);
                if (element == null)
                    return null;
                return new TypeRef { Kind = TypeKind.List, Element = element };
            }
            if (Ident.IsMatch(text))
                return new TypeRef { Kind = TypeKind.Message, MessageName = text };
            diag.Error(pos, Codes.BadType, "cannot parse type '" + text + "'. Allowed: bool, int, int64, uint, uint64, double, string, string(N), bytes, list<T>, message name");
            return null;
        }

        private static string ReadText(object value, SourcePos pos, Diag diag)
        {
            YScalar scalar = value as YScalar;
            if (scalar == null)
            {
                diag.Error(pos, Codes.BadValue, "scalar value expected");
                return null;
            }
            return scalar.Text;
        }

        private static string ReadIdent(object value, SourcePos pos, Diag diag)
        {
            string text = ReadText(value, pos, diag);
            if (text == null)
                return null;
            if (!Ident.IsMatch(text))
            {
                diag.Error(pos, Codes.BadValue, "bad name '" + text + "'. Use letters, digits and '_', not starting with a digit");
                return null;
            }
            return text;
        }

        private static bool ReadBool(object value, SourcePos pos, Diag diag)
        {
            YScalar scalar = value as YScalar;
            if (scalar != null && !scalar.Quoted && (scalar.Text == "true" || scalar.Text == "false"))
                return scalar.Text == "true";
            diag.Error(pos, Codes.BadValue, "expected true or false");
            return false;
        }

        private static int ReadInt(object value, SourcePos pos, int min, int max, Diag diag)
        {
            YScalar scalar = value as YScalar;
            int number;
            if (scalar != null && !scalar.Quoted && IntText.IsMatch(scalar.Text) && int.TryParse(scalar.Text, out number) && number >= min && number <= max)
                return number;
            diag.Error(pos, Codes.BadValue, "expected an integer " + min + ".." + max);
            return min;
        }
    }
}
