// Protaco.hpp. Header-only runtime for protacoc generated code (*.g.h).
// C++14 compatible. Only ToObject throws; every Try* call reports failure through its return value and an optional
// std::string* error. Format v4: README.md "Serialization format".
// protacoc.exe writes this file next to the generated code, so generator and runtime versions always match.
#pragma once
// Each output folder has its own copy. The same version is included once; mixing versions is a redefinition error.
#ifndef PROTACO_RUNTIME_0_6_0
#define PROTACO_RUNTIME_0_6_0

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
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
    UInt = 9,
    UInt64 = 10,
    Map = 11,
};

enum : uint32_t
{
    kFrameVersion = 4,
    kMaxDepth = 100,               // max nested message depth
    kMaxSchemaVersions = 255,      // max supported versions per frame (min version..version)
    kMaxFields = 1024,             // max variable fields per body
    kMaxFrameBytes = 0xFFFFFFFFu,  // max frame size (4 GiB - 1): every length in the format is u32
    kMaxKeyLength = 255,
};

/// Message envelope, stored in the frame header. id: new id per message (NextId), corr: id of the request this answers
/// (set by the user), ts: UTC microseconds since 1970-01-01 (NowUs). id and ts are set when a message object is created.
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
    uint16_t minVersion = 0;   // oldest version the sender supports; the frame has a fingerprint per version
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
    bool envelope = false;          // show envelope{id, corr, ts} of the top-level message first

    // One field per line, envelope included.
    static TextStyle Pretty(int indentWidth = 2)
    {
        TextStyle style;
        style.indent = indentWidth;
        style.envelope = true;
        return style;
    }
};

/// Decode error carried by CParseError. path looks like "sections[0].name". Try* functions give the same text as one string.
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

namespace detail
{
// yaml type word of a TacoType, for error text.
inline const char* TypeName(uint8_t type)
{
    static const char* const kNames[] = { "unknown", "bool", "int", "int64", "double", "string", "message", "list", "bytes", "uint", "uint64", "map" };
    return type < sizeof(kNames) / sizeof(kNames[0]) ? kNames[type] : "unknown";
}

inline bool SetError(std::string* error, const std::string& text)
{
    if (error != nullptr)
        *error = text;
    return false;
}
}  // namespace detail

/// Extra key-value data not in the yaml. Receivers keep unknown keys here and send them on.
/// Value types: bool, int32_t, int64_t, uint32_t, uint64_t, double, std::string (also const char*), std::vector<uint8_t> (bytes).
/// Keys: 1..255 bytes, not starting with '_', not a field key of the message (any version).
/// Every call returns false on failure and, when error is not null, writes the reason there. Nothing throws.
class CExt
{
public:
    CExt() = default;
    CExt(const CExt&) = default;
    CExt(CExt&&) = default;

    // Keeps this object's schema key check. Counts as a change for the Serialize() reuse.
    CExt& operator=(const CExt& other)
    {
        entries_ = other.entries_;
        ++revision_;
        return *this;
    }

    CExt& operator=(CExt&& other) noexcept
    {
        entries_ = std::move(other.entries_);
        ++revision_;
        return *this;
    }

    bool SetValue(const std::string& key, bool value, std::string* error = nullptr)
    {
        const uint8_t raw = value ? 1 : 0;
        return Put(key, TacoType::Bool, &raw, sizeof(raw), error);
    }

    bool SetValue(const std::string& key, int32_t value, std::string* error = nullptr)
    {
        return Put(key, TacoType::Int, &value, sizeof(value), error);
    }

    bool SetValue(const std::string& key, int64_t value, std::string* error = nullptr)
    {
        return Put(key, TacoType::Int64, &value, sizeof(value), error);
    }

    bool SetValue(const std::string& key, uint32_t value, std::string* error = nullptr)
    {
        return Put(key, TacoType::UInt, &value, sizeof(value), error);
    }

    bool SetValue(const std::string& key, uint64_t value, std::string* error = nullptr)
    {
        return Put(key, TacoType::UInt64, &value, sizeof(value), error);
    }

    bool SetValue(const std::string& key, double value, std::string* error = nullptr)
    {
        return Put(key, TacoType::Double, &value, sizeof(value), error);
    }

    bool SetValue(const std::string& key, const std::string& value, std::string* error = nullptr)
    {
        return Put(key, TacoType::String, value.data(), value.size(), error);
    }

    bool SetValue(const std::string& key, const char* value, std::string* error = nullptr)
    {
        return SetValue(key, std::string(value != nullptr ? value : ""), error);
    }

    bool SetValue(const std::string& key, const std::vector<uint8_t>& value, std::string* error = nullptr)
    {
        return Put(key, TacoType::Bytes, value.data(), value.size(), error);
    }

    // Other types (long long on Linux, char, float, ...) are not stored implicitly: cast to one of the types above.
    template <class T>
    bool SetValue(const std::string& key, T value, std::string* error = nullptr) = delete;

    /// Returns false and leaves out unchanged if the key is missing or holds another type.
    bool TryGetValue(const std::string& key, bool* out, std::string* error = nullptr) const
    {
        const Entry* entry = Find(key, TacoType::Bool, 1, error);
        if (entry == nullptr)
            return false;
        *out = entry->value[0] != 0;
        return true;
    }

    bool TryGetValue(const std::string& key, int32_t* out, std::string* error = nullptr) const
    {
        return GetPod(key, TacoType::Int, out, error);
    }

    bool TryGetValue(const std::string& key, int64_t* out, std::string* error = nullptr) const
    {
        return GetPod(key, TacoType::Int64, out, error);
    }

    bool TryGetValue(const std::string& key, uint32_t* out, std::string* error = nullptr) const
    {
        return GetPod(key, TacoType::UInt, out, error);
    }

    bool TryGetValue(const std::string& key, uint64_t* out, std::string* error = nullptr) const
    {
        return GetPod(key, TacoType::UInt64, out, error);
    }

    bool TryGetValue(const std::string& key, double* out, std::string* error = nullptr) const
    {
        return GetPod(key, TacoType::Double, out, error);
    }

    bool TryGetValue(const std::string& key, std::string* out, std::string* error = nullptr) const
    {
        const Entry* entry = Find(key, TacoType::String, SIZE_MAX, error);
        if (entry == nullptr)
            return false;
        out->assign(entry->value.begin(), entry->value.end());
        return true;
    }

    bool TryGetValue(const std::string& key, std::vector<uint8_t>* out, std::string* error = nullptr) const
    {
        const Entry* entry = Find(key, TacoType::Bytes, SIZE_MAX, error);
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
        ++revision_;
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
    bool GetPod(const std::string& key, TacoType type, T* out, std::string* error) const
    {
        const Entry* entry = Find(key, type, sizeof(T), error);
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

    // size == SIZE_MAX skips the length check (strings, bytes).
    const Entry* Find(const std::string& key, TacoType type, size_t size, std::string* error) const
    {
        const size_t index = IndexOf(key);
        if (index == SIZE_MAX)
        {
            detail::SetError(error, "ext key not found: " + key);
            return nullptr;
        }
        const Entry& entry = entries_[index];
        if (entry.type != type)
        {
            detail::SetError(error, "ext key " + key + " holds " + detail::TypeName(static_cast<uint8_t>(entry.type)) + ", not "
                + detail::TypeName(static_cast<uint8_t>(type)));
            return nullptr;
        }
        if (size != SIZE_MAX && entry.value.size() != size)
        {
            detail::SetError(error, "ext key " + key + " has a bad " + detail::TypeName(static_cast<uint8_t>(type)) + " value");
            return nullptr;
        }
        return &entry;
    }

    bool CheckKey(const std::string& key, std::string* error) const
    {
        if (key.empty())
            return detail::SetError(error, "ext key is empty");
        if (key.size() > kMaxKeyLength)
            return detail::SetError(error, "ext key is longer than 255 bytes: " + key.substr(0, 32) + "...");
        if (key[0] == '_')
            return detail::SetError(error, "ext key starts with '_' (reserved): " + key);
        if (isSchemaKey_ != nullptr && isSchemaKey_(key.c_str()))
            return detail::SetError(error, "ext key is a field of the message: " + key);
        return true;
    }

    bool Put(const std::string& key, TacoType type, const void* data, size_t size, std::string* error)
    {
        if (!CheckKey(key, error))
            return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        const size_t index = IndexOf(key);
        ++revision_;
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
    uint32_t revision_ = 0;   // changes on every SetValue, Remove and assignment
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

// Returns false and keeps the old value if too long (never truncates). key names the field in the error text.
inline bool StoreChars(uint8_t* dst, size_t capacity, const std::string& value, const char* key, std::string* error)
{
    if (value.size() > capacity)
        return SetError(error, std::string(key) + ": " + std::to_string(value.size()) + " bytes, longer than string(" + std::to_string(capacity) + ")");
    std::memcpy(dst, value.data(), value.size());
    std::memset(dst + value.size(), 0, capacity - value.size());
    return true;
}

// Element index of a fixed array getter: asserted in debug builds, unchecked otherwise (like std::array::operator[]).
inline size_t AssertIndex(size_t index, size_t size)
{
    assert(index < size && "fixed array index out of range");
    (void)size;
    return index;
}

// Element setters of fixed arrays: an index out of range is an error, not a write.
inline bool IndexError(std::string* error, const char* key, size_t index, size_t size)
{
    return SetError(error, std::string(key) + "[" + std::to_string(index) + "]: index out of range (size " + std::to_string(size) + ")");
}

// string(N) element of a fixed array. The error path names the element: key[index].
inline bool StoreCharsAt(uint8_t* dst, size_t capacity, const std::string& value, const char* key, size_t index, std::string* error)
{
    if (value.size() > capacity)
        return SetError(error, std::string(key) + "[" + std::to_string(index) + "]: " + std::to_string(value.size()) + " bytes, longer than string("
            + std::to_string(capacity) + ")");
    return StoreChars(dst, capacity, value, key, nullptr);
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

// Envelope fields inside the frame header: id at +0, corr at +4, ts at +8.
inline uint8_t* EnvelopeAt(std::vector<uint8_t>& buf)
{
    return buf.data() + 8 + buf[3];
}

// Constructor tag for objects that must not take an envelope id or time: decoded messages and nested fields.
struct NoStampTag
{
};

// Constructor tag: a received frame of exactly the type's layout becomes the object's buffer.
struct AdoptTag
{
};

inline int64_t SystemClockUs()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

using ClockFn = int64_t (*)();

// Per module (each DLL or executable has its own copy).
inline ClockFn& ClockSource()
{
    static ClockFn source = &SystemClockUs;
    return source;
}

inline std::atomic<uint32_t>& IdCounter()
{
    static std::atomic<uint32_t> counter(0);
    return counter;
}

inline std::atomic<bool>& AutoEnvelope()
{
    static std::atomic<bool> on(true);
    return on;
}
}  // namespace detail

/// New message objects of this module (DLL or executable) keep id 0 and ts 0, as with auto_envelope false. Thread-safe.
inline void DisableAutoEnvelope()
{
    detail::AutoEnvelope().store(false, std::memory_order_relaxed);
}

/// Default: a new message object gets id = NextId() and ts = NowUs(), unless its yaml has auto_envelope false.
inline void EnableAutoEnvelope()
{
    detail::AutoEnvelope().store(true, std::memory_order_relaxed);
}

/// Next envelope id of this module (DLL or executable): 1, 2, 3, ... and 0 is skipped after wrap-around. Thread-safe.
inline uint32_t NextId()
{
    uint32_t id = 0;
    do
        id = ++detail::IdCounter();
    while (id == 0);
    return id;
}

/// Current time in UTC microseconds since 1970-01-01 from the clock set by SetClock (default: the system clock).
inline int64_t NowUs()
{
    return detail::ClockSource()();
}

/// Replaces the time source of NowUs, for example with an injected or simulated clock. nullptr restores the system clock.
/// Set it before creating messages; it is not synchronized with other threads.
inline void SetClock(int64_t (*nowUs)())
{
    detail::ClockSource() = nowUs != nullptr ? nowUs : &detail::SystemClockUs;
}

enum : int64_t
{
    kTimestampMin = 946684800000000LL,      // 2000-01-01 00:00:00 UTC; smaller values are not treated as times
    kTimestampMax = 253402300799999999LL,   // 9999-12-31 23:59:59.999999 UTC
};

/// Timestamp (UTC microseconds) to local time. Values outside 2000-01-01..9999-12-31 are not times: returns false.
inline bool TryGetTime(int64_t timestampUs, int* year, int* month, int* day, int* hour, int* minute, int* second, int* millisecond,
                       int* microsecond, std::string* error = nullptr)
{
    if (timestampUs < kTimestampMin || timestampUs > kTimestampMax)
        return detail::SetError(error, "timestamp " + std::to_string(timestampUs) + " is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)");
    const std::time_t seconds = static_cast<std::time_t>(timestampUs / 1000000);
    std::tm local = std::tm();
#ifdef _WIN32
    if (localtime_s(&local, &seconds) != 0)
#else
    if (localtime_r(&seconds, &local) == nullptr)
#endif
        return detail::SetError(error, "timestamp " + std::to_string(timestampUs) + " has no local time");
    const int fraction = static_cast<int>(timestampUs % 1000000);
    *year = local.tm_year + 1900;
    *month = local.tm_mon + 1;
    *day = local.tm_mday;
    *hour = local.tm_hour;
    *minute = local.tm_min;
    *second = local.tm_sec;
    *millisecond = fraction / 1000;
    *microsecond = fraction % 1000;
    return true;
}

/// Timestamp to local "YYYY-MM-DD" and "hh:mm:ss.ffffff".
inline bool TryGetTime(int64_t timestampUs, std::string* date, std::string* time, std::string* error = nullptr)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millisecond = 0;
    int microsecond = 0;
    if (!TryGetTime(timestampUs, &year, &month, &day, &hour, &minute, &second, &millisecond, &microsecond, error))
        return false;
    char text[32];
    std::snprintf(text, sizeof(text), "%04d-%02d-%02d", year, month, day);
    *date = text;
    std::snprintf(text, sizeof(text), "%02d:%02d:%02d.%06d", hour, minute, second, millisecond * 1000 + microsecond);
    *time = text;
    return true;
}

namespace detail
{
// New message objects: id from NextId, ts from NowUs. Nothing after DisableAutoEnvelope.
inline void StampEnvelope(std::vector<uint8_t>& buf)
{
    if (!AutoEnvelope().load(std::memory_order_relaxed))
        return;
    uint8_t* p = EnvelopeAt(buf);
    Store<uint32_t>(p, NextId());
    Store<int64_t>(p + 8, NowUs());
}

// Serialize() reuse of the variable block. set_ of a variable field clears valid. mutable_ sets exposed for
// the object's lifetime: writes through the returned reference cannot be seen, so the block is rebuilt every time.
// ext changes are seen through CExt::revision_. Fixed fields and the envelope live in the buffer and need no tracking.
struct VarCache
{
    bool valid = false;
    bool exposed = false;
    uint32_t extRevision = 0;

    VarCache() = default;

    // A copy rebuilds once. Assignment keeps exposed: references into the assigned object stay alive.
    // noexcept keeps generated messages nothrow-movable, so std::vector growth moves them instead of copying.
    VarCache(const VarCache&) noexcept
    {
    }

    VarCache& operator=(const VarCache&) noexcept
    {
        valid = false;
        return *this;
    }
};

// A field of the message's own type: absent until set, nothing in the serialized data while absent. Copies deep.
template <class T>
class Box
{
public:
    Box() = default;

    Box(const Box& other)
        : value_(other.value_ ? new T(*other.value_) : nullptr)
    {
    }

    Box(Box&&) noexcept = default;

    Box& operator=(const Box& other)
    {
        if (this != &other)
            value_.reset(other.value_ ? new T(*other.value_) : nullptr);
        return *this;
    }

    Box& operator=(Box&&) noexcept = default;

    bool Has() const
    {
        return value_ != nullptr;
    }

    // Absent: a shared empty object.
    const T& Get() const
    {
        if (value_)
            return *value_;
        static const T kEmpty{ NoStampTag() };
        return kEmpty;
    }

    T& Mutable()
    {
        if (!value_)
            value_.reset(new T(NoStampTag()));
        return *value_;
    }

    // Copies first, so setting a node into its own field works.
    void Set(const T& value)
    {
        value_.reset(new T(value));
    }

    void Clear()
    {
        value_.reset();
    }

private:
    std::unique_ptr<T> value_;
};

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

// Parsed TLVs of one body. The first kLocal live on the stack, so a typical decode allocates nothing here.
class CRawFields
{
public:
    size_t Size() const
    {
        return count_;
    }

    RawField* begin()
    {
        return count_ <= kLocal ? local_ : heap_.data();
    }

    RawField* end()
    {
        return begin() + count_;
    }

    void Add(const RawField& raw)
    {
        if (count_ < kLocal)
        {
            local_[count_] = raw;
        }
        else
        {
            if (count_ == kLocal)
                heap_.assign(local_, local_ + kLocal);
            heap_.push_back(raw);
        }
        ++count_;
    }

private:
    enum : size_t
    {
        kLocal = 16,
    };

    RawField local_[kLocal];
    std::vector<RawField> heap_;
    size_t count_ = 0;
};

inline RawField* FindRaw(CRawFields& fields, const char* key, size_t keyLength)
{
    for (RawField& raw : fields)
    {
        if (raw.keyLength == keyLength && std::memcmp(raw.key, key, keyLength) == 0)
            return &raw;
    }
    return nullptr;
}

inline bool ParseVar(CReader& reader, CRawFields* fields, Error* error)
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
        if (!reader.Pod(&raw.type) || !reader.Pod(&raw.size) || !reader.Take(raw.size, &raw.value))
        {
            PrependPath(error, std::string(raw.key, raw.keyLength));
            return Fail(error, "field value truncated");
        }
        if (FindRaw(*fields, raw.key, raw.keyLength) != nullptr)
        {
            PrependPath(error, std::string(raw.key, raw.keyLength));
            return Fail(error, "duplicate key");
        }
        if (fields->Size() >= kMaxFields)
            return Fail(error, "too many fields");
        fields->Add(raw);
    }
    return true;
}

// FixedLen u32 | fixed block. Same version means same size; a newer version only appends. Also guards nested bodies,
// which carry no fingerprint of their own.
inline bool TakeFixed(CReader& reader, uint32_t expected, bool newer, const uint8_t** fixed, Error* error)
{
    uint32_t fixedLength = 0;
    if (!reader.Pod(&fixedLength) || !reader.Take(fixedLength, fixed))
        return Fail(error, "fixed block truncated");
    if (newer ? fixedLength < expected : fixedLength != expected)
        return Fail(error, "fixed block size mismatch: expected " + std::to_string(expected) + ", got " + std::to_string(fixedLength));
    return true;
}

// Frame header read in place. name points into the frame.
struct FrameInfo
{
    const char* name = nullptr;
    size_t nameLength = 0;
    uint16_t version = 0;
    uint16_t minVersion = 0;
    size_t hashCount = 0;
    size_t headerSize = 0;
    uint32_t bodySize = 0;
};

// Size, write and read per value kind. The primary template handles generated message types.
template <class T, class Enable = void>
struct Codec;

class CTextWriter;

// Base of generated ConstRef classes (fixed: true messages): a fixed block inside another message's bytes.
struct BlockRef
{
};

// An array of fixed messages, for text: N blocks of Size bytes read through R (a generated ConstRef).
template <class R, size_t N, size_t Size>
struct RefArray
{
    const uint8_t* p;
};

struct Access
{
    template <class T>
    static std::string ToText(const T& message, const TextStyle& style);

    template <class T>
    static void WriteText(const T& message, CTextWriter& writer);

    // ConstRef text: the fields only (no envelope, no ext).
    template <class R>
    static std::string RefText(const R& ref, const TextStyle& style);

    template <class R>
    static void WriteRefText(const R& ref, CTextWriter& writer)
    {
        R::VisitText(ref, writer);
    }

    // Inline fixed message: the default fixed block of T, built once, copied into a new parent.
    template <class T>
    static void InitBlock(uint8_t* dst)
    {
        static const T defaults{NoStampTag()};
        std::memcpy(dst, defaults.buf_.data() + T::kFixedOffset, T::kFixedSize);
    }

    // Inline fixed message: the fields of value copied into dst.
    template <class T>
    static void CopyBlock(uint8_t* dst, const T& value)
    {
        std::memcpy(dst, value.buf_.data() + T::kFixedOffset, T::kFixedSize);
    }

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

    // Same version range and fingerprints as T: the frame header is the one T would write, envelope aside.
    template <class T>
    static bool SameLayout(const uint8_t* frame, const FrameInfo& info);

    template <class T>
    static T Adopted(std::vector<uint8_t>&& frame)
    {
        return T(AdoptTag(), std::move(frame));
    }

    // Reads the variable fields of an adopted frame. The frame then is the serialized form: Serialize() returns it.
    template <class T>
    static bool ReadAdopted(T& message, const FrameInfo& info, Error* error);

    template <class T>
    static void MarkSerialized(const T& message)
    {
        message.cache_.valid = true;
        message.cache_.extRevision = message.ext_.revision_;
    }

    template <class T>
    static bool ViewOf(const uint8_t* src, size_t size, typename T::View* out, Error* error);

private:
    template <class T>
    static size_t VarSize(const T& message);

    template <class T>
    static void WriteVar(const T& message, CWriter& writer);
};

template <class T, class Enable>
struct Codec
{
    static TacoType Type()
    {
        return TacoType::Message;
    }

    static T Make()
    {
        return T(NoStampTag());
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

    static T Make()
    {
        return T();
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

// Enums (generated .enum.h): int32 in the serialized data. Numbers this build does not know are kept as they are.
template <class T>
struct Codec<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
    static TacoType Type()
    {
        return TacoType::Int;
    }

    static T Make()
    {
        return T();
    }

    static size_t Size(T)
    {
        return sizeof(int32_t);
    }

    static void Write(CWriter& writer, T value)
    {
        writer.Pod<int32_t>(static_cast<int32_t>(value));
    }

    static bool Read(const uint8_t* src, size_t size, T* out, Error* error, int)
    {
        if (size != sizeof(int32_t))
            return Fail(error, "scalar size mismatch");
        *out = static_cast<T>(Load<int32_t>(src));
        return true;
    }
};

template <>
struct Codec<int64_t> : PodCodec<int64_t, TacoType::Int64>
{
};

template <>
struct Codec<uint32_t> : PodCodec<uint32_t, TacoType::UInt>
{
};

template <>
struct Codec<uint64_t> : PodCodec<uint64_t, TacoType::UInt64>
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

    static std::string Make()
    {
        return std::string();
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

    static std::vector<uint8_t> Make()
    {
        return std::vector<uint8_t>();
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

    static std::vector<E> Make()
    {
        return std::vector<E>();
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
            out->push_back(Codec<E>::Make());
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

// bool as a map value: 1 byte, 0 or 1. Fixed bool fields do not go through codecs.
template <>
struct Codec<bool>
{
    static TacoType Type()
    {
        return TacoType::Bool;
    }

    static bool Make()
    {
        return false;
    }

    static size_t Size(bool)
    {
        return 1;
    }

    static void Write(CWriter& writer, bool value)
    {
        writer.Pod<uint8_t>(value ? 1 : 0);
    }

    static bool Read(const uint8_t* src, size_t size, bool* out, Error* error, int)
    {
        if (size != 1)
            return Fail(error, "scalar size mismatch");
        *out = src[0] != 0;
        return true;
    }
};

// map: KeyType u8 | ValueType u8 | Count u32 | (KeyLen u32 | Key | ValueLen u32 | Value) x Count.
// Entries are written in the container's iteration order: two maps with the same entries may differ in bytes.
template <class K, class V>
struct Codec<std::unordered_map<K, V>>
{
    using Map = std::unordered_map<K, V>;

    static TacoType Type()
    {
        return TacoType::Map;
    }

    static Map Make()
    {
        return Map();
    }

    static size_t Size(const Map& map)
    {
        size_t size = 1 + 1 + 4;
        for (const auto& entry : map)
            size += 4 + Codec<K>::Size(entry.first) + 4 + Codec<V>::Size(entry.second);
        return size;
    }

    static void Write(CWriter& writer, const Map& map)
    {
        writer.Pod<uint8_t>(static_cast<uint8_t>(Codec<K>::Type()));
        writer.Pod<uint8_t>(static_cast<uint8_t>(Codec<V>::Type()));
        writer.Pod<uint32_t>(static_cast<uint32_t>(map.size()));
        for (const auto& entry : map)
        {
            writer.Pod<uint32_t>(static_cast<uint32_t>(Codec<K>::Size(entry.first)));
            Codec<K>::Write(writer, entry.first);
            writer.Pod<uint32_t>(static_cast<uint32_t>(Codec<V>::Size(entry.second)));
            Codec<V>::Write(writer, entry.second);
        }
    }

    static bool Read(const uint8_t* src, size_t size, Map* out, Error* error, int depth)
    {
        CReader reader(src, size);
        uint8_t keyType = 0;
        uint8_t valueType = 0;
        uint32_t count = 0;
        if (!reader.Pod(&keyType) || !reader.Pod(&valueType) || !reader.Pod(&count))
            return Fail(error, "map header truncated");
        if (keyType != static_cast<uint8_t>(Codec<K>::Type()) || valueType != static_cast<uint8_t>(Codec<V>::Type()))
            return Fail(error, "map key or value type mismatch");
        // Each entry needs two 4-byte lengths. Rejects huge counts.
        if (count > reader.Left() / 8)
            return Fail(error, "map count too large");
        out->clear();
        out->reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::string at = "[" + std::to_string(i) + "]";
            K key = Codec<K>::Make();
            V value = Codec<V>::Make();
            uint32_t itemSize = 0;
            const uint8_t* item = nullptr;
            if (!reader.Pod(&itemSize) || !reader.Take(itemSize, &item) || !Codec<K>::Read(item, itemSize, &key, error, depth))
            {
                PrependPath(error, at);
                return Fail(error, "map key truncated or bad");
            }
            if (!reader.Pod(&itemSize) || !reader.Take(itemSize, &item))
            {
                PrependPath(error, at);
                return Fail(error, "map value truncated");
            }
            if (!Codec<V>::Read(item, itemSize, &value, error, depth))
            {
                PrependPath(error, at);
                return false;
            }
            if (!out->emplace(std::move(key), std::move(value)).second)
            {
                PrependPath(error, at);
                return Fail(error, "duplicate map key");
            }
        }
        if (reader.Left() != 0)
            return Fail(error, "trailing bytes in map");
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

    template <class T>
    void Field(const char* key, const Box<T>& value, bool)
    {
        if (value.Has())
            Field(key, value.Get(), false);
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

    template <class T>
    void Field(const char* key, const Box<T>& value, bool)
    {
        if (value.Has())
            Field(key, value.Get(), false);
    }

private:
    CWriter& writer_;
};

class CReadVisitor
{
public:
    CReadVisitor(CRawFields& fields, Error* error, int depth)
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

    // Absent in the data: stays absent.
    template <class T>
    void Field(const char* key, Box<T>& value, bool)
    {
        if (!failed_ && FindRaw(fields_, key, std::strlen(key)) != nullptr)
            Field(key, value.Mutable(), false);
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

    CRawFields& fields_;
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
    const uint8_t* fixed = nullptr;
    if (!TakeFixed(reader, T::kFixedSize, newer, &fixed, error))
        return false;
    // An adopted frame already is the buffer.
    if (IsOn(T::kFixedSize) && fixed != message.buf_.data() + T::kFixedOffset)
        std::memcpy(message.buf_.data() + T::kFixedOffset, fixed, T::kFixedSize);
    CRawFields fields;
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

// The fixed block is already in the buffer. The variable block is rebuilt only after a variable field or ext changed
// (VarCache). With no variable fields and an empty ext, there is nothing to append (zero-cost serialize).
template <class T>
const std::vector<uint8_t>& Access::Serialize(const T& message)
{
    VarCache& cache = message.cache_;
    if (cache.valid && !cache.exposed && cache.extRevision == message.ext_.revision_)
        return message.buf_;
    const size_t fixedEnd = static_cast<size_t>(T::kFixedOffset) + static_cast<size_t>(T::kFixedSize);
    const size_t varSize = VarSize(message);
    message.buf_.resize(fixedEnd + varSize);
    StoreBodySize(message.buf_, 4 + static_cast<size_t>(T::kFixedSize) + varSize);
    if (varSize != 0)
    {
        CWriter writer(message.buf_.data() + fixedEnd);
        WriteVar(message, writer);
    }
    cache.valid = true;
    cache.extRevision = message.ext_.revision_;
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

    // Name{field=value, ...}. A ConstRef (inline fixed message) has fields only.
    template <class T>
    void Message(const T& message)
    {
        out_ += T::MsgName();
        Open(style_.open);
        Fields(message, std::is_base_of<BlockRef, T>());
        Close(style_.close);
    }

    // Top-level message: Name{envelope{...}, field=value, ...} when TextStyle::envelope is set. Nested bodies have no envelope.
    template <class T>
    void TopMessage(const T& message)
    {
        out_ += T::MsgName();
        Open(style_.open);
        if (style_.envelope)
            EnvelopeItem(message.GetEnvelope());
        Access::WriteText(message, *this);
        Close(style_.close);
    }

    // envelope{id=7, corr=0, ts=2026-10-20 14:03:22.123456}. ts outside the time range prints as the integer.
    void EnvelopeItem(const Envelope& env)
    {
        Item();
        out_ += "envelope";
        out_ += style_.open;
        out_ += "id=" + std::to_string(env.id) + style_.separator + "corr=" + std::to_string(env.corr) + style_.separator + "ts=";
        std::string date;
        std::string time;
        if (TryGetTime(env.ts, &date, &time))
            out_ += date + " " + time;
        else
            out_ += std::to_string(env.ts);
        out_ += style_.close;
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

    void Value(uint32_t value)
    {
        out_ += std::to_string(value);
    }

    void Value(uint64_t value)
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

    template <class T>
    void Value(const Box<T>& box)
    {
        if (box.Has())
            Value(box.Get());
        else
            out_ += "null";
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

    // Fixed-length array: the same text as a list.
    template <class E, size_t N>
    void Value(const std::array<E, N>& items)
    {
        Open("[");
        for (const E& item : items)
        {
            Item();
            Value(item);
        }
        Close("]");
    }

    template <class T>
    void Value(const T& value)
    {
        ValueOf(value, std::is_enum<T>());
    }

    // {key: value, ...} with keys sorted, so the text does not depend on the map's iteration order.
    template <class K, class V>
    void Value(const std::unordered_map<K, V>& map)
    {
        std::vector<const std::pair<const K, V>*> entries;
        entries.reserve(map.size());
        for (const auto& entry : map)
            entries.push_back(&entry);
        std::sort(entries.begin(), entries.end(), [](const std::pair<const K, V>* a, const std::pair<const K, V>* b) { return a->first < b->first; });
        Open("{");
        for (const std::pair<const K, V>* entry : entries)
        {
            Item();
            Value(entry->first);
            out_ += ": ";
            Value(entry->second);
        }
        Close("}");
    }

    template <class T>
    void ValueOf(const T& message, std::false_type)
    {
        Message(message);
    }

    template <class T>
    void Fields(const T& message, std::false_type)
    {
        Access::WriteText(message, *this);
    }

    template <class R>
    void Fields(const R& ref, std::true_type)
    {
        Access::WriteRefText(ref, *this);
    }

    // Array of fixed messages: the same text as a list of messages.
    template <class R, size_t N, size_t Size>
    void Value(const RefArray<R, N, Size>& items)
    {
        Open("[");
        for (size_t i = 0; i < N; ++i)
        {
            Item();
            Message(R(items.p + Size * i));
        }
        Close("]");
    }

    // Enum: Name(number), ?(number) for a number this build does not know. EnumName comes from the generated .enum.h.
    template <class T>
    void ValueOf(T value, std::true_type)
    {
        const char* name = EnumName(value);
        out_ += name != nullptr ? name : "?";
        out_ += "(" + std::to_string(static_cast<int32_t>(value)) + ")";
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
        case TacoType::UInt:
            if (value.size() == 4)
                return Value(Load<uint32_t>(value.data()));
            break;
        case TacoType::UInt64:
            if (value.size() == 8)
                return Value(Load<uint64_t>(value.data()));
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
        case TacoType::Map:
            out_ += "<map " + std::to_string(value.size()) + " bytes>";
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
    writer.TopMessage(message);
    return writer.Text();
}

template <class R>
std::string Access::RefText(const R& ref, const TextStyle& style)
{
    CTextWriter writer(style);
    writer.Message(ref);
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

namespace detail
{
inline bool ReadFrameInfo(const uint8_t* src, size_t size, FrameInfo* out, Error* error)
{
    if (size < HeaderSize(0, 0))
        return Fail(error, "frame too short");
    if (src[0] != 'P' || src[1] != 'C')
        return Fail(error, "bad magic");
    if (src[2] != kFrameVersion)
        return Fail(error, "unknown frame version " + std::to_string(src[2]) + ", expected " + std::to_string(static_cast<unsigned>(kFrameVersion)));
    const size_t nameLength = src[3];
    if (size < HeaderSize(nameLength, 0))
        return Fail(error, "frame too short");
    const uint16_t version = Load<uint16_t>(src + 4 + nameLength);
    const uint16_t minVersion = Load<uint16_t>(src + 6 + nameLength);
    if (minVersion == 0 || minVersion > version || static_cast<uint32_t>(version - minVersion) >= kMaxSchemaVersions)
        return Fail(error, "bad version range " + std::to_string(minVersion) + ".." + std::to_string(version));
    const size_t hashCount = static_cast<size_t>(version - minVersion) + 1;
    if (size < HeaderSize(nameLength, hashCount))
        return Fail(error, "frame too short");
    out->name = reinterpret_cast<const char*>(src + 4);
    out->nameLength = nameLength;
    out->version = version;
    out->minVersion = minVersion;
    out->hashCount = hashCount;
    out->headerSize = HeaderSize(nameLength, hashCount);
    out->bodySize = Load<uint32_t>(src + 24 + nameLength);
    return true;
}

inline bool ReadHeader(const uint8_t* src, size_t size, Header* out, Error* error)
{
    FrameInfo info;
    if (!ReadFrameInfo(src, size, &info, error))
        return false;
    out->name.assign(info.name, info.nameLength);
    out->version = info.version;
    out->minVersion = info.minVersion;
    out->env = LoadEnvelope(src);
    out->bodySize = info.bodySize;
    out->schemaHash = Load<uint32_t>(src + 28 + info.nameLength + 4 * (info.hashCount - 1));
    return true;
}

inline std::string ErrorText(const Error& error)
{
    return error.path.empty() ? error.what : error.path + ": " + error.what;
}

// Checks shared by ToObject and TryView: header, name, supported versions, own fingerprint, body length.
// Nothing is allocated unless the frame is rejected.
template <class T>
bool CheckFrame(const uint8_t* src, size_t size, FrameInfo* info, Error* error)
{
    if (size > kMaxFrameBytes)
        return Fail(error, "frame too large");
    if (!ReadFrameInfo(src, size, info, error))
        return false;
    const char* name = T::MsgName();
    const size_t nameLength = std::strlen(name);
    const uint16_t mine = T::MsgVersion();
    if (info->nameLength != nameLength || std::memcmp(info->name, name, nameLength) != 0)
        return Fail(error, std::string("message name mismatch: expected ") + name + ", got " + std::string(info->name, info->nameLength));
    if (info->version < mine)
    {
        std::string what = std::string("message version too old: ") + name + " reads version " + std::to_string(mine) + " or newer, got "
            + std::to_string(info->version);
        if (info->version >= T::MsgMinVersion())
            what += ". Use the v" + std::to_string(info->version) + " type";
        return Fail(error, what);
    }
    if (info->minVersion > mine)
        return Fail(error, std::string("message version too new: ") + name + " reads version " + std::to_string(mine) + ", sender supports "
            + std::to_string(info->minVersion) + ".." + std::to_string(info->version));
    const uint32_t hash = Load<uint32_t>(src + 28 + nameLength + 4 * static_cast<size_t>(mine - info->minVersion));
    if (hash != T::MsgSchemaHash())
        return Fail(error, std::string("schema mismatch: ") + name + " version " + std::to_string(mine) + " expected " + Hex32(T::MsgSchemaHash())
            + ", got " + Hex32(hash));
    if (size - info->headerSize != info->bodySize)
        return Fail(error, size - info->headerSize < info->bodySize ? "frame truncated" : "trailing bytes after frame");
    return true;
}

// Body errors of a newer frame name both versions.
template <class T>
bool FailNewer(Error* error, const FrameInfo& info)
{
    if (info.version > T::MsgVersion() && error != nullptr)
        error->what += " (frame version " + std::to_string(info.version) + " read as " + std::to_string(T::MsgVersion()) + ")";
    return false;
}

// Decodes into decoded, a new object (NoStampTag).
template <class T>
bool DecodeInto(const uint8_t* src, size_t size, T& decoded, Error* error)
{
    try
    {
        FrameInfo info;
        if (!CheckFrame<T>(src, size, &info, error))
            return false;
        decoded.SetEnvelope(LoadEnvelope(src));
        if (!Access::ReadBody(decoded, src + info.headerSize, info.bodySize, error, 0, info.version > T::MsgVersion()))
            return FailNewer<T>(error, info);
        return true;
    }
    catch (const std::bad_alloc&)
    {
        return Fail(error, "out of memory");
    }
}

// Decodes into a temporary and then moves it, so src may point into out's own buffer. out is unchanged on failure.
template <class T>
bool Decode(const uint8_t* src, size_t size, T* out, Error* error)
{
    try
    {
        T decoded{NoStampTag()};
        if (!DecodeInto(src, size, decoded, error))
            return false;
        *out = std::move(decoded);
        return true;
    }
    catch (const std::bad_alloc&)
    {
        return Fail(error, "out of memory");
    }
}

// Takes the bytes. A frame of exactly T's layout becomes the object's buffer; any other frame is copied like Decode.
template <class T>
bool DecodeAdopt(std::vector<uint8_t>&& bytes, T* out, Error* error)
{
    try
    {
        FrameInfo info;
        if (!CheckFrame<T>(bytes.data(), bytes.size(), &info, error))
            return false;
        if (!Access::SameLayout<T>(bytes.data(), info))
            return Decode(bytes.data(), bytes.size(), out, error);
        T decoded = Access::Adopted<T>(std::move(bytes));
        if (!Access::ReadAdopted(decoded, info, error))
            return false;
        *out = std::move(decoded);
        Access::MarkSerialized(*out);
        return true;
    }
    catch (const std::bad_alloc&)
    {
        return Fail(error, "out of memory");
    }
}

template <class T>
bool Access::SameLayout(const uint8_t* frame, const FrameInfo& info)
{
    if (info.version != T::MsgVersion() || info.minVersion != T::MsgMinVersion())
        return false;
    const uint32_t* hashes = T::SchemaHashes();
    for (size_t i = 0; i < info.hashCount; ++i)
    {
        if (Load<uint32_t>(frame + 28 + info.nameLength + 4 * i) != hashes[i])
            return false;
    }
    return true;
}

template <class T>
bool Access::ReadAdopted(T& message, const FrameInfo& info, Error* error)
{
    if (!ReadBody(message, message.buf_.data() + info.headerSize, info.bodySize, error, 0, false))
        return false;
    MarkSerialized(message);
    return true;
}
}  // namespace detail

/// Base of the generated <Message>::View: fixed fields read in place from received bytes (TryView).
/// Valid after a successful TryView while the bytes live and stay unchanged. ext entries are not visible.
class CView
{
public:
    Envelope GetEnvelope() const
    {
        return detail::LoadEnvelope(frame_);
    }

    uint32_t GetId() const
    {
        return GetEnvelope().id;
    }

    uint32_t GetCorr() const
    {
        return GetEnvelope().corr;
    }

    /// UTC microseconds since 1970-01-01.
    int64_t GetTimestamp() const
    {
        return GetEnvelope().ts;
    }

protected:
    const uint8_t* Fixed() const
    {
        return fixed_;
    }

private:
    friend struct detail::Access;

    const uint8_t* frame_ = nullptr;
    const uint8_t* fixed_ = nullptr;
};

namespace detail
{
template <class T>
bool Access::ViewOf(const uint8_t* src, size_t size, typename T::View* out, Error* error)
{
    try
    {
        FrameInfo info;
        if (!CheckFrame<T>(src, size, &info, error))
            return false;
        const bool newer = info.version > T::MsgVersion();
        CReader reader(src + info.headerSize, info.bodySize);
        const uint8_t* fixed = nullptr;
        if (!TakeFixed(reader, T::kFixedSize, newer, &fixed, error))
            return FailNewer<T>(error, info);
        if (reader.Left() != 0)
        {
            // ext entries or fields of a newer version: checked like ToObject, not visible through the view.
            CRawFields fields;
            if (!ParseVar(reader, &fields, error))
                return FailNewer<T>(error, info);
            if (!IsOn(T::kAllowExtension) && !newer)
            {
                const RawField& raw = *fields.begin();
                Fail(error, "unknown key (allow_extension false)");
                PrependPath(error, std::string(raw.key, raw.keyLength));
                return false;
            }
        }
        out->frame_ = src;
        out->fixed_ = fixed;
        return true;
    }
    catch (const std::bad_alloc&)
    {
        return Fail(error, "out of memory");
    }
}
}  // namespace detail

/// Envelope API shared by every generated message (CRTP base). The envelope lives in the frame header, so these calls
/// read and write the frame bytes directly. A new object gets id = NextId() and ts = NowUs(); corr starts at 0.
/// auto_envelope false in the yaml, or DisableAutoEnvelope(), leaves id and ts at 0.
template <class T>
class CMessage
{
public:
    uint32_t GetId() const
    {
        return detail::LoadEnvelope(Frame()).id;
    }

    void SetId(uint32_t id)
    {
        detail::Store<uint32_t>(detail::EnvelopeAt(Buffer()), id);
    }

    /// Id of the message this one answers. Set by the user.
    uint32_t GetCorr() const
    {
        return detail::LoadEnvelope(Frame()).corr;
    }

    void SetCorr(uint32_t corr)
    {
        detail::Store<uint32_t>(detail::EnvelopeAt(Buffer()) + 4, corr);
    }

    /// UTC microseconds since 1970-01-01.
    int64_t GetTimestamp() const
    {
        return detail::LoadEnvelope(Frame()).ts;
    }

    void SetTimestamp(int64_t timestampUs)
    {
        detail::Store<int64_t>(detail::EnvelopeAt(Buffer()) + 8, timestampUs);
    }

    void SetTimestampAsNow()
    {
        SetTimestamp(NowUs());
    }

    Envelope GetEnvelope() const
    {
        return detail::LoadEnvelope(Frame());
    }

    void SetEnvelope(const Envelope& env)
    {
        detail::StoreEnvelope(Buffer(), env);
    }

    /// Timestamp as local "YYYY-MM-DD" and "hh:mm:ss.ffffff". False if the timestamp is not a time (see TryGetTime).
    bool TryGetTimeFromTimestamp(std::string* date, std::string* time, std::string* error = nullptr) const
    {
        return TryGetTime(GetTimestamp(), date, time, error);
    }

    bool TryGetTimeFromTimestamp(int* year, int* month, int* day, int* hour, int* minute, int* second, int* millisecond,
                                 int* microsecond, std::string* error = nullptr) const
    {
        return TryGetTime(GetTimestamp(), year, month, day, hour, minute, second, millisecond, microsecond, error);
    }

protected:
    CMessage() = default;

private:
    const uint8_t* Frame() const
    {
        return static_cast<const T*>(this)->buf_.data();
    }

    std::vector<uint8_t>& Buffer()
    {
        return static_cast<T*>(this)->buf_;
    }
};

/// Parses the frame header only, not the body (for dispatch). Returns false with the reason in error.
inline bool PeekHeader(const uint8_t* src, size_t size, Header* out, std::string* error = nullptr)
{
    Error detailError;
    if (detail::ReadHeader(src, size, out, &detailError))
        return true;
    return detail::SetError(error, detail::ErrorText(detailError));
}

/// Bytes to object without exceptions. On failure returns false, out is unchanged, and error gets "path: reason"
/// (path such as "sections[0].name", empty for header errors).
/// Accepts frames of T's version and newer: a newer frame is read as T's version (README.md "Versioning").
template <class T>
bool TryToObject(const uint8_t* src, size_t size, T* out, std::string* error = nullptr)
{
    Error detailError;
    if (detail::Decode(src, size, out, &detailError))
        return true;
    return detail::SetError(error, detail::ErrorText(detailError));
}

template <class T>
bool TryToObject(const std::vector<uint8_t>& bytes, T* out, std::string* error = nullptr)
{
    return TryToObject(bytes.data(), bytes.size(), out, error);
}

/// Takes the bytes. A frame of exactly T's version (same supported versions and fingerprints) becomes the object's
/// buffer without a copy, and Serialize() returns it as it is until a variable field or ext changes.
/// Other frames are copied as usual.
template <class T>
bool TryToObject(std::vector<uint8_t>&& bytes, T* out, std::string* error = nullptr)
{
    Error detailError;
    if (detail::DecodeAdopt(std::move(bytes), out, &detailError))
        return true;
    return detail::SetError(error, detail::ErrorText(detailError));
}

/// Bytes to object. Throws CParseError on failure (catch it in the same module).
template <class T>
T ToObject(const uint8_t* src, size_t size)
{
    T object{detail::NoStampTag()};
    Error error;
    if (!detail::DecodeInto(src, size, object, &error))
        throw CParseError(error);
    return object;
}

template <class T>
T ToObject(const std::vector<uint8_t>& bytes)
{
    return ToObject<T>(bytes.data(), bytes.size());
}

/// Takes the bytes, like TryToObject with std::vector&&.
template <class T>
T ToObject(std::vector<uint8_t>&& bytes)
{
    Error error;
    detail::FrameInfo info;
    if (!detail::CheckFrame<T>(bytes.data(), bytes.size(), &info, &error))
        throw CParseError(error);
    if (!detail::Access::SameLayout<T>(bytes.data(), info))
        return ToObject<T>(bytes.data(), bytes.size());
    T object = detail::Access::Adopted<T>(std::move(bytes));
    bool ok = false;
    try
    {
        ok = detail::Access::ReadAdopted(object, info, &error);
    }
    catch (const std::bad_alloc&)
    {
        detail::Fail(&error, "out of memory");
    }
    if (!ok)
        throw CParseError(error);
    return object;
}

/// Fixed-only messages (T::View exists): checks the frame like TryToObject and reads the fixed fields in place,
/// without copying the bytes. No allocation unless more than 16 entries (ext, or fields of a newer version) follow
/// the fixed block. view stays valid while the bytes live and stay unchanged.
/// Call with the type: protaco::TryView<Heartbeat>(bytes, &view).
template <class T>
bool TryView(const uint8_t* src, size_t size, typename T::View* view, std::string* error = nullptr)
{
    Error detailError;
    if (detail::Access::ViewOf<T>(src, size, view, &detailError))
        return true;
    return detail::SetError(error, detail::ErrorText(detailError));
}

template <class T>
bool TryView(const std::vector<uint8_t>& bytes, typename T::View* view, std::string* error = nullptr)
{
    return TryView<T>(bytes.data(), bytes.size(), view, error);
}

/// A view into a temporary would dangle.
template <class T>
bool TryView(std::vector<uint8_t>&& bytes, typename T::View* view, std::string* error = nullptr) = delete;

}  // namespace protaco

#endif  // PROTACO_RUNTIME_0_6_0
