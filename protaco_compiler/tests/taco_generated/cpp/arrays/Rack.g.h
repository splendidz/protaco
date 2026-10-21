// Rack.g.h. protacoc 0.7.0, source Rack.yaml (sha1 fd710f61bd22). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "Point.g.h"
#include "Slot.g.h"

namespace arrays
{

/// A rack of slots. Not fixed: true itself, but every field is fixed, so TryView reads it in place.
/// [version 1; schema 0xD3E62388; allow_extension false; source Rack.yaml]
class Rack : public protaco::CMessage<Rack>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Rack()
        : Rack(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Rack(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Rack::IsSchemaKey);
        protaco::detail::Access::InitBlock<Point>(buf_.data() + kFixedOffset + 0);
        for (size_t i = 0; i < 4; ++i)
            protaco::detail::Access::InitBlock<Slot>(buf_.data() + kFixedOffset + 8 + 56 * i);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Rack";
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
        return 0xD3E62388u;
    }

    /// [type Point; fixed message, inline 8 bytes]
    /// Read in place: a view of the Point bytes inside this message.
    Point::ConstRef get_origin() const
    {
        return Point::ConstRef(buf_.data() + kFixedOffset + 0);
    }

    /// Written in place: the Point bytes inside this message.
    Point::Ref mutable_origin()
    {
        return Point::Ref(buf_.data() + kFixedOffset + 0);
    }

    /// Copies the fields of value in.
    void set_origin(const Point& value)
    {
        protaco::detail::Access::CopyBlock(buf_.data() + kFixedOffset + 0, value);
    }

    /// [type Slot[Sizes.Slots]; 4 elements, fixed 224 bytes]
    /// Element count.
    static constexpr size_t slots_size()
    {
        return 4;
    }

    /// Element read in place. index below slots_size() (asserted in debug builds).
    Slot::ConstRef get_slots(size_t index) const
    {
        return Slot::ConstRef(buf_.data() + kFixedOffset + 8 + 56 * protaco::detail::AssertIndex(index, 4));
    }

    /// Element written in place. index below slots_size() (asserted in debug builds).
    Slot::Ref mutable_slots(size_t index)
    {
        return Slot::Ref(buf_.data() + kFixedOffset + 8 + 56 * protaco::detail::AssertIndex(index, 4));
    }

    /// Copies the fields of value into the element. False and the reason in error for an index out of range.
    bool set_slots(size_t index, const Slot& value, std::string* error = nullptr)
    {
        if (index >= 4)
            return protaco::detail::IndexError(error, "slots", index, 4);
        protaco::detail::Access::CopyBlock(buf_.data() + kFixedOffset + 8 + 56 * index, value);
        return true;
    }

    /// [type int]
    int32_t get_count() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 232);
    }

    void set_count(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 232, value);
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Rack>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// [type Point; fixed message, inline 8 bytes]
        /// Read in place: a view of the Point bytes inside this message.
        Point::ConstRef get_origin() const
        {
            return Point::ConstRef(Fixed() + 0);
        }

        /// [type Slot[Sizes.Slots]; 4 elements, fixed 224 bytes]
        /// Element count.
        static constexpr size_t slots_size()
        {
            return 4;
        }

        /// Element read in place. index below slots_size() (asserted in debug builds).
        Slot::ConstRef get_slots(size_t index) const
        {
            return Slot::ConstRef(Fixed() + 8 + 56 * protaco::detail::AssertIndex(index, 4));
        }

        /// [type int]
        int32_t get_count() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 232);
        }
    };

    /// Fixed fields read and written in place in a frame in memory the caller owns (protaco::TryMutableView<Rack>),
    /// for example shared memory. Nothing is copied. Valid after a successful TryMutableView while the memory lives.
    /// Writes are plain stores: concurrent readers and writers need the caller's synchronization.
    class MutableView : public protaco::CMutableView
    {
    public:
        /// Bytes of a new Rack frame (header and fixed block): the memory to fill with Serialize() and give TryMutableView.
        static constexpr size_t FrameSize()
        {
            return 276;
        }

        /// [type Point; fixed message, inline 8 bytes]
        /// Read in place: a view of the Point bytes inside this message.
        Point::ConstRef get_origin() const
        {
            return Point::ConstRef(Fixed() + 0);
        }

        /// Written in place: the Point bytes inside this message.
        Point::Ref mutable_origin()
        {
            return Point::Ref(Fixed() + 0);
        }

        /// Copies the fields of value in.
        void set_origin(const Point& value)
        {
            protaco::detail::Access::CopyBlock(Fixed() + 0, value);
        }

        /// [type Slot[Sizes.Slots]; 4 elements, fixed 224 bytes]
        /// Element count.
        static constexpr size_t slots_size()
        {
            return 4;
        }

        /// Element read in place. index below slots_size() (asserted in debug builds).
        Slot::ConstRef get_slots(size_t index) const
        {
            return Slot::ConstRef(Fixed() + 8 + 56 * protaco::detail::AssertIndex(index, 4));
        }

        /// Element written in place. index below slots_size() (asserted in debug builds).
        Slot::Ref mutable_slots(size_t index)
        {
            return Slot::Ref(Fixed() + 8 + 56 * protaco::detail::AssertIndex(index, 4));
        }

        /// Copies the fields of value into the element. False and the reason in error for an index out of range.
        bool set_slots(size_t index, const Slot& value, std::string* error = nullptr)
        {
            if (index >= 4)
                return protaco::detail::IndexError(error, "slots", index, 4);
            protaco::detail::Access::CopyBlock(Fixed() + 8 + 56 * index, value);
            return true;
        }

        /// [type int]
        int32_t get_count() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 232);
        }

        void set_count(int32_t value)
        {
            protaco::detail::Store<int32_t>(Fixed() + 232, value);
        }

    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Rack>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Rack(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Rack::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 40,   // frame header (28 + name 4 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 236,
        kAllowExtension = 0,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xD3E62388u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "origin", "slots", "count" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Rack& self, TWriter& writer)
    {
        writer.Field("origin", self.get_origin());
        writer.Field("slots", protaco::detail::RefArray<Slot::ConstRef, 4, 56>{ self.buf_.data() + kFixedOffset + 8 });
        writer.Field("count", self.get_count());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;   // stays empty: allow_extension false
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace arrays
