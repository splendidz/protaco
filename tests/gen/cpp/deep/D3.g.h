// D3.g.h. protacoc 0.4.0, source D3.yaml (sha1 5f1911271792). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "D2.g.h"

namespace deep
{

/// [version 1; schema 0x93F6E6B3; source D3.yaml]
class D3
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    D3()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &D3::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "D3";
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
        return 0x93F6E6B3u;
    }

    /// [type D2]
    const D2& get_d() const
    {
        return d_;
    }

    D2& get_mutable_d()
    {
        return d_;
    }

    void set_d(const D2& value)
    {
        d_ = value;
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
        kFixedOffset = 38,   // frame header (28 + name 2 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 0,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x93F6E6B3u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "d" };
        return protaco::detail::InList(key, kKeys, 1);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("d", self.d_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const D3& self, TWriter& writer)
    {
        writer.Field("d", self.d_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    D2 d_;
    protaco::CExt ext_;
};

}  // namespace deep
