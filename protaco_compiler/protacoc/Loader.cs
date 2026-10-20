// Loader.cs. YDoc -> MessageDef or EnumDef. Checks key and value formats, file naming and per-field rules.
// Cross-definition rules (references, cycles, layout) belong to Linker.
// File kinds: <Name>.yaml declares one message (message:), <Name>.enum.yaml one enum (enum:), <Name>.const.yaml one const set (const:).
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
        public const string EnumSuffix = ".enum.yaml";
        public const string ConstSuffix = ".const.yaml";

        private static readonly Regex Ident = new Regex("^[A-Za-z_][A-Za-z0-9_]*$");
        private static readonly Regex IntText = new Regex("^-?(0|[1-9][0-9]*)$");
        private static readonly Regex FixedStringType = new Regex("^string\\(([0-9]+)\\)$");
        private static readonly Regex ListType = new Regex("^list<(.+)>$");
        private static readonly Regex MapType = new Regex("^map<(.+)>$");
        private static readonly Regex ArrayType = new Regex("^(.+)\\[([^\\[\\]]*)\\]$");
        private static readonly Regex QualifiedName = new Regex("^[A-Za-z_][A-Za-z0-9_]*\\.[A-Za-z_][A-Za-z0-9_]*$");

        private static readonly HashSet<string> MessageKeys = new HashSet<string> { "message", "version", "min_version", "namespace", "allow_extension", "auto_envelope", "desc", "fields" };
        private static readonly HashSet<string> EnumKeys = new HashSet<string> { "enum", "version", "min_version", "namespace", "desc", "values" };
        private static readonly HashSet<string> FieldAttrs = new HashSet<string> { "key", "type", "required", "default", "added", "removed", "desc" };
        private static readonly HashSet<string> ValueAttrs = new HashSet<string> { "key", "value", "added", "removed", "desc" };
        private static readonly HashSet<string> ConstKeys = new HashSet<string> { "const", "namespace", "desc", "values" };
        private static readonly HashSet<string> ConstValueAttrs = new HashSet<string> { "key", "value", "desc" };

        public const int MaxFixedLength = 4096;

        // Integer text the same in C++ and C#: no sign '+', no leading zero (C++ reads 010 as octal 8).
        public static bool IsIntText(string text)
        {
            return IntText.IsMatch(text);
        }

        public static bool IsEnumFile(string path)
        {
            return path.EndsWith(EnumSuffix, StringComparison.OrdinalIgnoreCase);
        }

        public static bool IsConstFile(string path)
        {
            return path.EndsWith(ConstSuffix, StringComparison.OrdinalIgnoreCase);
        }

        // Backup copy such as Order.bak.yaml or Order.bak2.yaml: not read. A definition name has no '.', so no valid file matches.
        public static bool IsBackupFile(string path)
        {
            return Path.GetFileName(path).IndexOf(".bak", StringComparison.OrdinalIgnoreCase) >= 0;
        }

        // File name without .yaml, .enum.yaml or .const.yaml: the name the file must declare.
        public static string Stem(string path)
        {
            string name = Path.GetFileName(path);
            if (IsEnumFile(path))
                return name.Substring(0, name.Length - EnumSuffix.Length);
            if (IsConstFile(path))
                return name.Substring(0, name.Length - ConstSuffix.Length);
            return Path.GetFileNameWithoutExtension(name);
        }

        // Kind word of a definition (message, enum, const) and the file name it belongs in.
        public static string KindOf(SchemaDef def)
        {
            return def is EnumDef ? "enum" : def is ConstDef ? "const" : "message";
        }

        public static string FileNameOf(SchemaDef def)
        {
            return def.Name + (def is EnumDef ? EnumSuffix : def is ConstDef ? ConstSuffix : ".yaml");
        }

        public static SchemaDef Load(string path, Diag diag)
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
            SourcePos filePos = new SourcePos(path, 1, 1);
            if (!CheckFileKind(path, doc, "enum", EnumSuffix, IsEnumFile(path), filePos, diag)
                || !CheckFileKind(path, doc, "const", ConstSuffix, IsConstFile(path), filePos, diag))
                return null;
            SchemaDef def = IsEnumFile(path) ? new EnumDef() : IsConstFile(path) ? (SchemaDef)new ConstDef() : new MessageDef();
            def.FilePath = path;
            def.FileName = Path.GetFileName(path);
            // Without BOM and with LF line ends, so every checkout (autocrlf or not) writes the same header.
            def.Sha1 = Sha1Hex(new UTF8Encoding(false).GetBytes(text.TrimStart('﻿').Replace("\r\n", "\n")));
            def.Pos = filePos;
            def.Doc.AddRange(doc.HeaderDoc);
            ReadTop(doc, def, diag);
            return diag.ErrorCount == before ? def : null;
        }

        // A <Name><suffix> file declares exactly that kind (top key word), and that kind lives only in such files.
        private static bool CheckFileKind(string path, YDoc doc, string word, string suffix, bool kindFile, SourcePos filePos, Diag diag)
        {
            bool declares = doc.Entries.Exists(t => t.Key.Text == word);
            if (kindFile && !declares)
            {
                diag.Error(filePos, Codes.FileKind, "a " + suffix + " file declares one " + word + " ('" + word + ": " + Stem(path) + "')");
                return false;
            }
            if (!kindFile && declares)
            {
                diag.Error(filePos, Codes.FileKind, word + "s are declared in <Name>" + suffix + " files; rename this file to " + Stem(path) + suffix);
                return false;
            }
            return true;
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

        private static void ReadTop(YDoc doc, SchemaDef def, Diag diag)
        {
            MessageDef message = def as MessageDef;
            EnumDef enumDef = def as EnumDef;
            ConstDef constDef = def as ConstDef;
            string kind = KindOf(def);
            HashSet<string> allowed = message != null ? MessageKeys : enumDef != null ? EnumKeys : ConstKeys;
            HashSet<string> seen = new HashSet<string>();
            foreach (YTop top in doc.Entries)
            {
                string key = top.Key.Text;
                if (!allowed.Contains(key))
                {
                    diag.Error(top.Key.Pos, Codes.UnknownTop, "unknown top-level key '" + key + "'. Allowed: " + string.Join(", ", allowed));
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
                    case "enum":
                    case "const":
                        def.Name = ReadIdent(top.Value, top.Key.Pos, diag);
                        def.Pos = top.Key.Pos;
                        break;
                    case "version":
                        def.Version = ReadInt(top.Value, top.Key.Pos, 1, 65535, diag);
                        break;
                    case "min_version":
                        def.MinVersion = ReadInt(top.Value, top.Key.Pos, 1, 65535, diag);
                        break;
                    case "namespace":
                        def.Namespace = ReadNamespace(top.Value, top.Key.Pos, diag);
                        break;
                    case "allow_extension":
                        message.AllowExtension = ReadBool(top.Value, top.Key.Pos, diag);
                        break;
                    case "auto_envelope":
                        message.AutoEnvelope = ReadBool(top.Value, top.Key.Pos, diag);
                        break;
                    case "desc":
                        def.Desc = ReadText(top.Value, top.Key.Pos, diag);
                        break;
                    case "fields":
                        ReadFields(top, message, diag);
                        break;
                    case "values":
                        if (enumDef != null)
                            ReadValues(top, enumDef, diag);
                        else
                            ReadConstValues(top, constDef, diag);
                        break;
                }
            }
            if (def.Name == null)
                diag.Error(def.Pos, Codes.MissingTop, "missing '" + kind + ":'");
            if (constDef == null && !seen.Contains("version"))
                diag.Error(def.Pos, Codes.MissingTop, "missing 'version:'");
            if (!seen.Contains("namespace"))
                diag.Error(def.Pos, Codes.MissingTop, "missing 'namespace:' (for example 'namespace: app.proto')");
            if (!seen.Contains(message != null ? "fields" : "values"))
                diag.Error(def.Pos, Codes.MissingTop, "missing '" + (message != null ? "fields" : "values") + ":'");
            if (!seen.Contains("min_version"))
                def.MinVersion = def.Version;
            string stem = Stem(def.FilePath);
            if (def.Name != null && def.Name != stem)
                diag.Error(def.Pos, Codes.FileName, kind + " name '" + def.Name + "' does not match file name '" + def.FileName + "'. One " + kind
                    + " per file: " + FileNameOf(def));
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

        private static void ReadValues(YTop top, EnumDef enumDef, Diag diag)
        {
            foreach (YItem item in Items(top, diag))
            {
                EnumValueDef value = new EnumValueDef { Pos = item.Pos };
                value.Doc.AddRange(item.Doc);
                HashSet<string> seen = new HashSet<string>();
                foreach (KeyValuePair<YScalar, object> entry in item.Map.Entries)
                {
                    string attr = entry.Key.Text;
                    SourcePos pos = entry.Key.Pos;
                    if (!ValueAttrs.Contains(attr))
                    {
                        diag.Error(pos, Codes.UnknownAttr, "unknown enum value attribute '" + attr + "'. Allowed: " + string.Join(", ", ValueAttrs));
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
                            value.Key = ReadIdent(entry.Value, pos, diag);
                            break;
                        case "value":
                            value.Number = ReadInt(entry.Value, pos, int.MinValue + 1, int.MaxValue, diag);
                            break;
                        case "added":
                            value.Added = ReadInt(entry.Value, pos, 1, 65535, diag);
                            break;
                        case "removed":
                            value.Removed = ReadInt(entry.Value, pos, 2, 65535, diag);
                            break;
                        case "desc":
                            value.Desc = ReadText(entry.Value, pos, diag);
                            break;
                    }
                }
                if (value.Key == null || !seen.Contains("value"))
                {
                    diag.Error(item.Pos, Codes.MissingTop, "an enum value needs key and value");
                    continue;
                }
                enumDef.Values.Add(value);
            }
        }

        private static void ReadConstValues(YTop top, ConstDef constDef, Diag diag)
        {
            foreach (YItem item in Items(top, diag))
            {
                ConstValueDef value = new ConstValueDef { Pos = item.Pos };
                value.Doc.AddRange(item.Doc);
                HashSet<string> seen = new HashSet<string>();
                foreach (KeyValuePair<YScalar, object> entry in item.Map.Entries)
                {
                    string attr = entry.Key.Text;
                    SourcePos pos = entry.Key.Pos;
                    if (!ConstValueAttrs.Contains(attr))
                    {
                        diag.Error(pos, Codes.UnknownAttr, "unknown const value attribute '" + attr + "'. Allowed: " + string.Join(", ", ConstValueAttrs));
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
                            value.Key = ReadIdent(entry.Value, pos, diag);
                            break;
                        case "value":
                            value.Number = ReadInt(entry.Value, pos, int.MinValue + 1, int.MaxValue, diag);
                            break;
                        case "desc":
                            value.Desc = ReadText(entry.Value, pos, diag);
                            break;
                    }
                }
                if (value.Key == null || !seen.Contains("value"))
                {
                    diag.Error(item.Pos, Codes.MissingTop, "a const value needs key and value");
                    continue;
                }
                constDef.Values.Add(value);
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

        // Type syntax: bool | int | int64 | uint | uint64 | double | string | string(N) | bytes | list<T> | map<K, V> | T[N]
        // | message or enum name. N of T[N]: a number, Const.Key or Enum.Value (resolved by the linker).
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
            Match mapMatch = MapType.Match(text);
            if (mapMatch.Success)
            {
                // Keys are single words, so the first comma ends the key.
                string inner = mapMatch.Groups[1].Value;
                int comma = inner.IndexOf(',');
                if (comma < 0)
                {
                    diag.Error(pos, Codes.BadType, "map needs a key and a value type: '" + text + "', for example map<string, int>");
                    return null;
                }
                TypeRef key = ParseType(inner.Substring(0, comma).Trim(), pos, diag);
                TypeRef value = ParseType(inner.Substring(comma + 1).Trim(), pos, diag);
                if (key == null || value == null)
                    return null;
                return new TypeRef { Kind = TypeKind.Map, Key = key, Element = value };
            }
            Match arrayMatch = ArrayType.Match(text);
            if (arrayMatch.Success)
            {
                string length = arrayMatch.Groups[2].Value.Trim();
                if (!IntText.IsMatch(length) && !QualifiedName.IsMatch(length))
                {
                    diag.Error(pos, Codes.BadType, "array length must be a number, Const.Key or Enum.Value: '" + text + "'");
                    return null;
                }
                TypeRef element = ParseType(arrayMatch.Groups[1].Value.Trim(), pos, diag);
                if (element == null)
                    return null;
                return new TypeRef { Kind = TypeKind.Array, Element = element, LengthText = length };
            }
            // A message or an enum: the linker decides which.
            if (Ident.IsMatch(text))
                return new TypeRef { Kind = TypeKind.Message, TypeName = text };
            diag.Error(pos, Codes.BadType, "cannot parse type '" + text + "'. Allowed: bool, int, int64, uint, uint64, double, string, string(N), bytes, list<T>, map<K, V>, T[N], message or enum name");
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
