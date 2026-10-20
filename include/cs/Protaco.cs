// Protaco.cs. C# runtime for protacoc generated code (*.g.cs). .NET Framework 4.6.1, C# 7.3.
// Produces the same bytes as the C++ runtime (Protaco.hpp). Format v4: README.md "Serialization format".
// Strings are UTF-8 on every platform. Try* calls report failure through the return value and an error string.
// protacoc.exe writes this file next to the generated code, so generator and runtime versions always match.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Threading;

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
        Map = 11,
    }

    /// <summary>
    /// Message envelope, stored in the frame header. Id: new id per message (Envelopes.NextId), Corr: id of the request this
    /// answers (set by the user), Ts: UTC microseconds since 1970-01-01 (Envelopes.NowUs). Id and Ts are set when a message
    /// object is created.
    /// </summary>
    public struct Envelope
    {
        public uint Id;
        public uint Corr;
        public long Ts;
    }

    /// <summary>Constructor tag: no envelope id or time. Generated code uses it for nested fields.</summary>
    public struct NoStamp
    {
        public static readonly NoStamp Value = default(NoStamp);
    }

    /// <summary>Envelope ids, the clock and timestamp conversion. Same rules as the C++ runtime.</summary>
    public static class Envelopes
    {
        /// <summary>2000-01-01 00:00:00 UTC. Smaller timestamps are not treated as times.</summary>
        public const long TimestampMin = 946684800000000L;

        /// <summary>9999-12-31 23:59:59.999999 UTC.</summary>
        public const long TimestampMax = 253402300799999999L;

        private const long UnixEpochTicks = 621355968000000000L;
        private static int lastId_;
        private static Func<long> clock_ = SystemClockUs;
        private static volatile bool autoEnvelope_ = true;

        internal static bool AutoEnvelope => autoEnvelope_;

        /// <summary>New message objects of this process keep Id 0 and Ts 0, as with auto_envelope false. Thread-safe.</summary>
        public static void DisableAutoEnvelope()
        {
            autoEnvelope_ = false;
        }

        /// <summary>Default: a new message object gets Id = NextId() and Ts = NowUs(), unless its yaml has auto_envelope false.</summary>
        public static void EnableAutoEnvelope()
        {
            autoEnvelope_ = true;
        }

        /// <summary>Next envelope id of this process: 1, 2, 3, ... and 0 is skipped after wrap-around. Thread-safe.</summary>
        public static uint NextId()
        {
            uint id;
            do
                id = unchecked((uint)Interlocked.Increment(ref lastId_));
            while (id == 0);
            return id;
        }

        /// <summary>Current time in UTC microseconds since 1970-01-01 from the clock set by SetClock (default: the system clock).</summary>
        public static long NowUs()
        {
            return clock_();
        }

        /// <summary>Replaces the time source of NowUs, for example with a simulated clock. null restores the system clock.</summary>
        public static void SetClock(Func<long> nowUs)
        {
            clock_ = nowUs ?? SystemClockUs;
        }

        private static long SystemClockUs()
        {
            return (DateTime.UtcNow.Ticks - UnixEpochTicks) / 10;
        }

        /// <summary>Timestamp (UTC microseconds) to local time. Values outside 2000-01-01..9999-12-31 are not times: returns false.</summary>
        public static bool TryGetTime(long timestampUs, out int year, out int month, out int day, out int hour, out int minute, out int second,
                                      out int millisecond, out int microsecond, out string error)
        {
            year = month = day = hour = minute = second = millisecond = microsecond = 0;
            if (timestampUs < TimestampMin || timestampUs > TimestampMax)
            {
                error = "timestamp " + timestampUs.ToString(CultureInfo.InvariantCulture) + " is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)";
                return false;
            }
            DateTime local = new DateTime(UnixEpochTicks + timestampUs * 10, DateTimeKind.Utc).ToLocalTime();
            int fraction = (int)(timestampUs % 1000000);
            year = local.Year;
            month = local.Month;
            day = local.Day;
            hour = local.Hour;
            minute = local.Minute;
            second = local.Second;
            millisecond = fraction / 1000;
            microsecond = fraction % 1000;
            error = null;
            return true;
        }

        /// <summary>Timestamp to local "yyyy-MM-dd" and "HH:mm:ss.ffffff".</summary>
        public static bool TryGetTime(long timestampUs, out string date, out string time, out string error)
        {
            int year;
            int month;
            int day;
            int hour;
            int minute;
            int second;
            int millisecond;
            int microsecond;
            date = null;
            time = null;
            if (!TryGetTime(timestampUs, out year, out month, out day, out hour, out minute, out second, out millisecond, out microsecond, out error))
                return false;
            date = year.ToString("D4", CultureInfo.InvariantCulture) + "-" + month.ToString("D2", CultureInfo.InvariantCulture) + "-"
                + day.ToString("D2", CultureInfo.InvariantCulture);
            time = hour.ToString("D2", CultureInfo.InvariantCulture) + ":" + minute.ToString("D2", CultureInfo.InvariantCulture) + ":"
                + second.ToString("D2", CultureInfo.InvariantCulture) + "." + (millisecond * 1000 + microsecond).ToString("D6", CultureInfo.InvariantCulture);
            return true;
        }
    }

    /// <summary>
    /// Base of every generated message: the envelope carried in the frame header. A new object gets Id = Envelopes.NextId()
    /// and Ts = Envelopes.NowUs(); Corr starts at 0. Objects created by the decoder or as nested fields get none.
    /// auto_envelope false in the yaml, or Envelopes.DisableAutoEnvelope(), leaves Id and Ts at 0.
    /// </summary>
    public abstract class Message
    {
        private Envelope env_;

        protected Message()
        {
            if (Codec.Decoding || !Envelopes.AutoEnvelope)
                return;
            env_.Id = Envelopes.NextId();
            env_.Ts = Envelopes.NowUs();
        }

        protected Message(NoStamp noStamp)
        {
        }

        public uint GetId()
        {
            return env_.Id;
        }

        public void SetId(uint id)
        {
            env_.Id = id;
        }

        /// <summary>Id of the message this one answers. Set by the user.</summary>
        public uint GetCorr()
        {
            return env_.Corr;
        }

        public void SetCorr(uint corr)
        {
            env_.Corr = corr;
        }

        /// <summary>UTC microseconds since 1970-01-01.</summary>
        public long GetTimestamp()
        {
            return env_.Ts;
        }

        public void SetTimestamp(long timestampUs)
        {
            env_.Ts = timestampUs;
        }

        public void SetTimestampAsNow()
        {
            env_.Ts = Envelopes.NowUs();
        }

        public Envelope GetEnvelope()
        {
            return env_;
        }

        public void SetEnvelope(Envelope env)
        {
            env_ = env;
        }

        /// <summary>Timestamp as local "yyyy-MM-dd" and "HH:mm:ss.ffffff". False if the timestamp is not a time.</summary>
        public bool TryGetTimeFromTimestamp(out string date, out string time)
        {
            string error;
            return Envelopes.TryGetTime(env_.Ts, out date, out time, out error);
        }

        public bool TryGetTimeFromTimestamp(out string date, out string time, out string error)
        {
            return Envelopes.TryGetTime(env_.Ts, out date, out time, out error);
        }

        public bool TryGetTimeFromTimestamp(out int year, out int month, out int day, out int hour, out int minute, out int second,
                                            out int millisecond, out int microsecond)
        {
            string error;
            return Envelopes.TryGetTime(env_.Ts, out year, out month, out day, out hour, out minute, out second, out millisecond, out microsecond, out error);
        }

        public bool TryGetTimeFromTimestamp(out int year, out int month, out int day, out int hour, out int minute, out int second,
                                            out int millisecond, out int microsecond, out string error)
        {
            return Envelopes.TryGetTime(env_.Ts, out year, out month, out day, out hour, out minute, out second, out millisecond, out microsecond, out error);
        }
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
        public const int MaxDepth = 100;
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

        // Objects created while decoding take no envelope id or time (Message constructor).
        [ThreadStatic]
        private static int decoding_;

        internal static bool Decoding => decoding_ > 0;

        /// <summary>Bytes to object, envelope included. Throws ProtacoException on failure.</summary>
        public static T ToObject<T>(byte[] bytes) where T : IMessage, new()
        {
            ++decoding_;
            try
            {
                return Decode<T>(bytes);
            }
            finally
            {
                --decoding_;
            }
        }

        private static T Decode<T>(byte[] bytes) where T : IMessage, new()
        {
            T message = new T();
            ushort mine = message.Version;
            int headerSize;
            Header header = CheckFrame(bytes, message.Name, mine, message.MinVersion, message.SchemaHashes[message.SchemaHashes.Length - 1], out headerSize);
            (message as Protaco.Message)?.SetEnvelope(header.Env);
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

        // Checks shared by ToObject and TryFixedBlock: header, name, supported versions, own fingerprint, body length.
        private static Header CheckFrame(byte[] bytes, string name, ushort mine, ushort minVersion, uint expected, out int headerSize)
        {
            if (bytes.Length > MaxFrameBytes)
                throw new ProtacoException("", "frame too large");
            Header header = ReadHeader(bytes);
            if (header.Name != name)
                throw new ProtacoException("", "message name mismatch: expected " + name + ", got " + header.Name);
            if (header.Version < mine)
            {
                string what = "message version too old: " + name + " reads version " + mine + " or newer, got " + header.Version;
                if (header.Version >= minVersion)
                    what += ". Use the v" + header.Version + " type";
                throw new ProtacoException("", what);
            }
            if (header.MinVersion > mine)
                throw new ProtacoException("", "message version too new: " + name + " reads version " + mine + ", sender supports " + header.MinVersion + ".." + header.Version);
            int nameLength = bytes[3];
            uint hash = BitConverter.ToUInt32(bytes, 28 + nameLength + 4 * (mine - header.MinVersion));
            if (hash != expected)
                throw new ProtacoException("", "schema mismatch: " + name + " version " + mine + " expected " + Hex32(expected) + ", got " + Hex32(hash));
            headerSize = 28 + nameLength + 4 * (header.Version - header.MinVersion + 1);
            long left = bytes.Length - headerSize;
            if (left != header.BodySize)
                throw new ProtacoException("", left < header.BodySize ? "frame truncated" : "trailing bytes after frame");
            return header;
        }

        /// <summary>Generated TryView methods call this. Checks the frame like ToObject; fixedAt is where the fixed block starts.</summary>
        public static bool TryFixedBlock(byte[] bytes, string name, ushort version, ushort minVersion, uint schemaHash, int fixedSize,
            bool allowExtension, out int fixedAt, out string error)
        {
            fixedAt = 0;
            try
            {
                int headerSize;
                Header header = CheckFrame(bytes, name, version, minVersion, schemaHash, out headerSize);
                bool newer = header.Version > version;
                try
                {
                    int count = (int)header.BodySize;
                    uint fixedLength = TakeFixed(bytes, headerSize, count, (uint)fixedSize, newer);
                    int varStart = headerSize + 4 + (int)fixedLength;
                    int varCount = headerSize + count - varStart;
                    // ext entries or fields of a newer version: checked like ToObject, not visible through the view.
                    if (varCount != 0)
                    {
                        VarReader reader = new VarReader(bytes, varStart, varCount, 0);
                        if (!allowExtension && !newer)
                            reader.MoveUnusedTo(null, false, false);
                    }
                    fixedAt = headerSize + 4;
                }
                catch (ProtacoException e) when (newer)
                {
                    throw new ProtacoException(e.Path, e.What + " (frame version " + header.Version + " read as " + version + ")");
                }
                error = null;
                return true;
            }
            catch (ProtacoException e)
            {
                error = e.Message;
                return false;
            }
        }

        /// <summary>Envelope of a frame (generated View).</summary>
        public static Envelope EnvelopeOf(byte[] frame)
        {
            int p = 8 + frame[3];
            Envelope env;
            env.Id = BitConverter.ToUInt32(frame, p);
            env.Corr = BitConverter.ToUInt32(frame, p + 4);
            env.Ts = BitConverter.ToInt64(frame, p + 8);
            return env;
        }

        /// <summary>A string(N) value: UTF-8 bytes up to the first 0 (generated View).</summary>
        public static string Chars(byte[] src, int at, int capacity)
        {
            int length = 0;
            while (length < capacity && src[at + length] != 0)
                ++length;
            return Text.GetString(src, at, length);
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
                case TacoType.Map:
                    return "map";
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

        /// <summary>Generated fixed-length arrays with a default: count copies of value.</summary>
        public static T[] Filled<T>(int count, T value)
        {
            T[] items = new T[count];
            for (int i = 0; i < count; ++i)
                items[i] = value;
            return items;
        }

        /// <summary>Generated arrays of fixed messages: one new object per element.</summary>
        public static T[] Created<T>(int count, Func<T> create)
        {
            T[] items = new T[count];
            for (int i = 0; i < count; ++i)
                items[i] = create();
            return items;
        }

        /// <summary>Generated ConstRef.Copy: the fixed block at at read into target.</summary>
        public static T FixedCopy<T>(T target, byte[] src, int at) where T : IMessage
        {
            target.ReadFixed(new FixedReader(src, at));
            return target;
        }

        /// <summary>Generated View element reads: index in 0..length-1, otherwise ArgumentOutOfRangeException.</summary>
        public static int Index(int index, int length)
        {
            if ((uint)index >= (uint)length)
                throw new ArgumentOutOfRangeException("index", index, "fixed array of " + length + " elements");
            return index;
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

        // FixedLen u32 | fixed block. Same version means same size; a newer version only appends. Also guards nested bodies,
        // which carry no fingerprint of their own.
        private static uint TakeFixed(byte[] src, int offset, int count, uint expected, bool newer)
        {
            if (count < 4)
                throw new ProtacoException("", "fixed block truncated");
            uint fixedLength = BitConverter.ToUInt32(src, offset);
            if (fixedLength > (uint)(count - 4))
                throw new ProtacoException("", "fixed block truncated");
            if (newer ? fixedLength < expected : fixedLength != expected)
                throw new ProtacoException("", "fixed block size mismatch: expected " + expected + ", got " + fixedLength);
            return fixedLength;
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
            uint fixedLength = TakeFixed(src, offset, count, (uint)message.FixedSize, newer);
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
        private readonly int base_;

        // at: where this message's fixed block starts in block (an inline fixed message starts inside its parent's).
        internal FixedWriter(byte[] block, int at = 0)
        {
            block_ = block;
            base_ = at;
        }

        public void Bool(int offset, bool value)
        {
            block_[base_ + offset] = (byte)(value ? 1 : 0);
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
            Buffer.BlockCopy(bytes, 0, block_, base_ + offset, Math.Min(bytes.Length, capacity));
            for (int i = bytes.Length; i < capacity; ++i)
                block_[base_ + offset + i] = 0;
        }

        /// <summary>string(N) element of a fixed-length array. Elements are not checked on assignment, so Serialize checks them here.</summary>
        public void Chars(int offset, int capacity, string value, string key, int index)
        {
            int length = Codec.Text.GetByteCount(value ?? "");
            if (length > capacity)
                throw new ProtacoException(key + "[" + index + "]", length + " bytes, longer than string(" + capacity + ")");
            Chars(offset, capacity, value);
        }

        /// <summary>Inline fixed message: its fixed fields at offset. null is an error (ProtacoException at key).</summary>
        public void Message(int offset, IMessage value, string key)
        {
            if (value == null)
                throw new ProtacoException(key, "null fixed message");
            value.WriteFixed(new FixedWriter(block_, base_ + offset));
        }

        public void Message(int offset, IMessage value, string key, int index)
        {
            Message(offset, value, key + "[" + index + "]");
        }

        private void Put(int offset, byte[] bytes)
        {
            Buffer.BlockCopy(bytes, 0, block_, base_ + offset, bytes.Length);
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
            return Codec.Chars(src_, start_ + offset, capacity);
        }

        /// <summary>Inline fixed message at offset, read into target (a new object if target is null).</summary>
        public T Message<T>(int offset, T target) where T : IMessage, new()
        {
            if (target == null)
                target = new T();
            target.ReadFixed(new FixedReader(src_, start_ + offset));
            return target;
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

        /// <summary>A map field (or any value with an item descriptor).</summary>
        public void Value<T>(string key, T value, Item<T> item)
        {
            Tlv(key, item.Type, () => item.Write(writer_, value));
        }

        internal void Raw(string key, TacoType type, byte[] value)
        {
            Tlv(key, type, () => writer_.Write(value));
        }

        // Len u32 slot before a value: BeginSized writes 0 and returns the value start, EndSized fills the length.
        internal static long BeginSized(BinaryWriter writer)
        {
            writer.Write(0u);
            return writer.BaseStream.Position;
        }

        internal static void EndSized(BinaryWriter writer, long start)
        {
            Stream stream = writer.BaseStream;
            long end = stream.Position;
            stream.Position = start - 4;
            writer.Write((uint)(end - start));
            stream.Position = end;
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
            long start = BeginSized(writer_);
            writeValue();
            EndSized(writer_, start);
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

        /// <summary>A map field (or any value with an item descriptor).</summary>
        public T Value<T>(string key, bool required, T fallback, Item<T> item)
        {
            Raw raw = Take(key, item.Type, required);
            if (raw == null)
                return fallback;
            return Guard(key, () => item.Read(src_, raw.Offset, raw.Length, depth_));
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

    /// <summary>One kind of map key or value: how it is written, read and shown (generated code only, see Items).</summary>
    public sealed class Item<T>
    {
        internal readonly TacoType Type;
        internal readonly Action<BinaryWriter, T> Write;
        internal readonly Func<byte[], int, int, int, T> Read;   // src, offset, length, depth of the enclosing body
        internal readonly Action<TextBuilder, T> Text;
        internal readonly Comparison<T> Order;                   // key order in ToString

        internal Item(TacoType type, Action<BinaryWriter, T> write, Func<byte[], int, int, int, T> read, Action<TextBuilder, T> text,
            Comparison<T> order = null)
        {
            Type = type;
            Write = write;
            Read = read;
            Text = text;
            Order = order ?? Comparer<T>.Default.Compare;
        }
    }

    /// <summary>
    /// Item descriptors of generated map fields. Same bytes as the C++ codecs:
    /// list: ElemType u8 | Count u32 | (Len u32 | Value) x Count,
    /// map: KeyType u8 | ValueType u8 | Count u32 | (KeyLen u32 | Key | ValueLen u32 | Value) x Count.
    /// </summary>
    public static class Items
    {
        public static readonly Item<bool> Bool = new Item<bool>(TacoType.Bool, (w, v) => w.Write((byte)(v ? 1 : 0)),
            (s, o, l, d) => s[Scalar(o, l, 1)] != 0, (b, v) => b.Value(v));

        public static readonly Item<int> Int = new Item<int>(TacoType.Int, (w, v) => w.Write(v),
            (s, o, l, d) => BitConverter.ToInt32(s, Scalar(o, l, 4)), (b, v) => b.Value(v));

        public static readonly Item<long> Int64 = new Item<long>(TacoType.Int64, (w, v) => w.Write(v),
            (s, o, l, d) => BitConverter.ToInt64(s, Scalar(o, l, 8)), (b, v) => b.Value(v));

        public static readonly Item<uint> UInt = new Item<uint>(TacoType.UInt, (w, v) => w.Write(v),
            (s, o, l, d) => BitConverter.ToUInt32(s, Scalar(o, l, 4)), (b, v) => b.Value(v));

        public static readonly Item<ulong> UInt64 = new Item<ulong>(TacoType.UInt64, (w, v) => w.Write(v),
            (s, o, l, d) => BitConverter.ToUInt64(s, Scalar(o, l, 8)), (b, v) => b.Value(v));

        public static readonly Item<double> Double = new Item<double>(TacoType.Double, (w, v) => w.Write(v),
            (s, o, l, d) => BitConverter.ToDouble(s, Scalar(o, l, 8)), (b, v) => b.Value(v));

        public static readonly Item<string> String = new Item<string>(TacoType.String, (w, v) => w.Write(Codec.Text.GetBytes(v ?? "")),
            (s, o, l, d) => Codec.Text.GetString(s, o, l), (b, v) => b.Value(v), CodePointOrder);

        public static readonly Item<byte[]> Bytes = new Item<byte[]>(TacoType.Bytes, (w, v) => w.Write(v ?? new byte[0]),
            (s, o, l, d) => Copy(s, o, l), (b, v) => b.Value(v));

        /// <summary>int32 in the serialized data. Numbers this build does not know are kept.</summary>
        public static Item<T> Enum<T>(Func<T, int> toNumber, Func<int, T> fromNumber, Func<T, string> name) where T : struct, System.Enum
        {
            return new Item<T>(TacoType.Int, (w, v) => w.Write(toNumber(v)),
                (s, o, l, d) => fromNumber(BitConverter.ToInt32(s, Scalar(o, l, 4))), (b, v) => b.EnumValue(v, name));
        }

        public static Item<T> Message<T>() where T : IMessage, new()
        {
            return MessageItem<T>.Value;
        }

        public static Item<List<T>> List<T>(Item<T> element)
        {
            return new Item<List<T>>(TacoType.List, (w, v) => WriteList(w, v, element), (s, o, l, d) => ReadList(s, o, l, d, element),
                (b, v) => b.ListItems(v, item => element.Text(b, item)));
        }

        public static Item<Dictionary<K, V>> Map<K, V>(Item<K> key, Item<V> value)
        {
            return new Item<Dictionary<K, V>>(TacoType.Map, (w, v) => WriteMap(w, v, key, value), (s, o, l, d) => ReadMap(s, o, l, d, key, value),
                (b, v) => b.MapItems(v, key, value));
        }

        private static class MessageItem<T> where T : IMessage, new()
        {
            public static readonly Item<T> Value = new Item<T>(TacoType.Message, (w, v) => Codec.WriteBody(v, w), ReadMessage, (b, v) => b.Message(v));

            private static T ReadMessage(byte[] src, int offset, int length, int depth)
            {
                T message = new T();
                Codec.ReadBody(message, src, offset, length, depth + 1, false);
                return message;
            }
        }

        private static int Scalar(int offset, int length, int expected)
        {
            if (length != expected)
                throw new ProtacoException("", "scalar size mismatch");
            return offset;
        }

        private static byte[] Copy(byte[] src, int offset, int length)
        {
            byte[] value = new byte[length];
            Buffer.BlockCopy(src, offset, value, 0, length);
            return value;
        }

        // Unicode code point order, which is UTF-8 byte order: the same key order as C++ std::string.
        private static int CodePointOrder(string a, string b)
        {
            a = a ?? "";
            b = b ?? "";
            int shared = Math.Min(a.Length, b.Length);
            for (int i = 0; i < shared; ++i)
            {
                char x = a[i];
                char y = b[i];
                if (x == y)
                    continue;
                // A surrogate stands for a code point above U+FFFF, so it sorts after every other char.
                bool xs = char.IsSurrogate(x);
                bool ys = char.IsSurrogate(y);
                if (xs != ys)
                    return xs ? 1 : -1;
                return x < y ? -1 : 1;
            }
            return a.Length.CompareTo(b.Length);
        }

        // Len u32 of the next value. False if it runs past end.
        private static bool TakeSize(byte[] src, ref int p, int end, out int length)
        {
            length = 0;
            if (end - p < 4)
                return false;
            uint size = BitConverter.ToUInt32(src, p);
            if (size > (uint)(end - p - 4))
                return false;
            p += 4;
            length = (int)size;
            return true;
        }

        private static void WriteList<T>(BinaryWriter writer, List<T> list, Item<T> element)
        {
            writer.Write((byte)element.Type);
            writer.Write((uint)(list == null ? 0 : list.Count));
            if (list == null)
                return;
            foreach (T item in list)
            {
                long start = VarWriter.BeginSized(writer);
                element.Write(writer, item);
                VarWriter.EndSized(writer, start);
            }
        }

        private static List<T> ReadList<T>(byte[] src, int offset, int length, int depth, Item<T> element)
        {
            int p = offset;
            int end = offset + length;
            if (length < 5)
                throw new ProtacoException("", "list header truncated");
            if ((TacoType)src[p] != element.Type)
                throw new ProtacoException("", "list element type mismatch");
            uint count = BitConverter.ToUInt32(src, p + 1);
            p += 5;
            if (count > (uint)((end - p) / 4))
                throw new ProtacoException("", "list count too large");
            List<T> list = new List<T>((int)count);
            for (int i = 0; i < (int)count; ++i)
            {
                int size;
                if (!TakeSize(src, ref p, end, out size))
                    throw new ProtacoException("[" + i + "]", "list element truncated");
                try
                {
                    list.Add(element.Read(src, p, size, depth));
                }
                catch (ProtacoException e)
                {
                    throw e.Prepend("[" + i + "]");
                }
                p += size;
            }
            if (p != end)
                throw new ProtacoException("", "trailing bytes in list");
            return list;
        }

        // Entries in the dictionary's order: two maps with the same entries may differ in bytes.
        private static void WriteMap<K, V>(BinaryWriter writer, Dictionary<K, V> map, Item<K> key, Item<V> value)
        {
            writer.Write((byte)key.Type);
            writer.Write((byte)value.Type);
            writer.Write((uint)(map == null ? 0 : map.Count));
            if (map == null)
                return;
            foreach (KeyValuePair<K, V> entry in map)
            {
                long start = VarWriter.BeginSized(writer);
                key.Write(writer, entry.Key);
                VarWriter.EndSized(writer, start);
                start = VarWriter.BeginSized(writer);
                value.Write(writer, entry.Value);
                VarWriter.EndSized(writer, start);
            }
        }

        private static Dictionary<K, V> ReadMap<K, V>(byte[] src, int offset, int length, int depth, Item<K> key, Item<V> value)
        {
            int p = offset;
            int end = offset + length;
            if (length < 6)
                throw new ProtacoException("", "map header truncated");
            if ((TacoType)src[p] != key.Type || (TacoType)src[p + 1] != value.Type)
                throw new ProtacoException("", "map key or value type mismatch");
            uint count = BitConverter.ToUInt32(src, p + 2);
            p += 6;
            if (count > (uint)((end - p) / 8))
                throw new ProtacoException("", "map count too large");
            Dictionary<K, V> map = new Dictionary<K, V>((int)count);
            for (int i = 0; i < (int)count; ++i)
            {
                string at = "[" + i + "]";
                int size;
                K k;
                if (!TakeSize(src, ref p, end, out size))
                    throw new ProtacoException(at, "map key truncated or bad");
                try
                {
                    k = key.Read(src, p, size, depth);
                }
                catch (ProtacoException)
                {
                    throw new ProtacoException(at, "map key truncated or bad");
                }
                p += size;
                if (!TakeSize(src, ref p, end, out size))
                    throw new ProtacoException(at, "map value truncated");
                V v;
                try
                {
                    v = value.Read(src, p, size, depth);
                }
                catch (ProtacoException e)
                {
                    throw e.Prepend(at);
                }
                p += size;
                if (map.ContainsKey(k))
                    throw new ProtacoException(at, "duplicate map key");
                map.Add(k, v);
            }
            if (p != end)
                throw new ProtacoException("", "trailing bytes in map");
            return map;
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

        /// <summary>Show envelope{id, corr, ts} of the top-level message first.</summary>
        public bool Envelope { get; set; }

        public static TextStyle Pretty(int indentWidth = 2)
        {
            return new TextStyle { Indent = indentWidth, Envelope = true };
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
            builder.Message(message, true);
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
            ListItems(value, Value);
        }

        public void Field(string key, List<long> value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, List<uint> value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, List<ulong> value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, List<double> value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, List<string> value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, List<byte[]> value)
        {
            Key(key);
            ListItems(value, Value);
        }

        /// <summary>Enum field: Name(number), ?(number) for a number the build does not know.</summary>
        public void Field<T>(string key, T value, Func<T, string> name) where T : struct, Enum
        {
            Key(key);
            EnumValue(value, name);
        }

        public void Field<T>(string key, List<T> value, Func<T, string> name) where T : struct, Enum
        {
            Key(key);
            ListItems(value, item => EnumValue(item, name));
        }

        /// <summary>Fixed-length arrays: the same text as lists.</summary>
        public void Field(string key, bool[] value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, int[] value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, long[] value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, uint[] value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, ulong[] value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, double[] value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field(string key, string[] value)
        {
            Key(key);
            ListItems(value, Value);
        }

        public void Field<T>(string key, T[] value, Func<T, string> name) where T : struct, Enum
        {
            Key(key);
            ListItems(value, item => EnumValue(item, name));
        }

        /// <summary>Map field: {key: value, ...} with keys sorted, so the text does not depend on the dictionary order.</summary>
        public void Field<T>(string key, T value, Item<T> item)
        {
            Key(key);
            item.Text(this, value);
        }

        internal void EnumValue<T>(T value, Func<T, string> name) where T : struct, Enum
        {
            out_.Append(name(value) ?? "?").Append('(').Append(Convert.ToInt32(value).ToString(CultureInfo.InvariantCulture)).Append(')');
        }

        public void Field<T>(string key, List<T> value) where T : IMessage
        {
            Key(key);
            ListItems(value, item => Message(item));
        }

        /// <summary>Array of fixed messages: the same text as a list of messages.</summary>
        public void Field<T>(string key, T[] value) where T : IMessage
        {
            Key(key);
            ListItems(value, item => Message(item));
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

        // Name{field=value, ...}. The top-level message shows its envelope first when TextStyle.Envelope is set.
        internal void Message(IMessage message, bool top = false)
        {
            if (message == null)
            {
                out_.Append("null");
                return;
            }
            out_.Append(message.Name);
            Open(style_.Open);
            Protaco.Message envelopeOwner = message as Protaco.Message;
            if (top && style_.Envelope && envelopeOwner != null)
                EnvelopeItem(envelopeOwner.GetEnvelope());
            message.WriteText(this);
            message.Ext.WriteText(this);
            Close(style_.Close);
        }

        // envelope{id=7, corr=0, ts=2026-10-20 14:03:22.123456}. Ts outside the time range prints as the integer.
        private void EnvelopeItem(Envelope env)
        {
            Item();
            string date;
            string time;
            string error;
            string ts = Envelopes.TryGetTime(env.Ts, out date, out time, out error) ? date + " " + time : env.Ts.ToString(CultureInfo.InvariantCulture);
            out_.Append("envelope").Append(style_.Open)
                .Append("id=").Append(env.Id.ToString(CultureInfo.InvariantCulture)).Append(style_.Separator)
                .Append("corr=").Append(env.Corr.ToString(CultureInfo.InvariantCulture)).Append(style_.Separator)
                .Append("ts=").Append(ts).Append(style_.Close);
        }

        private void Key(string key)
        {
            Item();
            out_.Append(key).Append('=');
        }

        internal void ListItems<T>(IEnumerable<T> list, Action<T> write)
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

        internal void MapItems<K, V>(Dictionary<K, V> map, Item<K> key, Item<V> value)
        {
            Open("{");
            if (map != null)
            {
                List<K> keys = new List<K>(map.Keys);
                keys.Sort(key.Order);
                foreach (K k in keys)
                {
                    Item();
                    key.Text(this, k);
                    out_.Append(": ");
                    value.Text(this, map[k]);
                }
            }
            Close("}");
        }

        internal void Value(bool value)
        {
            out_.Append(value ? "true" : "false");
        }

        internal void Value(int value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        internal void Value(long value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        internal void Value(uint value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        internal void Value(ulong value)
        {
            out_.Append(value.ToString(CultureInfo.InvariantCulture));
        }

        // Same digits as C++ "%.15g".
        internal void Value(double value)
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
        internal void Value(string value)
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

        internal void Value(byte[] value)
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
                case TacoType.Map:
                    out_.Append("<map ").Append(size).Append(" bytes>");
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
