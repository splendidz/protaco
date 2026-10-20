// Numbers.g.h. protacoc 0.6.0, source Numbers.yaml (sha1 c3af6c4b45e4). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace scalars
{

/// Unsigned integers at their limits and UTF-8 strings.
/// [version 1; schema 0x8230A950; source Numbers.yaml]
class Numbers : public protaco::CMessage<Numbers>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Numbers()
        : Numbers(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Numbers(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Numbers::IsSchemaKey);
        protaco::detail::Store<uint32_t>(buf_.data() + kFixedOffset + 0, 4000000000u);
        protaco::detail::Store<uint64_t>(buf_.data() + kFixedOffset + 4, 18446744073709551615ull);
        protaco::detail::Store<int64_t>(buf_.data() + kFixedOffset + 12, -9000000000ll);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Numbers";
    }

    /// Version of this type. ToObject reads frames of this version or newer as this version.
    static uint16_t MsgVersion()
    {
        return 1;
    }

    /// Oldest supported version (min_version). Frames carry one schema fingerprint per version from here up.
    static uint16_t MsgMinVersion()
    {
        return 1;
    }

    /// Schema fingerprint of this version. A frame whose fingerprint for this version differs is rejected by ToObject.
    static uint32_t MsgSchemaHash()
    {
        return 0x8230A950u;
    }

    /// [type uint; default 4000000000]
    uint32_t get_u32() const
    {
        return protaco::detail::Load<uint32_t>(buf_.data() + kFixedOffset + 0);
    }

    void set_u32(uint32_t value)
    {
        protaco::detail::Store<uint32_t>(buf_.data() + kFixedOffset + 0, value);
    }

    /// [type uint64; default 18446744073709551615]
    uint64_t get_u64() const
    {
        return protaco::detail::Load<uint64_t>(buf_.data() + kFixedOffset + 4);
    }

    void set_u64(uint64_t value)
    {
        protaco::detail::Store<uint64_t>(buf_.data() + kFixedOffset + 4, value);
    }

    /// [type int64; default -9000000000]
    int64_t get_i64() const
    {
        return protaco::detail::Load<int64_t>(buf_.data() + kFixedOffset + 12);
    }

    void set_i64(int64_t value)
    {
        protaco::detail::Store<int64_t>(buf_.data() + kFixedOffset + 12, value);
    }

    /// [type list<uint>]
    const std::vector<uint32_t>& get_us() const
    {
        return us_;
    }

    std::vector<uint32_t>& mutable_us()
    {
        cache_.exposed = true;
        return us_;
    }

    void set_us(const std::vector<uint32_t>& value)
    {
        us_ = value;
        cache_.valid = false;
    }

    /// [type list<uint64>]
    const std::vector<uint64_t>& get_u64s() const
    {
        return u64s_;
    }

    std::vector<uint64_t>& mutable_u64s()
    {
        cache_.exposed = true;
        return u64s_;
    }

    void set_u64s(const std::vector<uint64_t>& value)
    {
        u64s_ = value;
        cache_.valid = false;
    }

    /// [type string(16); fixed 16 bytes, set rejects longer values; default ""]
    std::string get_name() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 20, 16);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 16 bytes.
    bool set_name(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 20, 16, value, "name", error);
    }

    /// [type string]
    const std::string& get_text() const
    {
        return text_;
    }

    void set_text(const std::string& value)
    {
        text_ = value;
        cache_.valid = false;
    }

    /// Extra key-value data not in the yaml. Unknown keys of received messages are kept here.
    const protaco::CExt& GetExt() const
    {
        return ext_;
    }

    protaco::CExt& GetMutableExt()
    {
        return ext_;
    }

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
    /// Unchanged variable fields and ext reuse the last bytes. After a mutable_ call the variable block is rebuilt every time.
    /// The returned reference is valid until this object is modified.
    const std::vector<uint8_t>& Serialize() const
    {
        return protaco::detail::Access::Serialize(*this);
    }

    /// Text for logs: field names and values, ext entries under ext{...}. protaco::TextStyle::Pretty() for one field per line.
    std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const
    {
        return protaco::detail::Access::ToText(*this, style);
    }

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Numbers>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Numbers(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Numbers::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 43,   // frame header (28 + name 7 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 36,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x8230A950u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "u32", "u64", "i64", "us", "u64s", "name", "text" };
        return protaco::detail::InList(key, kKeys, 7);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("us", self.us_, false);
        visitor.Field("u64s", self.u64s_, false);
        visitor.Field("text", self.text_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Numbers& self, TWriter& writer)
    {
        writer.Field("u32", self.get_u32());
        writer.Field("u64", self.get_u64());
        writer.Field("i64", self.get_i64());
        writer.Field("us", self.us_);
        writer.Field("u64s", self.u64s_);
        writer.Field("name", self.get_name());
        writer.Field("text", self.text_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::vector<uint32_t> us_;
    std::vector<uint64_t> u64s_;
    std::string text_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace scalars
