// DeliveryProof.g.h. protacoc 0.4.0, source DeliveryProof.yaml (sha1 fd0c2b150ffc). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// Proof of delivery. Shows the bytes type: binary data carried as is, zeros and 0xFF included.
/// [version 1; schema 0xF769024A; owner courier; source DeliveryProof.yaml]
class DeliveryProof
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    DeliveryProof()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &DeliveryProof::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "DeliveryProof";
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
        return 0xF769024Au;
    }

    /// order number
    /// [type string(32); fixed 32 bytes, set rejects longer values]
    std::string get_orderId() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 32);
    }

    /// Returns false and keeps the old value if longer than 32 bytes.
    bool set_orderId(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 32, value);
    }

    /// JPEG bytes, untouched.
    /// doorstep photo
    /// [type bytes; required, decoding fails if missing]
    const std::vector<uint8_t>& get_photo() const
    {
        return photo_;
    }

    std::vector<uint8_t>& get_mutable_photo()
    {
        return photo_;
    }

    void set_photo(const std::vector<uint8_t>& value)
    {
        photo_ = value;
    }

    /// signature images
    /// [type list<bytes>]
    const std::vector<std::vector<uint8_t>>& get_signatures() const
    {
        return signatures_;
    }

    std::vector<std::vector<uint8_t>>& get_mutable_signatures()
    {
        return signatures_;
    }

    void set_signatures(const std::vector<std::vector<uint8_t>>& value)
    {
        signatures_ = value;
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
        kFixedOffset = 49,   // frame header (28 + name 13 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 32,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xF769024Au };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "orderId", "photo", "signatures" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("photo", self.photo_, true);
        visitor.Field("signatures", self.signatures_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const DeliveryProof& self, TWriter& writer)
    {
        writer.Field("orderId", self.get_orderId());
        writer.Field("photo", self.photo_);
        writer.Field("signatures", self.signatures_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::vector<uint8_t> photo_;
    std::vector<std::vector<uint8_t>> signatures_;
    protaco::CExt ext_;
};

}  // namespace demo
