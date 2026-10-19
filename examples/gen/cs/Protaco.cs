// Protaco.cs. C# runtime for protacoc generated code (*.g.cs). .NET Framework 4.6.1, C# 7.3.
// Produces the same bytes as the C++ runtime (Protaco.hpp). Format v4: README.md "Serialization format".
// Strings are UTF-8 on every platform. Try* calls report failure through the return value and an error string.
// protacoc.exe writes this file next to the generated code, so generator and runtime versions always match.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace Protaco
{
    /// <summary>Value kinds. The numbers are part of the serialized data. Never change them.</summary>
    public enum TacoType : byte
    {
        Bool = 1,
        Int = 2,
        Int64 = 3,
        Double = 4,
        String = 5,
        Message = 6,
        List = 7,
        Bytes = 8,
        UInt = 9,
        UInt64 = 10,
    }

    /// <summary>Message envelope: request id, correlation id, time (us).</summary>
    public struct Envelope
    {
        public uint Id;
        public uint Corr;
        public long Ts;
    }

    /// <summary>Frame header.</summary>
    public sealed class Header
    {
        public string Name;
        public ushort Version;      // sender's version of the message
        public ushort MinVersion;   // oldest version the sender supports; the frame has a fingerprint per version
        public uint SchemaHash;     // fingerprint of Version (the sender's MsgSchemaHash)
        public Envelope Env;
        public uint BodySize;       // whole frame = header (28 + name + 4 per fingerprint) + BodySize
    }

    /// <summary>Decode error. Path looks like "moves[2].from".</summary>
    public sealed class ProtacoException : Exception
    {
        public ProtacoException(string path, string what)
            : base(string.IsNullOrEmpty(path) ? what : path + ": " + what)
        {
            Path = path ?? "";
            What = what;
        }

        public string Path { get; }

        public string What { get; }

        internal ProtacoException Prepend(string segment)
        {
            string path;
            if (Path.Length == 0)
                path = segment;
            else if (Path[0] == '[')
                path = segment + Path;
            else
                path = segment + "." + Path;
            return new ProtacoException(path, What);
        }
    }

    /// <summary>Implemented by generated messages. Runtime only, so generated code hides it with explicit implementation.</summary>
    public interface IMessage
    {
        string Name { get; }

        ushort Version { get; }

        ushort MinVersion { get; }

        /// <summary>Fingerprints of versions MinVersion..Version.</summary>
        uint[] SchemaHashes { get; }

        bool AllowExtension { get; }

        int FixedSize { get; }

        Ext Ext { get; }

        void WriteText(TextBuilder builder);

        void WriteFixed(FixedWriter writer);

        void ReadFixed(FixedReader reader);

        void WriteVar(VarWriter writer);

        void ReadVar(VarReader reader);
    }

    /// <summary>Serialize and deserialize entry points.</summary>
    public static class Codec
    {
        public const int FrameVersion = 4;
        public const int MaxDepth = 8;
        public const int MaxSchemaVersions = 255;
        public const int MaxFields = 1024;
        // The format allows 4 GiB - 1; a .NET byte array holds at most this many bytes.
        public const int MaxFrameBytes = 0x7FFFFFC7;

        // String bytes are UTF-8 on every platform. Invalid UTF-8 from a peer decodes to U+FFFD.
        internal static readonly Encoding Text = new UTF8Encoding(false);

        public static byte[] Serialize(IMessage message, Envelope env)
        {
            using (MemoryStream stream = new MemoryStream())
            using (BinaryWriter writer = new BinaryWriter(stream))
            {
                byte[] name = Text.GetBytes(message.Name);
                writer.Write((byte)'P');
                writer.Write((byte)'C');
                writer.Write((byte)FrameVersion);
                writer.Write((byte)name.Length);
                writer.Write(name);
                writer.Write(message.Version);
                writer.Write(message.MinVersion);
                writer.Write(env.Id);
                writer.Write(env.Corr);
                writer.Write(env.Ts);
                long bodySizeAt = stream.Position;
                writer.Write(0u);
                foreach (uint hash in message.SchemaHashes)
                    writer.Write(hash);
                long bodyStart = stream.Position;
                WriteBody(message, writer);
                long end = stream.Position;
                stream.Position = bodySizeAt;
                writer.Write((uint)(end - bodyStart));
                stream.Position = end;
                writer.Flush();
                return stream.ToArray();
            }
        }

        /// <summary>Bytes to object. Throws ProtacoException on failure.</summary>
        public static T ToObject<T>(byte[] bytes) where T : IMessage, new()
        {
            return ToObject<T>(bytes, out Envelope env);
        }

        public static T ToObject<T>(byte[] bytes, out Envelope env) where T : IMessage, new()
        {
            if (bytes.Length > MaxFrameBytes)
                throw new ProtacoException("", "frame too large");
            Header header = ReadHeader(bytes);
            T message = new T();
            string name = message.Name;
            ushort mine = message.Version;
            if (header.Name != name)
                throw new ProtacoException("", "message name mismatch: expected " + name + ", got " + header.Name);
            if (header.Version < mine)
            {
                string what = "message version too old: " + name + " reads version " + mine + " or newer, got " + header.Version;
                if (header.Version >= message.MinVersion)
                    what += ". Use the v" + header.Version + " type";
                throw new ProtacoException("", what);
            }
            if (header.MinVersion > mine)
                throw new ProtacoException("", "message version too new: " + name + " reads version " + mine + ", sender supports " + header.MinVersion + ".." + header.Version);
            int nameLength = bytes[3];
            uint hash = BitConverter.ToUInt32(bytes, 28 + nameLength + 4 * (mine - header.MinVersion));
            uint expected = message.SchemaHashes[message.SchemaHashes.Length - 1];
            if (hash != expected)
                throw new ProtacoException("", "schema mismatch: " + name + " version " + mine + " expected " + Hex32(expected) + ", got " + Hex32(hash));
            env = header.Env;
            int headerSize = 28 + nameLength + 4 * (header.Version - header.MinVersion + 1);
            long left = bytes.Length - headerSize;
            if (left != header.BodySize)
                throw new ProtacoException("", left < header.BodySize ? "frame truncated" : "trailing bytes after frame");
            bool newer = header.Version > mine;
            try
            {
                ReadBody(message, bytes, headerSize, (int)header.BodySize, 0, newer);
            }
            catch (ProtacoException e) when (newer)
            {
                throw new ProtacoException(e.Path, e.What + " (frame version " + header.Version + " read as " + mine + ")");
            }
            return message;
        }

        internal static string Hex32(uint value)
        {
            return "0x" + value.ToString("X8");
        }

        // yaml type word of a TacoType, for error text.
        internal static string TypeName(TacoType type)
        {
            switch (type)
            {
                case TacoType.Bool:
                    return "bool";
                case TacoType.Int:
                    return "int";
                case TacoType.Int64:
                    return "int64";
                case TacoType.UInt:
                    return "uint";
                case TacoType.UInt64:
                    return "uint64";
                case TacoType.Double:
                    return "double";
                case TacoType.String:
                    return "string";
                case TacoType.Message:
                    return "message";
                case TacoType.List:
                    return "list";
                case TacoType.Bytes:
                    return "bytes";
                default:
                    return "unknown";
            }
        }

        /// <summary>Bytes to object without exceptions. On failure message is default and error is "path: reason".</summary>
        public static bool TryToObject<T>(byte[] bytes, out T message, out string error) where T : IMessage, new()
        {
            try
            {
                message = ToObject<T>(bytes);
                error = null;
                return true;
            }
            catch (ProtacoException e)
            {
                message = default(T);
                error = e.Message;
                return false;
            }
        }

        public static bool TryToObject<T>(byte[] bytes, out T message) where T : IMessage, new()
        {
            string error;
            return TryToObject(bytes, out message, out error);
        }

        /// <summary>Frame header only, without the body (for dispatch).</summary>
        public static bool PeekHeader(byte[] bytes, out Header header, out string error)
        {
            try
            {
                header = ReadHeader(bytes);
                error = null;
                return true;
            }
            catch (ProtacoException e)
            {
                header = null;
                error = e.Message;
                return false;
            }
        }

        public static bool PeekHeader(byte[] bytes, out Header header)
        {
            string error;
            return PeekHeader(bytes, out header, out error);
        }

        /// <summary>Fixed-length string setters call this. Same bytes limit as C++ (UTF-8 byte count).</summary>
        public static void CheckChars(string value, int capacity, string key)
        {
            int length = Text.GetByteCount(value ?? "");
            if (length > capacity)
                throw new ArgumentException(key + ": " + length + " bytes, longer than string(" + capacity + ")");
        }

        private static Header ReadHeader(byte[] bytes)
        {
            if (bytes.Length < 28)
                throw new ProtacoException("", "frame too short");
            if (bytes[0] != 'P' || bytes[1] != 'C')
                throw new ProtacoException("", "bad magic");
            if (bytes[2] != FrameVersion)
                throw new ProtacoException("", "unknown frame version " + bytes[2] + ", expected " + FrameVersion);
            int nameLength = bytes[3];
            if (bytes.Length < 28 + nameLength)
                throw new ProtacoException("", "frame too short");
            int p = 4 + nameLength;
            ushort version = BitConverter.ToUInt16(bytes, p);
            ushort minVersion = BitConverter.ToUInt16(bytes, p + 2);
            if (minVersion == 0 || minVersion > version || version - minVersion >= MaxSchemaVersions)
                throw new ProtacoException("", "bad version range " + minVersion + ".." + version);
            int hashCount = version - minVersion + 1;
            if (bytes.Length < 28 + nameLength + 4 * hashCount)
                throw new ProtacoException("", "frame too short");
            Header header = new Header();
            header.Name = Text.GetString(bytes, 4, nameLength);
            header.Version = version;
            header.MinVersion = minVersion;
            header.Env.Id = BitConverter.ToUInt32(bytes, p + 4);
            header.Env.Corr = BitConverter.ToUInt32(bytes, p + 8);
            header.Env.Ts = BitConverter.ToInt64(bytes, p + 12);
            header.BodySize = BitConverter.ToUInt32(bytes, p + 20);
            header.SchemaHash = BitConverter.ToUInt32(bytes, p + 24 + 4 * (hashCount - 1));
            return header;
        }

        // body: FixedLen u32 | fixed block | variable TLV... | ext TLV...
        internal static void WriteBody(IMessage message, BinaryWriter writer)
        {
            byte[] fixedBlock = new byte[message.FixedSize];
            message.WriteFixed(new FixedWriter(fixedBlock));
            writer.Write((uint)fixedBlock.Length);
            writer.Write(fixedBlock);
            VarWriter varWriter = new VarWriter(writer);
            message.WriteVar(varWriter);
            message.Ext.WriteTo(varWriter);
        }

        // newer: the frame is of a newer version than message (verified by fingerprint). message reads its own prefix of it.
        internal static void ReadBody(IMessage message, byte[] src, int offset, int count, int depth, bool newer)
        {
            if (depth > MaxDepth)
                throw new ProtacoException("", "nesting too deep");
            if (count < 4)
                throw new ProtacoException("", "fixed block truncated");
            uint fixedLength = BitConverter.ToUInt32(src, offset);
            if (fixedLength > (uint)(count - 4))
                throw new ProtacoException("", "fixed block truncated");
            // Same version means same fixed block size; a newer version only appends. Also guards nested bodies,
            // which carry no fingerprint of their own.
            uint expected = (uint)message.FixedSize;
            if (newer ? fixedLength < expected : fixedLength != expected)
                throw new ProtacoException("", "fixed block size mismatch: expected " + expected + ", got " + fixedLength);
            message.ReadFixed(new FixedReader(src, offset + 4));
            int varStart = offset + 4 + (int)fixedLength;
            VarReader reader = new VarReader(src, varStart, offset + count - varStart, depth);
            message.ReadVar(reader);
            reader.MoveUnusedTo(message.Ext, message.AllowExtension, newer);
        }
    }

    /// <summary>Fixed block writer (runtime only).</summary>
    public sealed class FixedWriter
    {
        private readonly byte[] block_;

        internal FixedWriter(byte[] block)
        {
            block_ = block;
        }

        public void Bool(int offset, bool value)
        {
            block_[offset] = (byte)(value ? 1 : 0);
        }

        public void Int(int offset, int value)
        {
            Put(offset, BitConverter.GetBytes(value));
        }

        public void Int64(int offset, long value)
        {
            Put(offset, BitConverter.GetBytes(value));
        }

        public void UInt(int offset, uint value)
        {
            Put(offset, BitConverter.GetBytes(value));
        }

        public void UInt64(int offset, ulong value)
        {
            Put(offset, BitConverter.GetBytes(value));
        }

        public void Double(int offset, double value)
        {
            Put(offset, BitConverter.GetBytes(value));
        }

        public void Chars(int offset, int capacity, string value)
        {
            byte[] bytes = Codec.Text.GetBytes(value ?? "");
            Buffer.BlockCopy(bytes, 0, block_, offset, Math.Min(bytes.Length, capacity));
            for (int i = bytes.Length; i < capacity; ++i)
                block_[offset + i] = 0;
        }

        private void Put(int offset, byte[] bytes)
        {
            Buffer.BlockCopy(bytes, 0, block_, offset, bytes.Length);
        }
    }

    /// <summary>Fixed block reader (runtime only).</summary>
    public sealed class FixedReader
    {
        private readonly byte[] src_;
        private readonly int start_;

        internal FixedReader(byte[] src, int start)
        {
            src_ = src;
            start_ = start;
        }

        public bool Bool(int offset)
        {
            return src_[start_ + offset] != 0;
        }

        public int Int(int offset)
        {
            return BitConverter.ToInt32(src_, start_ + offset);
        }

        public long Int64(int offset)
        {
            return BitConverter.ToInt64(src_, start_ + offset);
        }

        public uint UInt(int offset)
        {
            return BitConverter.ToUInt32(src_, start_ + offset);
        }

        public ulong UInt64(int offset)
        {
            return BitConverter.ToUInt64(src_, start_ + offset);
        }

        public double Double(int offset)
        {
            return BitConverter.ToDouble(src_, start_ + offset);
        }

        public string Chars(int offset, int capacity)
        {
            int at = start_ + offset;
            int length = 0;
            while (length < capacity && src_[at + length] != 0)
                ++length;
            return Codec.Text.GetString(src_, at, length);
        }
    }

    /// <summary>Variable block writer (runtime only). TLV: KeyLen u8 | Key | Type u8 | Len u32 | Value</summary>
    public sealed class VarWriter
    {
        private readonly BinaryWriter writer_;

        internal VarWriter(BinaryWriter writer)
        {
            writer_ = writer;
        }

        public void String(string key, string value)
        {
            Tlv(key, TacoType.String, () => writer_.Write(Codec.Text.GetBytes(value ?? "")));
        }

        public void Bytes(string key, byte[] value)
        {
            Tlv(key, TacoType.Bytes, () => writer_.Write(value ?? new byte[0]));
        }

        public void Message(string key, IMessage value)
        {
            Tlv(key, TacoType.Message, () => Codec.WriteBody(value, writer_));
        }

        public void ListInt(string key, List<int> list)
        {
            WriteList(key, TacoType.Int, list, item => writer_.Write(item));
        }

        public void ListInt64(string key, List<long> list)
        {
            WriteList(key, TacoType.Int64, list, item => writer_.Write(item));
        }

        public void ListUInt(string key, List<uint> list)
        {
            WriteList(key, TacoType.UInt, list, item => writer_.Write(item));
        }

        public void ListUInt64(string key, List<ulong> list)
        {
            WriteList(key, TacoType.UInt64, list, item => writer_.Write(item));
        }

        public void ListDouble(string key, List<double> list)
        {
            WriteList(key, TacoType.Double, list, item => writer_.Write(item));
        }

        public void ListString(string key, List<string> list)
        {
            WriteList(key, TacoType.String, list, item => writer_.Write(Codec.Text.GetBytes(item ?? "")));
        }

        public void ListBytes(string key, List<byte[]> list)
        {
            WriteList(key, TacoType.Bytes, list, item => writer_.Write(item ?? new byte[0]));
        }

        public void ListMessage<T>(string key, List<T> list) where T : IMessage
        {
            WriteList(key, TacoType.Message, list, item => Codec.WriteBody(item, writer_));
        }

        internal void Raw(string key, TacoType type, byte[] value)
        {
            Tlv(key, type, () => writer_.Write(value));
        }

        // list: ElemType u8 | Count u32 | (Len u32 | Value) x Count
        private void WriteList<T>(string key, TacoType elemType, List<T> list, Action<T> writeItem)
        {
            Tlv(key, TacoType.List, () =>
            {
                writer_.Write((byte)elemType);
                writer_.Write((uint)list.Count);
                foreach (T item in list)
                    WithLength(() => writeItem(item));
            });
        }

        private void Tlv(string key, TacoType type, Action writeValue)
        {
            byte[] keyBytes = Codec.Text.GetBytes(key);
            writer_.Write((byte)keyBytes.Length);
            writer_.Write(keyBytes);
            writer_.Write((byte)type);
            WithLength(writeValue);
        }

        // Reserves the length slot, writes the value, then the length.
        private void WithLength(Action writeValue)
        {
            Stream stream = writer_.BaseStream;
            long lengthAt = stream.Position;
            writer_.Write(0u);
            long start = stream.Position;
            writeValue();
            long end = stream.Position;
            stream.Position = lengthAt;
            writer_.Write((uint)(end - start));
            stream.Position = end;
        }
    }

    /// <summary>Variable block reader (runtime only). Every read checks the remaining length.</summary>
    public sealed class VarReader
    {
        private sealed class Raw
        {
            public string Key;
            public TacoType Type;
            public int Offset;
            public int Length;
            public bool Used;
        }

        private readonly byte[] src_;
        private readonly int depth_;
        private readonly List<Raw> fields_ = new List<Raw>();

        internal VarReader(byte[] src, int offset, int count, int depth)
        {
            src_ = src;
            depth_ = depth;
            int p = offset;
            int end = offset + count;
            while (p < end)
            {
                int keyLength = src[p];
                if (keyLength == 0 || p + 1 + keyLength > end)
                    throw new ProtacoException("", "field key truncated");
                string key = Codec.Text.GetString(src, p + 1, keyLength);
                p += 1 + keyLength;
                if (p + 5 > end)
                    throw new ProtacoException(key, "field value truncated");
                TacoType type = (TacoType)src[p];
                uint length = BitConverter.ToUInt32(src, p + 1);
                p += 5;
                if (length > (uint)(end - p))
                    throw new ProtacoException(key, "field value truncated");
                if (Find(key) != null)
                    throw new ProtacoException(key, "duplicate key");
                if (fields_.Count >= Codec.MaxFields)
                    throw new ProtacoException("", "too many fields");
                fields_.Add(new Raw { Key = key, Type = type, Offset = p, Length = (int)length });
                p += (int)length;
            }
        }

        public string String(string key, bool required, string fallback)
        {
            Raw raw = Take(key, TacoType.String, required);
            return raw == null ? fallback : Codec.Text.GetString(src_, raw.Offset, raw.Length);
        }

        public byte[] Bytes(string key, bool required, byte[] fallback)
        {
            Raw raw = Take(key, TacoType.Bytes, required);
            return raw == null ? fallback : Copy(raw.Offset, raw.Length);
        }

        public T Message<T>(string key, bool required, T fallback) where T : IMessage, new()
        {
            Raw raw = Take(key, TacoType.Message, required);
            if (raw == null)
                return fallback;
            return Guard(key, () => ReadMessage<T>(raw.Offset, raw.Length));
        }

        public List<int> ListInt(string key, bool required, List<int> fallback)
        {
            return ReadList(key, required, fallback, TacoType.Int, (offset, length) => BitConverter.ToInt32(src_, CheckSize(offset, length, 4)));
        }

        public List<long> ListInt64(string key, bool required, List<long> fallback)
        {
            return ReadList(key, required, fallback, TacoType.Int64, (offset, length) => BitConverter.ToInt64(src_, CheckSize(offset, length, 8)));
        }

        public List<uint> ListUInt(string key, bool required, List<uint> fallback)
        {
            return ReadList(key, required, fallback, TacoType.UInt, (offset, length) => BitConverter.ToUInt32(src_, CheckSize(offset, length, 4)));
        }

        public List<ulong> ListUInt64(string key, bool required, List<ulong> fallback)
        {
            return ReadList(key, required, fallback, TacoType.UInt64, (offset, length) => BitConverter.ToUInt64(src_, CheckSize(offset, length, 8)));
        }

        public List<double> ListDouble(string key, bool required, List<double> fallback)
        {
            return ReadList(key, required, fallback, TacoType.Double, (offset, length) => BitConverter.ToDouble(src_, CheckSize(offset, length, 8)));
        }

        public List<string> ListString(string key, bool required, List<string> fallback)
        {
            return ReadList(key, required, fallback, TacoType.String, (offset, length) => Codec.Text.GetString(src_, offset, length));
        }

        public List<byte[]> ListBytes(string key, bool required, List<byte[]> fallback)
        {
            return ReadList(key, required, fallback, TacoType.Bytes, Copy);
        }

        private byte[] Copy(int offset, int length)
        {
            byte[] value = new byte[length];
            Buffer.BlockCopy(src_, offset, value, 0, length);
            return value;
        }

        public List<T> ListMessage<T>(string key, bool required, List<T> fallback) where T : IMessage, new()
        {
            return ReadList(key, required, fallback, TacoType.Message, (offset, length) => ReadMessage<T>(offset, length));
        }

        // Keys not in the schema are kept in ext. allow_extension false rejects them, except fields of a newer version,
        // which are dropped.
        internal void MoveUnusedTo(Ext ext, bool allowExtension, bool newer)
        {
            foreach (Raw raw in fields_)
            {
                if (raw.Used)
                    continue;
                if (!allowExtension)
                {
                    if (newer)
                        continue;
                    throw new ProtacoException(raw.Key, "unknown key (allow_extension false)");
                }
                byte[] value = new byte[raw.Length];
                Buffer.BlockCopy(src_, raw.Offset, value, 0, raw.Length);
                ext.AddRaw(raw.Key, raw.Type, value);
            }
        }

        private T ReadMessage<T>(int offset, int length) where T : IMessage, new()
        {
            T message = new T();
            Codec.ReadBody(message, src_, offset, length, depth_ + 1, false);
            return message;
        }

        private static int CheckSize(int offset, int length, int expected)
        {
            if (length != expected)
                throw new ProtacoException("", "scalar size mismatch");
            return offset;
        }

        private List<T> ReadList<T>(string key, bool required, List<T> fallback, TacoType elemType, Func<int, int, T> readItem)
        {
            Raw raw = Take(key, TacoType.List, required);
            if (raw == null)
                return fallback;
            return Guard(key, () =>
            {
                int p = raw.Offset;
                int end = raw.Offset + raw.Length;
                if (raw.Length < 5)
                    throw new ProtacoException("", "list header truncated");
                if ((TacoType)src_[p] != elemType)
                    throw new ProtacoException("", "list element type mismatch");
                uint count = BitConverter.ToUInt32(src_, p + 1);
                p += 5;
                if (count > (uint)((end - p) / 4))
                    throw new ProtacoException("", "list count too large");
                List<T> list = new List<T>((int)count);
                for (int i = 0; i < (int)count; ++i)
                {
                    if (p + 4 > end)
                        throw new ProtacoException("[" + i + "]", "list element truncated");
                    uint length = BitConverter.ToUInt32(src_, p);
                    p += 4;
                    if (length > (uint)(end - p))
                        throw new ProtacoException("[" + i + "]", "list element truncated");
                    int at = p;
                    int index = i;
                    list.Add(Guard("[" + index + "]", () => readItem(at, (int)length)));
                    p += (int)length;
                }
                if (p != end)
                    throw new ProtacoException("", "trailing bytes in list");
                return list;
            });
        }

        private static T Guard<T>(string segment, Func<T> read)
        {
            try
            {
                return read();
            }
            catch (ProtacoException e)
            {
                throw e.Prepend(segment);
            }
        }

        private Raw Find(string key)
        {
            foreach (Raw raw in fields_)
            {
                if (raw.Key == key)
                    return raw;
            }
            return null;
        }

        private Raw Take(string key, TacoType type, bool required)
        {
            Raw raw = Find(key);
            if (raw == null)
            {
                if (required)
                    throw new ProtacoException(key, "missing required");
                return null;
            }
            raw.Used = true;
            if (raw.Type != type)
                throw new ProtacoException(key, "type mismatch: expected " + Codec.TypeName(type) + ", got " + Codec.TypeName(raw.Type));
            return raw;
        }
    }

    /// <summary>
    /// Extra key-value data not in the yaml. Receivers keep unknown keys here.
    /// Schema keys and keys starting with '_' are rejected (Set returns false).
    /// </summary>
    public sealed class Ext
    {
        private sealed class Entry
        {
            public string Key;
            public TacoType Type;
            public byte[] Value;
        }

        private readonly Func<string, bool> isSchemaKey_;
        private readonly List<Entry> entries_ = new List<Entry>();

        public Ext(Func<string, bool> isSchemaKey)
        {
            isSchemaKey_ = isSchemaKey;
        }

        public int Count => entries_.Count;

        public IEnumerable<string> Keys
        {
            get
            {
                foreach (Entry entry in entries_)
                    yield return entry.Key;
            }
        }

        /// <summary>Value types: bool, int, long, uint, ulong, double, string, byte[]. Returns false if the key is rejected
        /// (empty, longer than 255 bytes, starting with '_', or a field of the message) or the type is not supported.</summary>
        public bool SetValue<T>(string key, T value)
        {
            string error;
            return SetValue(key, value, out error);
        }

        public bool SetValue<T>(string key, T value, out string error)
        {
            TacoType type;
            byte[] bytes;
            if (!Encode(value, out type, out bytes, out error))
                return false;
            return Put(key, type, bytes, out error);
        }

        /// <summary>Returns false (value is default) if the key is missing or holds another type.</summary>
        public bool TryGetValue<T>(string key, out T value)
        {
            string error;
            return TryGetValue(key, out value, out error);
        }

        public bool TryGetValue<T>(string key, out T value, out string error)
        {
            value = default(T);
            TacoType wanted;
            int size;
            if (!KindOf(typeof(T), out wanted, out size))
            {
                error = "unsupported ext value type " + typeof(T).Name;
                return false;
            }
            int index = IndexOf(key);
            if (index < 0)
            {
                error = "ext key not found: " + key;
                return false;
            }
            Entry entry = entries_[index];
            if (entry.Type != wanted)
            {
                error = "ext key " + key + " holds " + Codec.TypeName(entry.Type) + ", not " + Codec.TypeName(wanted);
                return false;
            }
            if (size >= 0 && entry.Value.Length != size)
            {
                error = "ext key " + key + " has a bad " + Codec.TypeName(wanted) + " value";
                return false;
            }
            value = (T)Decode(wanted, entry.Value);
            error = null;
            return true;
        }

        private static bool KindOf(Type type, out TacoType kind, out int size)
        {
            size = -1;
            kind = TacoType.Bool;
            if (type == typeof(bool))
                size = 1;
            else if (type == typeof(int))
            {
                kind = TacoType.Int;
                size = 4;
            }
            else if (type == typeof(long))
            {
                kind = TacoType.Int64;
                size = 8;
            }
            else if (type == typeof(uint))
            {
                kind = TacoType.UInt;
                size = 4;
            }
            else if (type == typeof(ulong))
            {
                kind = TacoType.UInt64;
                size = 8;
            }
            else if (type == typeof(double))
            {
                kind = TacoType.Double;
                size = 8;
            }
            else if (type == typeof(string))
                kind = TacoType.String;
            else if (type == typeof(byte[]))
                kind = TacoType.Bytes;
            else
                return false;
            return true;
        }

        private static bool Encode<T>(T value, out TacoType type, out byte[] bytes, out string error)
        {
            int size;
            bytes = null;
            if (!KindOf(typeof(T), out type, out size))
            {
                error = "unsupported ext value type " + typeof(T).Name;
                return false;
            }
            object boxed = value;
            switch (type)
            {
                case TacoType.Bool:
                    bytes = new[] { (byte)((bool)boxed ? 1 : 0) };
                    break;
                case TacoType.Int:
                    bytes = BitConverter.GetBytes((int)boxed);
                    break;
                case TacoType.Int64:
                    bytes = BitConverter.GetBytes((long)boxed);
                    break;
                case TacoType.UInt:
                    bytes = BitConverter.GetBytes((uint)boxed);
                    break;
                case TacoType.UInt64:
                    bytes = BitConverter.GetBytes((ulong)boxed);
                    break;
                case TacoType.Double:
                    bytes = BitConverter.GetBytes((double)boxed);
                    break;
                case TacoType.String:
                    bytes = Codec.Text.GetBytes((string)boxed ?? "");
                    break;
                default:
                    bytes = (byte[])((byte[])boxed ?? new byte[0]).Clone();
                    break;
            }
            error = null;
            return true;
        }

        private static object Decode(TacoType type, byte[] value)
        {
            switch (type)
            {
                case TacoType.Bool:
                    return value[0] != 0;
                case TacoType.Int:
                    return BitConverter.ToInt32(value, 0);
                case TacoType.Int64:
                    return BitConverter.ToInt64(value, 0);
                case TacoType.UInt:
                    return BitConverter.ToUInt32(value, 0);
                case TacoType.UInt64:
                    return BitConverter.ToUInt64(value, 0);
                case TacoType.Double:
                    return BitConverter.ToDouble(value, 0);
                case TacoType.String:
                    return Codec.Text.GetString(value);
                default:
                    return value.Clone();
            }
        }

        public bool Has(string key)
        {
            return IndexOf(key) >= 0;
        }

        public bool Remove(string key)
        {
            int index = IndexOf(key);
            if (index < 0)
                return false;
            entries_.RemoveAt(index);
            return true;
        }

        internal void AddRaw(string key, TacoType type, byte[] value)
        {
            entries_.Add(new Entry { Key = key, Type = type, Value = value });
        }

        internal void WriteTo(VarWriter writer)
        {
            foreach (Entry entry in entries_)
                writer.Raw(entry.Key, entry.Type, entry.Value);
        }

        internal void WriteText(TextBuilder builder)
        {
            if (entries_.Count == 0)
                return;
            builder.BeginExt();
            foreach (Entry entry in entries_)
                builder.ExtEntry(entry.Key, entry.Type, entry.Value);
            builder.EndExt();
        }

        private int IndexOf(string key)
        {
            for (int i = 0; i < entries_.Count; ++i)
            {
                if (entries_[i].Key == key)
                    return i;
            }
            return -1;
        }

        private bool Put(string key, TacoType type, byte[] value, out string error)
        {
            error = null;
            if (string.IsNullOrEmpty(key))
                error = "ext key is empty";
            else if (Codec.Text.GetByteCount(key) > 255)
                error = "ext key is longer than 255 bytes: " + key.Substring(0, Math.Min(32, key.Length)) + "...";
            else if (key[0] == '_')
                error = "ext key starts with '_' (reserved): " + key;
            else if (isSchemaKey_ != null && isSchemaKey_(key))
                error = "ext key is a field of the message: " + key;
            if (error != null)
                return false;
            int index = IndexOf(key);
            if (index >= 0)
            {
                entries_[index].Type = type;
                entries_[index].Value = value;
                return true;
            }
            entries_.Add(new Entry { Key = key, Type = type, Value = value });
            return true;
        }
    }

    /// <summary>ToString style. Default: one line. Pretty(): one field per line, indented.</summary>
    public sealed class TextStyle
    {
        /// <summary>Between fields and list items on one line.</summary>
        public string Separator { get; set; } = ", ";

        public string Open { get; set; } = "{";

        public string Close { get; set; } = "}";

        /// <summary>Spaces per level. 0 = one line.</summary>
        public int Indent { get; set; }

        /// <summary>Longer strings are cut. Bytes show at most 8 bytes.</summary>
        public int MaxText { get; set; } = 64;

        public static TextStyle Pretty(int indentWidth = 2)
        {
            return new TextStyle { Indent = indentWidth };
        }
    }

    /// <summary>
    /// Text for ToString (generated code only). Same text as C++ for ASCII values.
    /// Strings are quoted and escaped, long values are cut (TextStyle.MaxText), ext entries go under ext{...}.
    /// </summary>
    public sealed class TextBuilder
    {
        private readonly TextStyle style_;
        private readonly StringBuilder out_ = new StringBuilder();
        private readonly List<bool> first_ = new List<bool>();   // per open level: no item written yet

        private TextBuilder(TextStyle style)
        {
            style_ = style ?? new TextStyle();
        }

        public static string Format(IMessage message, TextStyle style)
        {
            TextBuilder builder = new TextBuilder(style);
            builder.Message(message);
            return builder.out_.ToString();
        }

        public void Field(string key, bool value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, int value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, long value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, uint value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, ulong value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, double value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, string value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, byte[] value)
        {
            Key(key);
            Value(value);
        }

        public void Field(string key, IMessage value)
        {
            Key(key);
            Message(value);
        }

        public void Field(string key, List<int> value)
        {
            Key(key);
            Items(value, Value);
        }

        public void Field(string key, List<long> value)
        {
            Key(key);
            Items(value, Value);
        }

        public void Field(string key, List<uint> value)
        {
            Key(key);
            Items(value, Value);
        }

        public void Field(string key, List<ulong> value)
        {
            Key(key);
            Items(value, Value);
        }

        public void Field(string key, List<double> value)
        {
            Key(key);
            Items(value, Value);
        }

        public void Field(string key, List<string> value)
        {
            Key(key);
            Items(value, Value);
        }

        public void Field(string key, List<byte[]> value)
        {
            Key(key);
            Items(value, Value);
        }

        public void Field<T>(string key, List<T> value) where T : IMessage
        {
            Key(key);
            Items(value, item => Message(item));
        }

        internal void BeginExt()
        {
            Item();
            out_.Append("ext");
            Open(style_.Open);
        }

        internal void ExtEntry(string key, TacoType type, byte[] value)
        {
            Key(key);
            RawValue(type, value);
        }

        internal void EndExt()
        {
            Close(style_.Close);
        }

        // Name{field=value, ...}
        private void Message(IMessage message)
        {
            if (message == null)
            {
                out_.Append("null");
                return;
            }
            out_.Append(message.Name);
            Open(style_.Open);
            message.WriteText(this);
            message.Ext.WriteText(this);
            Close(style_.Close);
        }

        private void Key(string key)
        {
            Item();
            out_.Append(key).Append('=');
        }

        private void Items<T>(List<T> list, Action<T> write)
        {
            Open("[");
            if (list != null)
            {
                foreach (T item in list)
                {
                    Item();
                    write(item);
                }
            }
            Close("]");
        }

        private void Value(bool value)
        {
            out_.Append(value ? "true" : "false");
        }

        private void Value(int value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        private void Value(long value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        private void Value(uint value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        private void Value(ulong value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        // Same digits as C++ "%.15g".
        private void Value(double value)
        {
            if (double.IsNaN(value))
                out_.Append("nan");
            else if (double.IsInfinity(value))
                out_.Append(value < 0 ? "-inf" : "inf");
            else if (value == 0)
                out_.Append('0');   // -0 too
            else
                out_.Append(value.ToString("G15", CultureInfo.InvariantCulture).Replace('E', 'e'));
        }

        // C# counts characters, C++ counts bytes.
        private void Value(string value)
        {
            value = value ?? "";
            int shown = Math.Min(value.Length, style_.MaxText);
            out_.Append('"');
            for (int i = 0; i < shown; ++i)
                Escaped(value[i]);
            out_.Append('"');
            if (shown < value.Length)
                out_.Append("...(+").Append((value.Length - shown).ToString(CultureInfo.InvariantCulture)).Append(')');
        }

        private void Value(byte[] value)
        {
            value = value ?? new byte[0];
            out_.Append('<').Append(value.Length.ToString(CultureInfo.InvariantCulture)).Append(" bytes");
            int shown = Math.Min(value.Length, Math.Min(style_.MaxText, 8));
            for (int i = 0; i < shown; ++i)
                out_.Append(i == 0 ? ": " : " ").Append(value[i].ToString("X2", CultureInfo.InvariantCulture));
            if (shown < value.Length)
                out_.Append(" ...");
            out_.Append('>');
        }

        // ext values carry only their kind, not a schema.
        private void RawValue(TacoType type, byte[] value)
        {
            string size = value.Length.ToString(CultureInfo.InvariantCulture);
            switch (type)
            {
                case TacoType.Bool when value.Length == 1:
                    Value(value[0] != 0);
                    return;
                case TacoType.Int when value.Length == 4:
                    Value(BitConverter.ToInt32(value, 0));
                    return;
                case TacoType.Int64 when value.Length == 8:
                    Value(BitConverter.ToInt64(value, 0));
                    return;
                case TacoType.UInt when value.Length == 4:
                    Value(BitConverter.ToUInt32(value, 0));
                    return;
                case TacoType.UInt64 when value.Length == 8:
                    Value(BitConverter.ToUInt64(value, 0));
                    return;
                case TacoType.Double when value.Length == 8:
                    Value(BitConverter.ToDouble(value, 0));
                    return;
                case TacoType.Bool:
                case TacoType.Int:
                case TacoType.Int64:
                case TacoType.UInt:
                case TacoType.UInt64:
                case TacoType.Double:
                    out_.Append("<bad value>");
                    return;
                case TacoType.String:
                    Value(Codec.Text.GetString(value));
                    return;
                case TacoType.Bytes:
                    Value(value);
                    return;
                case TacoType.Message:
                    out_.Append("<message ").Append(size).Append(" bytes>");
                    return;
                case TacoType.List:
                    out_.Append("<list ").Append(size).Append(" bytes>");
                    return;
                default:
                    out_.Append("<type ").Append(((int)type).ToString(CultureInfo.InvariantCulture)).Append(", ").Append(size).Append(" bytes>");
                    return;
            }
        }

        private void Escaped(char c)
        {
            switch (c)
            {
                case '"':
                    out_.Append("\\\"");
                    return;
                case '\\':
                    out_.Append("\\\\");
                    return;
                case '\n':
                    out_.Append("\\n");
                    return;
                case '\r':
                    out_.Append("\\r");
                    return;
                case '\t':
                    out_.Append("\\t");
                    return;
            }
            if (c < 0x20 || c == 0x7F)
                out_.Append("\\x").Append(((int)c).ToString("X2", CultureInfo.InvariantCulture));
            else
                out_.Append(c);
        }

        private void Open(string text)
        {
            out_.Append(text);
            first_.Add(true);
        }

        private void Close(string text)
        {
            bool empty = first_[first_.Count - 1];
            first_.RemoveAt(first_.Count - 1);
            if (style_.Indent > 0 && !empty)
                NewLine();
            out_.Append(text);
        }

        private void Item()
        {
            int last = first_.Count - 1;
            if (!first_[last])
                out_.Append(style_.Indent > 0 ? "," : style_.Separator);
            first_[last] = false;
            if (style_.Indent > 0)
                NewLine();
        }

        private void NewLine()
        {
            out_.Append('\n').Append(' ', first_.Count * style_.Indent);
        }
    }
}
