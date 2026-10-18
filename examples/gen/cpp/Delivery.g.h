// Delivery.g.h. protacoc 0.4.0, source Delivery.yaml (sha1 7aef43bfce7c). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// Delivery details. Used by PizzaOrder.delivery.
/// [version 1; schema 0xA031C44E; source Delivery.yaml]
class Delivery
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    Delivery()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Delivery::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, 1);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Delivery";
    }

    /// Version of this type. ToObject reads frames of this version or newer as this version.
    static uint16_t MsgVersion()
    {
        return 1;
    }

    /// Lowest version of the window. Frames carry one schema fingerprint per version from here up.
    static uint16_t MsgMinVersion()
    {
        return 1;
    }

    /// Schema fingerprint of this version. A frame whose fingerprint for this version differs is rejected by ToObject.
    static uint32_t MsgSchemaHash()
    {
        return 0xA031C44Eu;
    }

    /// street address
    /// [type string(64); fixed 64 bytes, set rejects longer values]
    std::string get_address() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 64);
    }

    /// Returns false and keeps the old value if longer than 64 bytes.
    bool set_address(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 64, value);
    }

    /// floor number
    /// [type int; default 1]
    int32_t get_floor() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 64);
    }

    void set_floor(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, value);
    }

    /// Building door code. One digit 0..9 per element.
    /// building door code
    /// [type list<int>]
    const std::vector<int32_t>& get_doorCode() const
    {
        return doorCode_;
    }

    std::vector<int32_t>& get_mutable_doorCode()
    {
        return doorCode_;
    }

    void set_doorCode(const std::vector<int32_t>& value)
    {
        doorCode_ = value;
    }

    /// phone numbers
    /// [type list<string>]
    const std::vector<std::string>& get_contacts() const
    {
        return contacts_;
    }

    std::vector<std::string>& get_mutable_contacts()
    {
        return contacts_;
    }

    void set_contacts(const std::vector<std::string>& value)
    {
        contacts_ = value;
    }

    /// Extra key-value data not in the yaml. Unknown keys of received messages are kept here.
    const protaco::CExt& get_ext() const
    {
        return ext_;
    }

    protaco::CExt& get_mutable_ext()
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
        kFixedOffset = 44,   // frame header (28 + name 8 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 68,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xA031C44Eu };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "address", "floor", "doorCode", "contacts" };
        return protaco::detail::InList(key, kKeys, 4);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("doorCode", self.doorCode_, false);
        visitor.Field("contacts", self.contacts_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Delivery& self, TWriter& writer)
    {
        writer.Field("address", self.get_address());
        writer.Field("floor", self.get_floor());
        writer.Field("doorCode", self.doorCode_);
        writer.Field("contacts", self.contacts_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::vector<int32_t> doorCode_;
    std::vector<std::string> contacts_;
    protaco::CExt ext_;
};

}  // namespace demo
