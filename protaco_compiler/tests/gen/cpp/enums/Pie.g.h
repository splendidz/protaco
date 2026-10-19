// Pie.g.h. protacoc 0.6.0, source Pie.yaml (sha1 b9d29a89197b). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "PizzaSize.enum.h"

namespace enums
{

/// A pie with enum fields.
/// [version 1; schema 0xB67183D5; source Pie.yaml]
class Pie : public protaco::CMessage<Pie>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Pie()
        : Pie(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Pie(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Pie::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 0, static_cast<int32_t>(PizzaSize::Large));
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 4, static_cast<int32_t>(PizzaSize::Small));
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Pie";
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
        return 0xB67183D5u;
    }

    /// [type PizzaSize; default Large]
    /// Numbers this build does not know are kept as they are (ToString shows ?(n)).
    PizzaSize get_size() const
    {
        return static_cast<PizzaSize>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 0));
    }

    void set_size(PizzaSize value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 0, static_cast<int32_t>(value));
    }

    /// No default: the first value of the enum (Small).
    /// [type PizzaSize]
    /// Numbers this build does not know are kept as they are (ToString shows ?(n)).
    PizzaSize get_crust() const
    {
        return static_cast<PizzaSize>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 4));
    }

    void set_crust(PizzaSize value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 4, static_cast<int32_t>(value));
    }

    /// [type list<PizzaSize>]
    const std::vector<PizzaSize>& get_sizes() const
    {
        return sizes_;
    }

    std::vector<PizzaSize>& mutable_sizes()
    {
        cache_.exposed = true;
        return sizes_;
    }

    void set_sizes(const std::vector<PizzaSize>& value)
    {
        sizes_ = value;
        cache_.valid = false;
    }

    /// [type string; default ""]
    const std::string& get_name() const
    {
        return name_;
    }

    void set_name(const std::string& value)
    {
        name_ = value;
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
    friend class protaco::CMessage<Pie>;

    enum : uint32_t
    {
        kFixedOffset = 39,   // frame header (28 + name 3 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 8,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xB67183D5u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "size", "crust", "sizes", "name" };
        return protaco::detail::InList(key, kKeys, 4);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("sizes", self.sizes_, false);
        visitor.Field("name", self.name_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Pie& self, TWriter& writer)
    {
        writer.Field("size", self.get_size());
        writer.Field("crust", self.get_crust());
        writer.Field("sizes", self.sizes_);
        writer.Field("name", self.name_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::vector<PizzaSize> sizes_;
    std::string name_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace enums
