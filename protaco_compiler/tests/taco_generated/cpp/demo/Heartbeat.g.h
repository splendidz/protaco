// Heartbeat.g.h. protacoc 0.7.1, source Heartbeat.yaml (sha1 32f326820062). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// Device liveness signal. Every field is fixed-length and ext is off, so Serialize() returns the buffer as is (zero cost).
/// Recommended shape for messages sent very often.
/// [version 1; schema 0x2BB85C55; allow_extension false; source Heartbeat.yaml]
class Heartbeat : public protaco::CMessage<Heartbeat>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Heartbeat()
        : Heartbeat(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Heartbeat(protaco::detail::NoStampTag)
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

    /// Returns false, keeps the old value and writes the reason to error if longer than 32 bytes.
    bool set_node(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 32, value, "node", error);
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

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
    /// Fixed fields only. Serializing costs nothing.
    /// Unchanged variable fields and ext reuse the last bytes. After a mutable_ call the variable block is rebuilt every time.
    /// The returned reference is valid until this object is modified.
    const std::vector<uint8_t>& Serialize() const override
    {
        return protaco::detail::Access::Serialize(*this);
    }

    /// Text for logs: field names and values. protaco::TextStyle::Pretty() for one field per line.
    std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const override
    {
        return protaco::detail::Access::ToText(*this, style);
    }

    /// Fixed fields read in place from received bytes (protaco::TryView<Heartbeat>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// device name
        /// [type string(32); fixed 32 bytes, set rejects longer values]
        std::string get_node() const
        {
            return protaco::detail::LoadChars(Fixed() + 0, 32);
        }

        /// sequence number
        /// [type int64]
        int64_t get_seq() const
        {
            return protaco::detail::Load<int64_t>(Fixed() + 32);
        }

        /// device is healthy
        /// [type bool; default true]
        bool get_healthy() const
        {
            return protaco::detail::LoadBool(Fixed() + 40);
        }

        /// CPU load (0.0..1.0)
        /// [type double]
        double get_cpuLoad() const
        {
            return protaco::detail::Load<double>(Fixed() + 41);
        }

        /// uptime in seconds
        /// [type int]
        int32_t get_uptimeSec() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 49);
        }
    };

    /// Fixed fields read and written in place in a frame in memory the caller owns (protaco::TryMutableView<Heartbeat>),
    /// for example shared memory. Nothing is copied. Valid after a successful TryMutableView while the memory lives.
    /// Writes are plain stores: concurrent readers and writers need the caller's synchronization.
    class MutableView : public protaco::CMutableView
    {
    public:
        /// Bytes of a new Heartbeat frame (header and fixed block): the memory to fill with Serialize() and give TryMutableView.
        static constexpr size_t FrameSize()
        {
            return 98;
        }

        /// device name
        /// [type string(32); fixed 32 bytes, set rejects longer values]
        std::string get_node() const
        {
            return protaco::detail::LoadChars(Fixed() + 0, 32);
        }

        /// Returns false, keeps the old value and writes the reason to error if longer than 32 bytes.
        bool set_node(const std::string& value, std::string* error = nullptr)
        {
            return protaco::detail::StoreChars(Fixed() + 0, 32, value, "node", error);
        }

        /// sequence number
        /// [type int64]
        int64_t get_seq() const
        {
            return protaco::detail::Load<int64_t>(Fixed() + 32);
        }

        void set_seq(int64_t value)
        {
            protaco::detail::Store<int64_t>(Fixed() + 32, value);
        }

        /// device is healthy
        /// [type bool; default true]
        bool get_healthy() const
        {
            return protaco::detail::LoadBool(Fixed() + 40);
        }

        void set_healthy(bool value)
        {
            protaco::detail::StoreBool(Fixed() + 40, value);
        }

        /// CPU load (0.0..1.0)
        /// [type double]
        double get_cpuLoad() const
        {
            return protaco::detail::Load<double>(Fixed() + 41);
        }

        void set_cpuLoad(double value)
        {
            protaco::detail::Store<double>(Fixed() + 41, value);
        }

        /// uptime in seconds
        /// [type int]
        int32_t get_uptimeSec() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 49);
        }

        void set_uptimeSec(int32_t value)
        {
            protaco::detail::Store<int32_t>(Fixed() + 49, value);
        }

    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Heartbeat>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Heartbeat(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Heartbeat::IsSchemaKey);
    }

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
