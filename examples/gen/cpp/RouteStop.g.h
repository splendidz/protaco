// RouteStop.g.h. protacoc 0.5.0, source RouteStop.yaml (sha1 7cc7901a2cd9). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// One stop of a delivery route. Element of DeliveryRoute.stops.
/// Every field is fixed-length, so sending it alone costs nothing to serialize.
/// [version 1; schema 0x9F1C87C1; source RouteStop.yaml]
class RouteStop
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    RouteStop()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &RouteStop::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, 15);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "RouteStop";
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
        return 0x9F1C87C1u;
    }

    /// Order delivered at this stop. Example: A-100
    /// order number
    /// [type string(16); fixed 16 bytes, set rejects longer values]
    std::string get_orderId() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 16);
    }

    /// Returns false and keeps the old value if longer than 16 bytes.
    bool set_orderId(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 16, value);
    }

    /// street address
    /// [type string(48); fixed 48 bytes, set rejects longer values]
    std::string get_address() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 16, 48);
    }

    /// Returns false and keeps the old value if longer than 48 bytes.
    bool set_address(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 16, 48, value);
    }

    /// minutes until arrival
    /// [type int; default 15]
    int32_t get_etaMin() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 64);
    }

    void set_etaMin(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, value);
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
    /// Fixed fields only. With an empty ext, serializing costs nothing.
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
        kFixedOffset = 45,   // frame header (28 + name 9 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 68,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x9F1C87C1u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "orderId", "address", "etaMin" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const RouteStop& self, TWriter& writer)
    {
        writer.Field("orderId", self.get_orderId());
        writer.Field("address", self.get_address());
        writer.Field("etaMin", self.get_etaMin());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace demo
