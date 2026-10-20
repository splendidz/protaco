// Spot.g.h. protacoc 0.6.0, source Spot.yaml (sha1 11b8a28b10ac). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// A place on the dispatch board. fixed: true, so it sits inline in the board's fixed block.
/// [version 1; schema 0x2538569A; fixed: inline in other messages; source Spot.yaml]
class Spot : public protaco::CMessage<Spot>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Spot()
        : Spot(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Spot(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Spot::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Spot";
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
        return 0x2538569Au;
    }

    /// [type double]
    double get_lat() const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 0);
    }

    void set_lat(double value)
    {
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 0, value);
    }

    /// [type double]
    double get_lon() const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 8);
    }

    void set_lon(double value)
    {
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 8, value);
    }

    /// [type string(16); fixed 16 bytes, set rejects longer values]
    std::string get_label() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 16, 16);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 16 bytes.
    bool set_label(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 16, 16, value, "label", error);
    }

    /// Frame bytes. Fixed fields are already written by their setters; only variable fields are appended here.
    /// Fixed fields only. Serializing costs nothing.
    /// Unchanged variable fields and ext reuse the last bytes. After a mutable_ call the variable block is rebuilt every time.
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Spot>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// [type double]
        double get_lat() const
        {
            return protaco::detail::Load<double>(Fixed() + 0);
        }

        /// [type double]
        double get_lon() const
        {
            return protaco::detail::Load<double>(Fixed() + 8);
        }

        /// [type string(16); fixed 16 bytes, set rejects longer values]
        std::string get_label() const
        {
            return protaco::detail::LoadChars(Fixed() + 16, 16);
        }
    };

    /// Fixed fields read and written in place in a frame in memory the caller owns (protaco::TryMutableView<Spot>),
    /// for example shared memory. Nothing is copied. Valid after a successful TryMutableView while the memory lives.
    /// Writes are plain stores: concurrent readers and writers need the caller's synchronization.
    class MutableView : public protaco::CMutableView
    {
    public:
        /// Bytes of a new Spot frame (header and fixed block): the memory to fill with Serialize() and give TryMutableView.
        static constexpr size_t FrameSize()
        {
            return 72;
        }

        /// [type double]
        double get_lat() const
        {
            return protaco::detail::Load<double>(Fixed() + 0);
        }

        void set_lat(double value)
        {
            protaco::detail::Store<double>(Fixed() + 0, value);
        }

        /// [type double]
        double get_lon() const
        {
            return protaco::detail::Load<double>(Fixed() + 8);
        }

        void set_lon(double value)
        {
            protaco::detail::Store<double>(Fixed() + 8, value);
        }

        /// [type string(16); fixed 16 bytes, set rejects longer values]
        std::string get_label() const
        {
            return protaco::detail::LoadChars(Fixed() + 16, 16);
        }

        /// Returns false, keeps the old value and writes the reason to error if longer than 16 bytes.
        bool set_label(const std::string& value, std::string* error = nullptr)
        {
            return protaco::detail::StoreChars(Fixed() + 16, 16, value, "label", error);
        }

    };

    /// Read-only Spot inside another message's fixed block: fields read in place, nothing copied.
    /// Valid while those bytes live.
    class ConstRef : public protaco::detail::BlockRef
    {
    public:
        explicit ConstRef(const uint8_t* block)
        {
            p_ = block;
        }

        /// Message name, for ToString.
        static const char* MsgName()
        {
            return "Spot";
        }

        /// [type double]
        double get_lat() const
        {
            return protaco::detail::Load<double>(p_ + 0);
        }

        /// [type double]
        double get_lon() const
        {
            return protaco::detail::Load<double>(p_ + 8);
        }

        /// [type string(16); fixed 16 bytes, set rejects longer values]
        std::string get_label() const
        {
            return protaco::detail::LoadChars(p_ + 16, 16);
        }

        /// The fields as a Spot object (copied).
        Spot Copy() const
        {
            Spot value{protaco::detail::NoStampTag()};
            std::memcpy(value.buf_.data() + kFixedOffset, p_, kFixedSize);
            return value;
        }

        /// Text for logs, the same as Spot::ToString without an envelope.
        std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const
        {
            return protaco::detail::Access::RefText(*this, style);
        }

    protected:
        const uint8_t* p_;

    private:
        friend struct protaco::detail::Access;

        template <class TWriter>
        static void VisitText(const ConstRef& self, TWriter& writer)
        {
            writer.Field("lat", self.get_lat());
            writer.Field("lon", self.get_lon());
            writer.Field("label", self.get_label());
        }
    };

    /// Spot inside another message's fixed block, written in place.
    class Ref : public ConstRef
    {
    public:
        explicit Ref(uint8_t* block)
            : ConstRef(block)
        {
        }

        /// [type double]
        void set_lat(double value)
        {
            protaco::detail::Store<double>(Data() + 0, value);
        }

        /// [type double]
        void set_lon(double value)
        {
            protaco::detail::Store<double>(Data() + 8, value);
        }

        /// [type string(16); fixed 16 bytes, set rejects longer values]
        /// Returns false, keeps the old value and writes the reason to error if longer than 16 bytes.
        bool set_label(const std::string& value, std::string* error = nullptr)
        {
            return protaco::detail::StoreChars(Data() + 16, 16, value, "label", error);
        }

        /// Copies the fields of value in.
        void Assign(const Spot& value)
        {
            std::memcpy(Data(), value.buf_.data() + kFixedOffset, kFixedSize);
        }

    private:
        uint8_t* Data() const
        {
            return const_cast<uint8_t*>(p_);
        }
    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Spot>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Spot(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Spot::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 40,   // frame header (28 + name 4 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 32,
        kAllowExtension = 0,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x2538569Au };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "lat", "lon", "label" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Spot& self, TWriter& writer)
    {
        writer.Field("lat", self.get_lat());
        writer.Field("lon", self.get_lon());
        writer.Field("label", self.get_label());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;   // stays empty: allow_extension false
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace demo
