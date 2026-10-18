// Protaco.hpp. Header-only runtime for protacoc generated code (*.g.h).
// C++14 compatible. Only ToObject throws. Format v3: README.md "Serialization format".
// protacoc.exe writes this file next to the generated code, so generator and runtime versions always match.
#pragma once
// Each output folder has its own copy. The same version is included once; mixing versions is a redefinition error.
#ifndef PROTACO_RUNTIME_0_4_0
#define PROTACO_RUNTIME_0_4_0

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace protaco
{

/// Value kinds. The numbers are part of the serialized data. Never change them; add new kinds at the end.
/// Every value carries a length, so unknown kinds are skipped.
enum class TacoType : uint8_t
{
    Bool = 1,
    Int = 2,
    Int64 = 3,
    Double = 4,
    String = 5,
    Message = 6,
    List = 7,
    Bytes = 8,
};

enum : uint32_t
{
    kFrameVersion = 3,
    kMaxDepth = 8,                 // max nested message depth
    kMaxSchemaVersions = 255,      // max versions in a frame's window
    kMaxFields = 1024,             // max variable fields per body
    kMaxFrameBytes = 1048575,      // max frame size
    kMaxKeyLength = 255,
};

/// Message envelope: request id, correlation id (id of the original request), time (us).
struct Envelope
{
    uint32_t id = 0;
    uint32_t corr = 0;
    int64_t ts = 0;
};

/// Frame header. PeekHeader reads it without parsing the body (for dispatch).
struct Header
{
    std::string name;
    uint16_t version = 0;      // sender's version of the message
    uint16_t minVersion = 0;   // lowest version of the sender's window; the frame has a fingerprint per version
    uint32_t schemaHash = 0;   // fingerprint of version (the sender's MsgSchemaHash)
    Envelope env;
    uint32_t bodySize = 0;     // whole frame = header (28 + name + 4 per fingerprint) + bodySize
};

/// ToString style. Default: one line. Pretty(): one field per line, indented.
struct TextStyle
{
    const char* separator = ", ";   // between fields and list items on one line
    const char* open = "{";
    const char* close = "}";
    int indent = 0;                 // spaces per level; 0 = one line
    size_t maxText = 64;            // longer strings are cut; bytes show at most 8 bytes

    static TextStyle Pretty(int indentWidth = 2)
    {
        TextStyle style;
        style.indent = indentWidth;
        return style;
    }
};

/// Decode error. path looks like "moves[2].from".
struct Error
{
    std::string path;
    std::string what;
};

/// Thrown by ToObject. Catch it in the same module; never let it cross a DLL boundary.
class CParseError : public std::runtime_error
{
public:
    explicit CParseError(const Error& error)
        : std::runtime_error(error.path.empty() ? error.what : error.path + ": " + error.what)
        , error_(error)
    {
    }

    const Error& GetError() const
    {
        return error_;
    }

private:
    Error error_;
};

namespace detail
{
struct Access;
}

/// Extra key-value data not in the yaml. Receivers keep unknown keys here.
/// Schema keys and keys starting with '_' are rejected (Set returns false).
class CExt
{
public:
    bool SetBool(const std::string& key, bool value)
    {
        const uint8_t raw = value ? 1 : 0;
        return Put(key, TacoType::Bool, &raw, sizeof(raw));
    }

    bool SetInt(const std::string& key, int32_t value)
    {
        return Put(key, TacoType::Int, &value, sizeof(value));
    }

    bool SetInt64(const std::string& key, int64_t value)
    {
        return Put(key, TacoType::Int64, &value, sizeof(value));
    }

    bool SetDouble(const std::string& key, double value)
    {
        return Put(key, TacoType::Double, &value, sizeof(value));
    }

    bool SetString(const std::string& key, const std::string& value)
    {
        return Put(key, TacoType::String, value.data(), value.size());
    }

    bool SetBytes(const std::string& key, const std::vector<uint8_t>& value)
    {
        return Put(key, TacoType::Bytes, value.data(), value.size());
    }

    /// Returns false and leaves out unchanged if missing or of another kind.
    bool GetBool(const std::string& key, bool* out) const
    {
        const Entry* entry = Find(key, TacoType::Bool, 1);
        if (entry == nullptr)
            return false;
        *out = entry->value[0] != 0;
        return true;
    }

    bool GetInt(const std::string& key, int32_t* out) const
    {
        return GetPod(key, TacoType::Int, out);
    }

    bool GetInt64(const std::string& key, int64_t* out) const
    {
        return GetPod(key, TacoType::Int64, out);
    }

    bool GetDouble(const std::string& key, double* out) const
    {
        return GetPod(key, TacoType::Double, out);
    }

    bool GetString(const std::string& key, std::string* out) const
    {
        const Entry* entry = Find(key, TacoType::String, SIZE_MAX);
        if (entry == nullptr)
            return false;
        out->assign(entry->value.begin(), entry->value.end());
        return true;
    }

    bool GetBytes(const std::string& key, std::vector<uint8_t>* out) const
    {
        const Entry* entry = Find(key, TacoType::Bytes, SIZE_MAX);
        if (entry == nullptr)
            return false;
        *out = entry->value;
        return true;
    }

    bool Has(const std::string& key) const
    {
        return IndexOf(key) != SIZE_MAX;
    }

    bool Remove(const std::string& key)
    {
        const size_t index = IndexOf(key);
        if (index == SIZE_MAX)
            return false;
        entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    size_t Count() const
    {
        return entries_.size();
    }

    std::vector<std::string> Keys() const
    {
        std::vector<std::string> keys;
        for (const Entry& entry : entries_)
            keys.push_back(entry.key);
        return keys;
    }

private:
    friend struct detail::Access;

    struct Entry
    {
        std::string key;
        TacoType type;
        std::vector<uint8_t> value;
    };

    template <class T>
    bool GetPod(const std::string& key, TacoType type, T* out) const
    {
        const Entry* entry = Find(key, type, sizeof(T));
        if (entry == nullptr)
            return false;
        std::memcpy(out, entry->value.data(), sizeof(T));
        return true;
    }

    size_t IndexOf(const std::string& key) const
    {
        for (size_t i = 0; i < entries_.size(); ++i)
        {
            if (entries_[i].key == key)
                return i;
        }
        return SIZE_MAX;
    }

    // size == SIZE_MAX skips the length check (strings).
    const Entry* Find(const std::string& key, TacoType type, size_t size) const
    {
        const size_t index = IndexOf(key);
        if (index == SIZE_MAX)
            return nullptr;
        const Entry& entry = entries_[index];
        if (entry.type != type)
            return nullptr;
        if (size != SIZE_MAX && entry.value.size() != size)
            return nullptr;
        return &entry;
    }

    bool IsUsableKey(const std::string& key) const
    {
        if (key.empty() || key.size() > kMaxKeyLength || key[0] == '_')
            return false;
        return isSchemaKey_ == nullptr || !isSchemaKey_(key.c_str());
    }

    bool Put(const std::string& key, TacoType type, const void* data, size_t size)
    {
        if (!IsUsableKey(key))
            return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        const size_t index = IndexOf(key);
        if (index != SIZE_MAX)
        {
            entries_[index].type = type;
            entries_[index].value.assign(bytes, bytes + size);
            return true;
        }
        Entry entry;
        entry.key = key;
        entry.type = type;
        entry.value.assign(bytes, bytes + size);
        entries_.push_back(entry);
        return true;
    }

    std::vector<Entry> entries_;
    bool (*isSchemaKey_)(const char*) = nullptr;
};

namespace detail
{

template <class T>
inline void Store(uint8_t* dst, T value)
{
    std::memcpy(dst, &value, sizeof(T));
}

template <class T>
inline T Load(const uint8_t* src)
{
    T value;
    std::memcpy(&value, src, sizeof(T));
    return value;
}

inline void StoreBool(uint8_t* dst, bool value)
{
    *dst = value ? 1 : 0;
}

inline bool LoadBool(const uint8_t* src)
{
    return *src != 0;
}

// Fixed-length string. capacity bytes, zero padded. Cannot hold NUL.
inline std::string LoadChars(const uint8_t* src, size_t capacity)
{
    size_t length = 0;
    while (length < capacity && src[length] != 0)
        ++length;
    return std::string(reinterpret_cast<const char*>(src), length);
}

// Returns false and keeps the old value if too long (never truncates).
inline bool StoreChars(uint8_t* dst, size_t capacity, const std::string& value)
{
    if (value.size() > capacity)
        return false;
    std::memcpy(dst, value.data(), value.size());
    std::memset(dst + value.size(), 0, capacity - value.size());
    return true;
}

// Frame header: 'P' 'C' | frame version u8 | name length u8 | name | message version u16 | min version u16 |
//               id u32 | corr u32 | ts i64 | BodyLen u32 | schema hash u32 x (version - min version + 1)
// BodyLen detects truncated frames and trailing bytes, and marks frame boundaries on a stream.
inline size_t HeaderSize(size_t nameLength, size_t hashCount)
{
    return 28 + nameLength + 4 * hashCount;
}

inline void StoreBodySize(std::vector<uint8_t>& buf, size_t bodySize)
{
    Store<uint32_t>(buf.data() + 24 + buf[3], static_cast<uint32_t>(bodySize));
}

// Preallocates header + FixedLen + zeroed fixed block. Fixed field setters write straight into this buffer.
// hashes: fingerprints of versions minVersion..version.
inline void InitFrame(std::vector<uint8_t>& buf, const char* name, uint16_t version, uint16_t minVersion,
                      const uint32_t* hashes, uint32_t fixedSize)
{
    const size_t nameLength = std::strlen(name);
    const size_t hashCount = static_cast<size_t>(version - minVersion) + 1;
    const size_t headerSize = HeaderSize(nameLength, hashCount);
    buf.assign(headerSize + 4 + fixedSize, 0);
    uint8_t* p = buf.data();
    p[0] = 'P';
    p[1] = 'C';
    p[2] = static_cast<uint8_t>(kFrameVersion);
    p[3] = static_cast<uint8_t>(nameLength);
    std::memcpy(p + 4, name, nameLength);
    Store<uint16_t>(p + 4 + nameLength, version);
    Store<uint16_t>(p + 6 + nameLength, minVersion);
    StoreBodySize(buf, 4 + fixedSize);
    std::memcpy(p + 28 + nameLength, hashes, 4 * hashCount);
    Store<uint32_t>(p + headerSize, fixedSize);
}

inline void StoreEnvelope(std::vector<uint8_t>& buf, const Envelope& env)
{
    uint8_t* p = buf.data() + 8 + buf[3];
    Store<uint32_t>(p, env.id);
    Store<uint32_t>(p + 4, env.corr);
    Store<int64_t>(p + 8, env.ts);
}

inline Envelope LoadEnvelope(const uint8_t* frame)
{
    const uint8_t* p = frame + 8 + frame[3];
    Envelope env;
    env.id = Load<uint32_t>(p);
    env.corr = Load<uint32_t>(p + 4);
    env.ts = Load<int64_t>(p + 8);
    return env;
}

// Per-type flag as a runtime value. A constant condition is warning C4127, and C++14 has no if constexpr.
inline bool IsOn(uint32_t flag)
{
    return flag != 0;
}

inline bool InList(const char* key, const char* const* keys, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (std::strcmp(key, keys[i]) == 0)
            return true;
    }
    return false;
}

inline bool Fail(Error* error, const std::string& what)
{
    if (error != nullptr)
        error->what = what;
    return false;
}

inline std::string Hex32(uint32_t value)
{
    char text[11];
    std::snprintf(text, sizeof(text), "0x%08X", value);
    return text;
}

// yaml type word of a TacoType, for error text.
inline const char* TypeName(uint8_t type)
{
    static const char* const kNames[] = { "unknown", "bool", "int", "int64", "double", "string", "message", "list", "bytes" };
    return type < sizeof(kNames) / sizeof(kNames[0]) ? kNames[type] : "unknown";
}

// Prefixes the outer name to an inner error path: "from" -> "[2].from" -> "moves[2].from"
inline void PrependPath(Error* error, const std::string& segment)
{
    if (error == nullptr)
        return;
    if (error->path.empty())
        error->path = segment;
    else if (error->path[0] == '[')
        error->path = segment + error->path;
    else
        error->path = segment + "." + error->path;
}

// Sequential writes into a preallocated buffer. Sizes are computed beforehand.
class CWriter
{
public:
    explicit CWriter(uint8_t* dst)
        : p_(dst)
    {
    }

    void Bytes(const void* src, size_t size)
    {
        if (size != 0)
            std::memcpy(p_, src, size);
        p_ += size;
    }

    template <class T>
    void Pod(T value)
    {
        Store<T>(p_, value);
        p_ += sizeof(T);
    }

private:
    uint8_t* p_;
};

// Every read checks the remaining length. Corrupt or truncated input returns an error.
class CReader
{
public:
    CReader(const uint8_t* src, size_t size)
        : p_(src)
        , end_(src + size)
    {
    }

    size_t Left() const
    {
        return static_cast<size_t>(end_ - p_);
    }

    bool Take(size_t size, const uint8_t** out)
    {
        if (Left() < size)
            return false;
        *out = p_;
        p_ += size;
        return true;
    }

    template <class T>
    bool Pod(T* out)
    {
        const uint8_t* p = nullptr;
        if (!Take(sizeof(T), &p))
            return false;
        *out = Load<T>(p);
        return true;
    }

private:
    const uint8_t* p_;
    const uint8_t* end_;
};

// One variable block TLV: KeyLen u8 | Key | Type u8 | Len u32 | Value
inline size_t TlvSize(size_t keyLength, size_t valueSize)
{
    return 1 + keyLength + 1 + 4 + valueSize;
}

// View of a parsed TLV (no copy).
struct RawField
{
    const char* key;
    size_t keyLength;
    uint8_t type;
    uint32_t size;
    const uint8_t* value;
    bool used;
};

inline RawField* FindRaw(std::vector<RawField>& fields, const char* key, size_t keyLength)
{
    for (RawField& raw : fields)
    {
        if (raw.keyLength == keyLength && std::memcmp(raw.key, key, keyLength) == 0)
            return &raw;
    }
    return nullptr;
}

inline bool ParseVar(CReader& reader, std::vector<RawField>* fields, Error* error)
{
    while (reader.Left() != 0)
    {
        RawField raw = RawField();
        uint8_t keyLength = 0;
        const uint8_t* key = nullptr;
        if (!reader.Pod(&keyLength) || keyLength == 0 || !reader.Take(keyLength, &key))
            return Fail(error, "field key truncated");
        raw.key = reinterpret_cast<const char*>(key);
        raw.keyLength = keyLength;
        const std::string keyText(raw.key, raw.keyLength);
        if (!reader.Pod(&raw.type) || !reader.Pod(&raw.size) || !reader.Take(raw.size, &raw.value))
        {
            PrependPath(error, keyText);
            return Fail(error, "field value truncated");
        }
        if (FindRaw(*fields, raw.key, raw.keyLength) != nullptr)
        {
            PrependPath(error, keyText);
            return Fail(error, "duplicate key");
        }
        if (fields->size() >= kMaxFields)
            return Fail(error, "too many fields");
        fields->push_back(raw);
    }
    return true;
}

// Size, write and read per value kind. The primary template handles generated message types.
template <class T>
struct Codec;

class CTextWriter;

struct Access
{
    template <class T>
    static std::string ToText(const T& message, const TextStyle& style);

    template <class T>
    static void WriteText(const T& message, CTextWriter& writer);

    template <class T>
    static size_t BodySize(const T& message);

    template <class T>
    static void WriteBody(const T& message, CWriter& writer);

    // newer: the frame is of a newer version than T (verified by fingerprint). T reads its own prefix of it.
    template <class T>
    static bool ReadBody(T& message, const uint8_t* src, size_t size, Error* error, int depth, bool newer);

    template <class T>
    static const std::vector<uint8_t>& Serialize(const T& message);

    static void BindExt(CExt& ext, bool (*isSchemaKey)(const char*))
    {
        ext.isSchemaKey_ = isSchemaKey;
    }

private:
    template <class T>
    static size_t VarSize(const T& message);

    template <class T>
    static void WriteVar(const T& message, CWriter& writer);
};

template <class T>
struct Codec
{
    static TacoType Type()
    {
        return TacoType::Message;
    }

    static size_t Size(const T& value)
    {
        return Access::BodySize(value);
    }

    static void Write(CWriter& writer, const T& value)
    {
        Access::WriteBody(value, writer);
    }

    static bool Read(const uint8_t* src, size_t size, T* out, Error* error, int depth)
    {
        return Access::ReadBody(*out, src, size, error, depth + 1, false);
    }
};

// Fixed-size scalars. In the variable block they appear only as list elements.
template <class T, TacoType kType>
struct PodCodec
{
    static TacoType Type()
    {
        return kType;
    }

    static size_t Size(T)
    {
        return sizeof(T);
    }

    static void Write(CWriter& writer, T value)
    {
        writer.Pod<T>(value);
    }

    static bool Read(const uint8_t* src, size_t size, T* out, Error* error, int)
    {
        if (size != sizeof(T))
            return Fail(error, "scalar size mismatch");
        *out = Load<T>(src);
        return true;
    }
};

template <>
struct Codec<int32_t> : PodCodec<int32_t, TacoType::Int>
{
};

template <>
struct Codec<int64_t> : PodCodec<int64_t, TacoType::Int64>
{
};

template <>
struct Codec<double> : PodCodec<double, TacoType::Double>
{
};

template <>
struct Codec<std::string>
{
    static TacoType Type()
    {
        return TacoType::String;
    }

    static size_t Size(const std::string& value)
    {
        return value.size();
    }

    static void Write(CWriter& writer, const std::string& value)
    {
        writer.Bytes(value.data(), value.size());
    }

    static bool Read(const uint8_t* src, size_t size, std::string* out, Error*, int)
    {
        out->assign(reinterpret_cast<const char*>(src), size);
        return true;
    }
};

// bytes. Full specialization ahead of the list partial specialization below.
template <>
struct Codec<std::vector<uint8_t>>
{
    static TacoType Type()
    {
        return TacoType::Bytes;
    }

    static size_t Size(const std::vector<uint8_t>& value)
    {
        return value.size();
    }

    static void Write(CWriter& writer, const std::vector<uint8_t>& value)
    {
        writer.Bytes(value.data(), value.size());
    }

    static bool Read(const uint8_t* src, size_t size, std::vector<uint8_t>* out, Error*, int)
    {
        out->assign(src, src + size);
        return true;
    }
};

// list: ElemType u8 | Count u32 | (Len u32 | Value) x Count
template <class E>
struct Codec<std::vector<E>>
{
    static TacoType Type()
    {
        return TacoType::List;
    }

    static size_t Size(const std::vector<E>& list)
    {
        size_t size = 1 + 4;
        for (const E& item : list)
            size += 4 + Codec<E>::Size(item);
        return size;
    }

    static void Write(CWriter& writer, const std::vector<E>& list)
    {
        writer.Pod<uint8_t>(static_cast<uint8_t>(Codec<E>::Type()));
        writer.Pod<uint32_t>(static_cast<uint32_t>(list.size()));
        for (const E& item : list)
        {
            writer.Pod<uint32_t>(static_cast<uint32_t>(Codec<E>::Size(item)));
            Codec<E>::Write(writer, item);
        }
    }

    static bool Read(const uint8_t* src, size_t size, std::vector<E>* out, Error* error, int depth)
    {
        CReader reader(src, size);
        uint8_t elemType = 0;
        uint32_t count = 0;
        if (!reader.Pod(&elemType) || !reader.Pod(&count))
            return Fail(error, "list header truncated");
        if (elemType != static_cast<uint8_t>(Codec<E>::Type()))
            return Fail(error, "list element type mismatch");
        // Each element needs a 4-byte Len. Rejects huge counts.
        if (count > reader.Left() / 4)
            return Fail(error, "list count too large");
        out->clear();
        out->reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t itemSize = 0;
            const uint8_t* item = nullptr;
            out->emplace_back();
            if (!reader.Pod(&itemSize) || !reader.Take(itemSize, &item))
            {
                PrependPath(error, "[" + std::to_string(i) + "]");
                return Fail(error, "list element truncated");
            }
            if (!Codec<E>::Read(item, itemSize, &out->back(), error, depth))
            {
                PrependPath(error, "[" + std::to_string(i) + "]");
                return false;
            }
        }
        if (reader.Left() != 0)
            return Fail(error, "trailing bytes in list");
        return true;
    }
};

class CSizeVisitor
{
public:
    template <class T>
    void Field(const char* key, const T& value, bool)
    {
        size_ += TlvSize(std::strlen(key), Codec<T>::Size(value));
    }

    size_t Size() const
    {
        return size_;
    }

private:
    size_t size_ = 0;
};

class CWriteVisitor
{
public:
    explicit CWriteVisitor(CWriter& writer)
        : writer_(writer)
    {
    }

    template <class T>
    void Field(const char* key, const T& value, bool)
    {
        const size_t keyLength = std::strlen(key);
        writer_.Pod<uint8_t>(static_cast<uint8_t>(keyLength));
        writer_.Bytes(key, keyLength);
        writer_.Pod<uint8_t>(static_cast<uint8_t>(Codec<T>::Type()));
        writer_.Pod<uint32_t>(static_cast<uint32_t>(Codec<T>::Size(value)));
        Codec<T>::Write(writer_, value);
    }

private:
    CWriter& writer_;
};

class CReadVisitor
{
public:
    CReadVisitor(std::vector<RawField>& fields, Error* error, int depth)
        : fields_(fields)
        , error_(error)
        , depth_(depth)
    {
    }

    template <class T>
    void Field(const char* key, T& value, bool required)
    {
        if (failed_)
            return;
        RawField* raw = FindRaw(fields_, key, std::strlen(key));
        if (raw == nullptr)
        {
            // A missing optional keeps the constructor default.
            if (required)
                Stop(key, "missing required");
            return;
        }
        raw->used = true;
        if (raw->type != static_cast<uint8_t>(Codec<T>::Type()))
        {
            Stop(key, std::string("type mismatch: expected ") + TypeName(static_cast<uint8_t>(Codec<T>::Type())) + ", got " + TypeName(raw->type));
            return;
        }
        if (!Codec<T>::Read(raw->value, raw->size, &value, error_, depth_))
        {
            PrependPath(error_, key);
            failed_ = true;
        }
    }

    bool Failed() const
    {
        return failed_;
    }

private:
    void Stop(const char* key, const std::string& what)
    {
        Fail(error_, what);
        PrependPath(error_, key);
        failed_ = true;
    }

    std::vector<RawField>& fields_;
    Error* error_;
    int depth_;
    bool failed_ = false;
};

template <class T>
size_t Access::VarSize(const T& message)
{
    CSizeVisitor visitor;
    T::VisitVar(message, visitor);
    size_t size = visitor.Size();
    for (const CExt::Entry& entry : message.ext_.entries_)
        size += TlvSize(entry.key.size(), entry.value.size());
    return size;
}

template <class T>
void Access::WriteVar(const T& message, CWriter& writer)
{
    CWriteVisitor visitor(writer);
    T::VisitVar(message, visitor);
    for (const CExt::Entry& entry : message.ext_.entries_)
    {
        writer.Pod<uint8_t>(static_cast<uint8_t>(entry.key.size()));
        writer.Bytes(entry.key.data(), entry.key.size());
        writer.Pod<uint8_t>(static_cast<uint8_t>(entry.type));
        writer.Pod<uint32_t>(static_cast<uint32_t>(entry.value.size()));
        writer.Bytes(entry.value.data(), entry.value.size());
    }
}

template <class T>
size_t Access::BodySize(const T& message)
{
    return 4 + T::kFixedSize + VarSize(message);
}

// body: FixedLen u32 | fixed block | variable TLV... | ext TLV...
template <class T>
void Access::WriteBody(const T& message, CWriter& writer)
{
    writer.Pod<uint32_t>(T::kFixedSize);
    writer.Bytes(message.buf_.data() + T::kFixedOffset, T::kFixedSize);
    WriteVar(message, writer);
}

template <class T>
bool Access::ReadBody(T& message, const uint8_t* src, size_t size, Error* error, int depth, bool newer)
{
    if (depth > static_cast<int>(kMaxDepth))
        return Fail(error, "nesting too deep");
    CReader reader(src, size);
    uint32_t fixedLength = 0;
    const uint8_t* fixed = nullptr;
    if (!reader.Pod(&fixedLength) || !reader.Take(fixedLength, &fixed))
        return Fail(error, "fixed block truncated");
    // Same version means same fixed block size; a newer version only appends. Also guards nested bodies,
    // which carry no fingerprint of their own.
    const uint32_t expected = static_cast<uint32_t>(T::kFixedSize);
    if (newer ? fixedLength < expected : fixedLength != expected)
        return Fail(error, "fixed block size mismatch: expected " + std::to_string(expected) + ", got " + std::to_string(fixedLength));
    if (IsOn(expected))
        std::memcpy(message.buf_.data() + T::kFixedOffset, fixed, expected);
    std::vector<RawField> fields;
    if (!ParseVar(reader, &fields, error))
        return false;
    CReadVisitor visitor(fields, error, depth);
    T::VisitVar(message, visitor);
    if (visitor.Failed())
        return false;
    // Keys not in the schema are kept in ext and sent again as they are. allow_extension false rejects them,
    // except fields of a newer version, which are dropped.
    for (const RawField& raw : fields)
    {
        if (raw.used)
            continue;
        if (!IsOn(T::kAllowExtension))
        {
            if (newer)
                continue;
            Fail(error, "unknown key (allow_extension false)");
            PrependPath(error, std::string(raw.key, raw.keyLength));
            return false;
        }
        CExt::Entry entry;
        entry.key.assign(raw.key, raw.keyLength);
        entry.type = static_cast<TacoType>(raw.type);
        entry.value.assign(raw.value, raw.value + raw.size);
        message.ext_.entries_.push_back(entry);
    }
    return true;
}

// The fixed block is already in the buffer. With no variable fields and an empty ext, only BodyLen is updated (zero-cost serialize).
template <class T>
const std::vector<uint8_t>& Access::Serialize(const T& message)
{
    const size_t fixedEnd = static_cast<size_t>(T::kFixedOffset) + static_cast<size_t>(T::kFixedSize);
    const size_t varSize = VarSize(message);
    message.buf_.resize(fixedEnd + varSize);
    StoreBodySize(message.buf_, 4 + static_cast<size_t>(T::kFixedSize) + varSize);
    if (varSize != 0)
    {
        CWriter writer(message.buf_.data() + fixedEnd);
        WriteVar(message, writer);
    }
    return message.buf_;
}

// Text for ToString. One line by default; TextStyle::indent > 0 puts each item on its own line.
// Strings are quoted and escaped, long values are cut (TextStyle::maxText), ext entries go under ext{...}.
class CTextWriter
{
public:
    explicit CTextWriter(const TextStyle& style)
        : style_(style)
    {
    }

    const std::string& Text() const
    {
        return out_;
    }

    // Name{field=value, ...}
    template <class T>
    void Message(const T& message)
    {
        out_ += T::MsgName();
        Open(style_.open);
        Access::WriteText(message, *this);
        Close(style_.close);
    }

    template <class T>
    void Field(const char* key, const T& value)
    {
        Item();
        out_ += key;
        out_ += '=';
        Value(value);
    }

    void BeginExt()
    {
        Item();
        out_ += "ext";
        Open(style_.open);
    }

    void ExtEntry(const std::string& key, TacoType type, const std::vector<uint8_t>& value)
    {
        Item();
        out_ += key;
        out_ += '=';
        RawValue(type, value);
    }

    void EndExt()
    {
        Close(style_.close);
    }

private:
    void Value(bool value)
    {
        out_ += value ? "true" : "false";
    }

    void Value(int32_t value)
    {
        out_ += std::to_string(value);
    }

    void Value(int64_t value)
    {
        out_ += std::to_string(value);
    }

    void Value(double value)
    {
        if (std::isnan(value))
        {
            out_ += "nan";
            return;
        }
        if (std::isinf(value))
        {
            out_ += value < 0 ? "-inf" : "inf";
            return;
        }
        if (value == 0)
        {
            out_ += '0';   // -0 too
            return;
        }
        char text[40];
        std::snprintf(text, sizeof(text), "%.15g", value);
        out_ += text;
    }

    void Value(const std::string& value)
    {
        out_ += '"';
        const size_t shown = value.size() < style_.maxText ? value.size() : style_.maxText;
        for (size_t i = 0; i < shown; ++i)
            Escaped(static_cast<unsigned char>(value[i]));
        out_ += '"';
        if (shown < value.size())
            out_ += "...(+" + std::to_string(value.size() - shown) + ")";
    }

    void Value(const std::vector<uint8_t>& value)
    {
        Bytes(value.data(), value.size());
    }

    template <class E>
    void Value(const std::vector<E>& list)
    {
        Open("[");
        for (const E& item : list)
        {
            Item();
            Value(item);
        }
        Close("]");
    }

    template <class T>
    void Value(const T& message)
    {
        Message(message);
    }

    // ext values carry only their kind, not a schema.
    void RawValue(TacoType type, const std::vector<uint8_t>& value)
    {
        switch (type)
        {
        case TacoType::Bool:
            if (value.size() == 1)
                return Value(value[0] != 0);
            break;
        case TacoType::Int:
            if (value.size() == 4)
                return Value(Load<int32_t>(value.data()));
            break;
        case TacoType::Int64:
            if (value.size() == 8)
                return Value(Load<int64_t>(value.data()));
            break;
        case TacoType::Double:
            if (value.size() == 8)
                return Value(Load<double>(value.data()));
            break;
        case TacoType::String:
            return Value(std::string(value.begin(), value.end()));
        case TacoType::Bytes:
            return Bytes(value.data(), value.size());
        case TacoType::Message:
            out_ += "<message " + std::to_string(value.size()) + " bytes>";
            return;
        case TacoType::List:
            out_ += "<list " + std::to_string(value.size()) + " bytes>";
            return;
        default:
            out_ += "<type " + std::to_string(static_cast<int>(type)) + ", " + std::to_string(value.size()) + " bytes>";
            return;
        }
        out_ += "<bad value>";
    }

    void Bytes(const uint8_t* data, size_t size)
    {
        out_ += "<" + std::to_string(size) + " bytes";
        const size_t limit = style_.maxText < 8 ? style_.maxText : 8;
        const size_t shown = size < limit ? size : limit;
        static const char kHex[] = "0123456789ABCDEF";
        for (size_t i = 0; i < shown; ++i)
        {
            out_ += i == 0 ? ": " : " ";
            out_ += kHex[data[i] >> 4];
            out_ += kHex[data[i] & 0x0F];
        }
        if (shown < size)
            out_ += " ...";
        out_ += '>';
    }

    void Escaped(unsigned char c)
    {
        switch (c)
        {
        case '"':
            out_ += "\\\"";
            return;
        case '\\':
            out_ += "\\\\";
            return;
        case '\n':
            out_ += "\\n";
            return;
        case '\r':
            out_ += "\\r";
            return;
        case '\t':
            out_ += "\\t";
            return;
        default:
            break;
        }
        if (c < 0x20 || c == 0x7F)
        {
            static const char kHex[] = "0123456789ABCDEF";
            out_ += "\\x";
            out_ += kHex[c >> 4];
            out_ += kHex[c & 0x0F];
            return;
        }
        out_ += static_cast<char>(c);
    }

    void Open(const char* text)
    {
        out_ += text;
        first_.push_back(true);
    }

    void Close(const char* text)
    {
        const bool empty = first_.back();
        first_.pop_back();
        if (style_.indent > 0 && !empty)
            NewLine();
        out_ += text;
    }

    void Item()
    {
        if (!first_.back())
            out_ += style_.indent > 0 ? "," : style_.separator;
        first_.back() = false;
        if (style_.indent > 0)
            NewLine();
    }

    void NewLine()
    {
        out_ += '\n';
        out_.append(first_.size() * static_cast<size_t>(style_.indent), ' ');
    }

    TextStyle style_;
    std::string out_;
    std::vector<bool> first_;   // per open level: no item written yet
};

template <class T>
std::string Access::ToText(const T& message, const TextStyle& style)
{
    CTextWriter writer(style);
    writer.Message(message);
    return writer.Text();
}

template <class T>
void Access::WriteText(const T& message, CTextWriter& writer)
{
    T::VisitText(message, writer);
    if (message.ext_.entries_.empty())
        return;
    writer.BeginExt();
    for (const CExt::Entry& entry : message.ext_.entries_)
        writer.ExtEntry(entry.key, entry.type, entry.value);
    writer.EndExt();
}

}  // namespace detail

/// Parses the frame header only, not the body.
inline bool PeekHeader(const uint8_t* src, size_t size, Header* out, Error* error = nullptr)
{
    if (size < detail::HeaderSize(0, 0))
        return detail::Fail(error, "frame too short");
    if (src[0] != 'P' || src[1] != 'C')
        return detail::Fail(error, "bad magic");
    if (src[2] != kFrameVersion)
        return detail::Fail(error, "unknown frame version " + std::to_string(src[2]) + ", expected " + std::to_string(static_cast<unsigned>(kFrameVersion)));
    const size_t nameLength = src[3];
    if (size < detail::HeaderSize(nameLength, 0))
        return detail::Fail(error, "frame too short");
    const uint16_t version = detail::Load<uint16_t>(src + 4 + nameLength);
    const uint16_t minVersion = detail::Load<uint16_t>(src + 6 + nameLength);
    if (minVersion == 0 || minVersion > version || static_cast<uint32_t>(version - minVersion) >= kMaxSchemaVersions)
        return detail::Fail(error, "bad version range " + std::to_string(minVersion) + ".." + std::to_string(version));
    const size_t hashCount = static_cast<size_t>(version - minVersion) + 1;
    if (size < detail::HeaderSize(nameLength, hashCount))
        return detail::Fail(error, "frame too short");
    out->name.assign(reinterpret_cast<const char*>(src + 4), nameLength);
    out->version = version;
    out->minVersion = minVersion;
    out->env = detail::LoadEnvelope(src);
    out->bodySize = detail::Load<uint32_t>(src + 24 + nameLength);
    out->schemaHash = detail::Load<uint32_t>(src + 28 + nameLength + 4 * (hashCount - 1));
    return true;
}

/// Bytes to object without exceptions. On failure returns false with the error path; out is unchanged.
/// Accepts frames of T's version and newer: a newer frame is read as T's version (README.md "Version window").
/// Decodes into a temporary and then moves it, so src may point into out's own buffer.
template <class T>
bool TryToObject(const uint8_t* src, size_t size, T* out, Error* error = nullptr)
{
    try
    {
        if (error != nullptr)
            *error = Error();
        if (size > kMaxFrameBytes)
            return detail::Fail(error, "frame too large");
        Header header;
        if (!PeekHeader(src, size, &header, error))
            return false;
        const std::string name = T::MsgName();
        const uint16_t mine = T::MsgVersion();
        if (header.name != name)
            return detail::Fail(error, "message name mismatch: expected " + name + ", got " + header.name);
        if (header.version < mine)
        {
            std::string what = "message version too old: " + name + " reads version " + std::to_string(mine) + " or newer, got " + std::to_string(header.version);
            if (header.version >= T::MsgMinVersion())
                what += ". Use the v" + std::to_string(header.version) + " type";
            return detail::Fail(error, what);
        }
        if (header.minVersion > mine)
            return detail::Fail(error, "message version too new: " + name + " reads version " + std::to_string(mine) + ", sender supports "
                + std::to_string(header.minVersion) + ".." + std::to_string(header.version));
        const size_t nameLength = header.name.size();
        const uint32_t hash = detail::Load<uint32_t>(src + 28 + nameLength + 4 * static_cast<size_t>(mine - header.minVersion));
        if (hash != T::MsgSchemaHash())
            return detail::Fail(error, "schema mismatch: " + name + " version " + std::to_string(mine) + " expected " + detail::Hex32(T::MsgSchemaHash())
                + ", got " + detail::Hex32(hash));
        const size_t headerSize = detail::HeaderSize(nameLength, static_cast<size_t>(header.version - header.minVersion) + 1);
        if (size - headerSize != header.bodySize)
            return detail::Fail(error, size - headerSize < header.bodySize ? "frame truncated" : "trailing bytes after frame");
        T decoded;
        decoded.SetEnvelope(header.env);
        const bool newer = header.version > mine;
        if (!detail::Access::ReadBody(decoded, src + headerSize, header.bodySize, error, 0, newer))
        {
            if (newer && error != nullptr)
                error->what += " (frame version " + std::to_string(header.version) + " read as " + std::to_string(mine) + ")";
            return false;
        }
        *out = std::move(decoded);
        return true;
    }
    catch (const std::bad_alloc&)
    {
        return detail::Fail(error, "out of memory");
    }
}

/// Bytes to object. Throws CParseError on failure.
template <class T>
T ToObject(const uint8_t* src, size_t size)
{
    T object;
    Error error;
    if (!TryToObject(src, size, &object, &error))
        throw CParseError(error);
    return object;
}

template <class T>
T ToObject(const std::vector<uint8_t>& bytes)
{
    return ToObject<T>(bytes.data(), bytes.size());
}

}  // namespace protaco

#endif  // PROTACO_RUNTIME_0_4_0
