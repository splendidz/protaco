// Tick.g.h. protacoc 0.6.0, source Tick.yaml (sha1 d48b7805e10f). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace scalars
{

/// The sender fills the envelope: new objects keep id 0 and ts 0.
/// [version 1; schema 0x6D9855B0; source Tick.yaml]
class Tick : public protaco::CMessage<Tick>
{
public:
    /// Initialized to defaults. The envelope stays 0 (auto_envelope false): set it with SetId, SetCorr and SetTimestamp.
    Tick()
        : Tick(protaco::detail::NoStampTag())
    {
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Tick(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Tick::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Tick";
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
        return 0x6D9855B0u;
    }

    /// [type int]
    int32_t get_n() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 0);
    }

    void set_n(int32_t value)
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

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
    /// Fixed fields only. With an empty ext, serializing costs nothing.
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Tick>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// [type int]
        int32_t get_n() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 0);
        }
    };

    /// Fixed fields read and written in place in a frame in memory the caller owns (protaco::TryMutableView<Tick>),
    /// for example shared memory. Nothing is copied. Valid after a successful TryMutableView while the memory lives.
    /// Writes are plain stores: concurrent readers and writers need the caller's synchronization.
    class MutableView : public protaco::CMutableView
    {
    public:
        /// Bytes of a new Tick frame (header and fixed block): the memory to fill with Serialize() and give TryMutableView.
        static constexpr size_t FrameSize()
        {
            return 44;
        }

        /// [type int]
        int32_t get_n() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 0);
        }

        void set_n(int32_t value)
        {
            protaco::detail::Store<int32_t>(Fixed() + 0, value);
        }

    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Tick>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Tick(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Tick::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 40,   // frame header (28 + name 4 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 4,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x6D9855B0u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "n" };
        return protaco::detail::InList(key, kKeys, 1);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Tick& self, TWriter& writer)
    {
        writer.Field("n", self.get_n());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace scalars
