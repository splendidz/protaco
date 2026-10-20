// Point.g.h. protacoc 0.6.0, source Point.yaml (sha1 8bf074d3ebbb). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace arrays
{

/// A 2D point. fixed: true, so it sits inline in the fixed block of other messages.
/// [version 1; schema 0x53239A70; fixed: inline in other messages; source Point.yaml]
class Point : public protaco::CMessage<Point>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Point()
        : Point(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Point(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Point::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 0, -1);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Point";
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
        return 0x53239A70u;
    }

    /// [type int; default -1]
    int32_t get_x() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 0);
    }

    void set_x(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 0, value);
    }

    /// [type int]
    int32_t get_y() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 4);
    }

    void set_y(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 4, value);
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Point>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// [type int; default -1]
        int32_t get_x() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 0);
        }

        /// [type int]
        int32_t get_y() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 4);
        }
    };

    /// Read-only Point inside another message's fixed block: fields read in place, nothing copied.
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
            return "Point";
        }

        /// [type int; default -1]
        int32_t get_x() const
        {
            return protaco::detail::Load<int32_t>(p_ + 0);
        }

        /// [type int]
        int32_t get_y() const
        {
            return protaco::detail::Load<int32_t>(p_ + 4);
        }

        /// The fields as a Point object (copied).
        Point Copy() const
        {
            Point value{protaco::detail::NoStampTag()};
            std::memcpy(value.buf_.data() + kFixedOffset, p_, kFixedSize);
            return value;
        }

        /// Text for logs, the same as Point::ToString without an envelope.
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
            writer.Field("x", self.get_x());
            writer.Field("y", self.get_y());
        }
    };

    /// Point inside another message's fixed block, written in place.
    class Ref : public ConstRef
    {
    public:
        explicit Ref(uint8_t* block)
            : ConstRef(block)
        {
        }

        /// [type int; default -1]
        void set_x(int32_t value)
        {
            protaco::detail::Store<int32_t>(Data() + 0, value);
        }

        /// [type int]
        void set_y(int32_t value)
        {
            protaco::detail::Store<int32_t>(Data() + 4, value);
        }

        /// Copies the fields of value in.
        void Assign(const Point& value)
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
    friend class protaco::CMessage<Point>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Point(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Point::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 41,   // frame header (28 + name 5 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 8,
        kAllowExtension = 0,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x53239A70u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "x", "y" };
        return protaco::detail::InList(key, kKeys, 2);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Point& self, TWriter& writer)
    {
        writer.Field("x", self.get_x());
        writer.Field("y", self.get_y());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;   // stays empty: allow_extension false
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace arrays
