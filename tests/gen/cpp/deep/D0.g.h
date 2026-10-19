// D0.g.h. protacoc 0.5.0, source D0.yaml (sha1 c36d92fc3a71). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace deep
{

/// Depth test. D<n> nests n levels; D8 is the deepest allowed (runtime kMaxDepth).
/// [version 1; schema 0x329E542F; source D0.yaml]
class D0
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    D0()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &D0::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "D0";
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
        return 0x329E542Fu;
    }

    /// [type int]
    int32_t get_v() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 0);
    }

    void set_v(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 0, value);
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
        kFixedOffset = 38,   // frame header (28 + name 2 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 4,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x329E542Fu };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "v" };
        return protaco::detail::InList(key, kKeys, 1);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const D0& self, TWriter& writer)
    {
        writer.Field("v", self.get_v());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace deep
