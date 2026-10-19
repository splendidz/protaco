// Heartbeat.g.h. protacoc 0.5.0, source Heartbeat.yaml (sha1 32f326820062). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// Device liveness signal. Every field is fixed-length and ext is off, so Serialize() returns the buffer as is (zero cost).
/// Recommended shape for messages sent very often.
/// [version 1; schema 0x2BB85C55; allow_extension false; source Heartbeat.yaml]
class Heartbeat
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    Heartbeat()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Heartbeat::IsSchemaKey);
        protaco::detail::StoreBool(buf_.data() + kFixedOffset + 40, true);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Heartbeat";
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
        return 0x2BB85C55u;
    }

    /// device name
    /// [type string(32); fixed 32 bytes, set rejects longer values]
    std::string get_node() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 32);
    }

    /// Returns false and keeps the old value if longer than 32 bytes.
    bool set_node(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 32, value);
    }

    /// sequence number
    /// [type int64]
    int64_t get_seq() const
    {
        return protaco::detail::Load<int64_t>(buf_.data() + kFixedOffset + 32);
    }

    void set_seq(int64_t value)
    {
        protaco::detail::Store<int64_t>(buf_.data() + kFixedOffset + 32, value);
    }

    /// device is healthy
    /// [type bool; default true]
    bool get_healthy() const
    {
        return protaco::detail::LoadBool(buf_.data() + kFixedOffset + 40);
    }

    void set_healthy(bool value)
    {
        protaco::detail::StoreBool(buf_.data() + kFixedOffset + 40, value);
    }

    /// CPU load (0.0..1.0)
    /// [type double]
    double get_cpuLoad() const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 41);
    }

    void set_cpuLoad(double value)
    {
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 41, value);
    }

    /// uptime in seconds
    /// [type int]
    int32_t get_uptimeSec() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 49);
    }

    void set_uptimeSec(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 49, value);
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
    /// Fixed fields only. Serializing costs nothing.
    /// Unchanged variable fields and ext reuse the last bytes. After a get_mutable_ call the variable block is rebuilt every time.
    /// The returned reference is valid until this object is modified.
    const std::vector<uint8_t>& Serialize() const
    {
        return protaco::detail::Access::Serialize(*this);
    }

    /// Text for logs: field names and values. protaco::TextStyle::Pretty() for one field per line.
    std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const
    {
        return protaco::detail::Access::ToText(*this, style);
    }

private:
    friend struct protaco::detail::Access;

    enum : uint32_t
    {
        kFixedOffset = 45,   // frame header (28 + name 9 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 53,
        kAllowExtension = 0,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x2BB85C55u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "node", "seq", "healthy", "cpuLoad", "uptimeSec" };
        return protaco::detail::InList(key, kKeys, 5);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Heartbeat& self, TWriter& writer)
    {
        writer.Field("node", self.get_node());
        writer.Field("seq", self.get_seq());
        writer.Field("healthy", self.get_healthy());
        writer.Field("cpuLoad", self.get_cpuLoad());
        writer.Field("uptimeSec", self.get_uptimeSec());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;   // stays empty: allow_extension false
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace demo
