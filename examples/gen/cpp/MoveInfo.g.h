// MoveInfo.g.h. protacoc 0.4.0, source MoveInfo.yaml (sha1 35bb174c6ba8). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// One robot move. Element of TransferRequest.moves.
/// A pick followed by a place.
/// Every field is fixed-length, so sending it alone costs nothing to serialize.
/// one robot move
/// [version 1; schema 0x99EC76D7; owner robot; source MoveInfo.yaml]
class MoveInfo
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    MoveInfo()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &MoveInfo::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, 1);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "MoveInfo";
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
        return 0x99EC76D7u;
    }

    /// Source location name. Examples: SHELF1, TABLE3
    /// source location
    /// [type string(32); fixed 32 bytes, set rejects longer values]
    std::string get_from() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 32);
    }

    /// Returns false and keeps the old value if longer than 32 bytes.
    bool set_from(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 32, value);
    }

    /// target location
    /// [type string(32); fixed 32 bytes, set rejects longer values]
    std::string get_to() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 32, 32);
    }

    /// Returns false and keeps the old value if longer than 32 bytes.
    bool set_to(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 32, 32, value);
    }

    /// source slot number (1-based)
    /// [type int; default 1]
    int32_t get_slot() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 64);
    }

    void set_slot(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, value);
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
    /// Fixed fields only. With an empty ext, serializing costs nothing.
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
        static const uint32_t kHashes[] = { 0x99EC76D7u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "from", "to", "slot" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const MoveInfo& self, TWriter& writer)
    {
        writer.Field("from", self.get_from());
        writer.Field("to", self.get_to());
        writer.Field("slot", self.get_slot());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;
};

}  // namespace demo
