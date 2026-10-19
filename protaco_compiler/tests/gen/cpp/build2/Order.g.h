// Order.g.h. protacoc 0.5.0, source Order.yaml (sha1 d90a16ea2114). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace build2
{

/// Version window test. Order at version 2: weight (fixed) and tags (variable) added.
/// [version 2; schema 0x0E7D8E49; supports 1..2; source Order.yaml]
class Order
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    Order()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Order::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 8, 1);
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 12, 0.5);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Order";
    }

    /// Version of this type. ToObject reads frames of this version or newer as this version.
    static uint16_t MsgVersion()
    {
        return 2;
    }

    /// Oldest supported version (min_version). Frames carry one schema fingerprint per version from here up.
    static uint16_t MsgMinVersion()
    {
        return 1;
    }

    /// Schema fingerprint of this version. A frame whose fingerprint for this version differs is rejected by ToObject.
    static uint32_t MsgSchemaHash()
    {
        return 0x0E7D8E49u;
    }

    /// [type string(8); fixed 8 bytes, set rejects longer values]
    std::string get_id() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 8);
    }

    /// Returns false and keeps the old value if longer than 8 bytes.
    bool set_id(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 8, value);
    }

    /// Declared in the middle, laid out after the version 1 fields.
    /// [type double; default 0.5; added in 2]
    double get_weight() const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 12);
    }

    void set_weight(double value)
    {
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 12, value);
    }

    /// [type int; default 1]
    int32_t get_qty() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 8);
    }

    void set_qty(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 8, value);
    }

    /// [type string]
    const std::string& get_note() const
    {
        return note_;
    }

    void set_note(const std::string& value)
    {
        note_ = value;
        cache_.valid = false;
    }

    /// [type string; required, decoding fails if missing]
    const std::string& get_color() const
    {
        return color_;
    }

    void set_color(const std::string& value)
    {
        color_ = value;
        cache_.valid = false;
    }

    /// [type list<string>; added in 2]
    const std::vector<std::string>& get_tags() const
    {
        return tags_;
    }

    std::vector<std::string>& get_mutable_tags()
    {
        cache_.exposed = true;
        return tags_;
    }

    void set_tags(const std::vector<std::string>& value)
    {
        tags_ = value;
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

    /// Writes the envelope (id, corr, ts) straight into the frame header.
    void SetEnvelope(const protaco::Envelope& env)
    {
        protaco::detail::StoreEnvelope(buf_, env);
    }

    protaco::Envelope GetEnvelope() const
    {
        return protaco::detail::LoadEnvelope(buf_.data());
    }

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
    /// Unchanged variable fields and ext reuse the last bytes. After a get_mutable_ call the variable block is rebuilt every time.
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

    enum : uint32_t
    {
        kFixedOffset = 45,   // frame header (28 + name 5 + fingerprints 4 x 2) + FixedLen (4)
        kFixedSize = 20,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xD05758A9u, 0x0E7D8E49u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "id", "weight", "qty", "note", "color", "tags" };
        return protaco::detail::InList(key, kKeys, 6);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("note", self.note_, false);
        visitor.Field("color", self.color_, true);
        visitor.Field("tags", self.tags_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Order& self, TWriter& writer)
    {
        writer.Field("id", self.get_id());
        writer.Field("weight", self.get_weight());
        writer.Field("qty", self.get_qty());
        writer.Field("note", self.note_);
        writer.Field("color", self.color_);
        writer.Field("tags", self.tags_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string note_;
    std::string color_;
    std::vector<std::string> tags_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

namespace v1
{

/// Version window test. Order at version 2: weight (fixed) and tags (variable) added.
/// Version 1 layout, for frames from peers on version 1.
/// [version 1; schema 0xD05758A9; supports 1..2; source Order.yaml]
class Order
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    Order()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Order::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 8, 1);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Order";
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
        return 0xD05758A9u;
    }

    /// [type string(8); fixed 8 bytes, set rejects longer values]
    std::string get_id() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 8);
    }

    /// Returns false and keeps the old value if longer than 8 bytes.
    bool set_id(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 8, value);
    }

    /// [type int; default 1]
    int32_t get_qty() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 8);
    }

    void set_qty(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 8, value);
    }

    /// [type string]
    const std::string& get_note() const
    {
        return note_;
    }

    void set_note(const std::string& value)
    {
        note_ = value;
        cache_.valid = false;
    }

    /// [type string; required, decoding fails if missing]
    const std::string& get_color() const
    {
        return color_;
    }

    void set_color(const std::string& value)
    {
        color_ = value;
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

    /// Writes the envelope (id, corr, ts) straight into the frame header.
    void SetEnvelope(const protaco::Envelope& env)
    {
        protaco::detail::StoreEnvelope(buf_, env);
    }

    protaco::Envelope GetEnvelope() const
    {
        return protaco::detail::LoadEnvelope(buf_.data());
    }

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
    /// Unchanged variable fields and ext reuse the last bytes. After a get_mutable_ call the variable block is rebuilt every time.
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

    enum : uint32_t
    {
        kFixedOffset = 41,   // frame header (28 + name 5 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 12,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xD05758A9u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "id", "weight", "qty", "note", "color", "tags" };
        return protaco::detail::InList(key, kKeys, 6);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("note", self.note_, false);
        visitor.Field("color", self.color_, true);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Order& self, TWriter& writer)
    {
        writer.Field("id", self.get_id());
        writer.Field("qty", self.get_qty());
        writer.Field("note", self.note_);
        writer.Field("color", self.color_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string note_;
    std::string color_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace v1

}  // namespace build2
