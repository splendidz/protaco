// Slot.g.h. protacoc 0.6.0, source Slot.yaml (sha1 c2aa1108a1e1). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "Level.enum.h"
#include "Point.g.h"

namespace arrays
{

/// One slot of a rack: a fixed message holding a fixed message, an array of them and scalar arrays.
/// [version 1; schema 0x9D40CDF3; fixed: inline in other messages; source Slot.yaml]
class Slot : public protaco::CMessage<Slot>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Slot()
        : Slot(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Slot(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Slot::IsSchemaKey);
        protaco::detail::StoreChars(buf_.data() + kFixedOffset + 4, 8, "empty", "name", nullptr);
        protaco::detail::Access::InitBlock<Point>(buf_.data() + kFixedOffset + 12);
        for (size_t i = 0; i < 2; ++i)
            protaco::detail::Access::InitBlock<Point>(buf_.data() + kFixedOffset + 20 + 8 * i);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 36, static_cast<int32_t>(Level::Low));
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Slot";
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
        return 0x9D40CDF3u;
    }

    /// [type uint]
    uint32_t get_id() const
    {
        return protaco::detail::Load<uint32_t>(buf_.data() + kFixedOffset + 0);
    }

    void set_id(uint32_t value)
    {
        protaco::detail::Store<uint32_t>(buf_.data() + kFixedOffset + 0, value);
    }

    /// [type string(8); fixed 8 bytes, set rejects longer values; default "empty"]
    std::string get_name() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 4, 8);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 8 bytes.
    bool set_name(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 4, 8, value, "name", error);
    }

    /// [type Point; fixed message, inline 8 bytes]
    /// Read in place: a view of the Point bytes inside this message.
    Point::ConstRef get_pos() const
    {
        return Point::ConstRef(buf_.data() + kFixedOffset + 12);
    }

    /// Written in place: the Point bytes inside this message.
    Point::Ref mutable_pos()
    {
        return Point::Ref(buf_.data() + kFixedOffset + 12);
    }

    /// Copies the fields of value in.
    void set_pos(const Point& value)
    {
        protaco::detail::Access::CopyBlock(buf_.data() + kFixedOffset + 12, value);
    }

    /// [type Point[2]; 2 elements, fixed 16 bytes]
    /// Element count.
    static constexpr size_t path_size()
    {
        return 2;
    }

    /// Element read in place. index below path_size() (asserted in debug builds).
    Point::ConstRef get_path(size_t index) const
    {
        return Point::ConstRef(buf_.data() + kFixedOffset + 20 + 8 * protaco::detail::AssertIndex(index, 2));
    }

    /// Element written in place. index below path_size() (asserted in debug builds).
    Point::Ref mutable_path(size_t index)
    {
        return Point::Ref(buf_.data() + kFixedOffset + 20 + 8 * protaco::detail::AssertIndex(index, 2));
    }

    /// Copies the fields of value into the element. False and the reason in error for an index out of range.
    bool set_path(size_t index, const Point& value, std::string* error = nullptr)
    {
        if (index >= 2)
            return protaco::detail::IndexError(error, "path", index, 2);
        protaco::detail::Access::CopyBlock(buf_.data() + kFixedOffset + 20 + 8 * index, value);
        return true;
    }

    /// [type Level]
    /// Numbers this build does not know are kept as they are (ToString shows ?(n)).
    Level get_level() const
    {
        return static_cast<Level>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 36));
    }

    void set_level(Level value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 36, static_cast<int32_t>(value));
    }

    /// [type double[2]; 2 elements, fixed 16 bytes]
    /// Element count.
    static constexpr size_t temps_size()
    {
        return 2;
    }

    /// Element read in place. index below temps_size() (asserted in debug builds).
    double get_temps(size_t index) const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 40 + 8 * protaco::detail::AssertIndex(index, 2));
    }

    /// All elements, copied.
    std::array<double, 2> get_temps() const
    {
        std::array<double, 2> values;
        for (size_t i = 0; i < 2; ++i)
            values[i] = protaco::detail::Load<double>(buf_.data() + kFixedOffset + 40 + 8 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_temps(size_t index, double value, std::string* error = nullptr)
    {
        if (index >= 2)
            return protaco::detail::IndexError(error, "temps", index, 2);
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 40 + 8 * index, value);
        return true;
    }

    /// All elements.
    void set_temps(const std::array<double, 2>& values)
    {
        for (size_t i = 0; i < 2; ++i)
            protaco::detail::Store<double>(buf_.data() + kFixedOffset + 40 + 8 * i, values[i]);
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Slot>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// [type uint]
        uint32_t get_id() const
        {
            return protaco::detail::Load<uint32_t>(Fixed() + 0);
        }

        /// [type string(8); fixed 8 bytes, set rejects longer values; default "empty"]
        std::string get_name() const
        {
            return protaco::detail::LoadChars(Fixed() + 4, 8);
        }

        /// [type Point; fixed message, inline 8 bytes]
        /// Read in place: a view of the Point bytes inside this message.
        Point::ConstRef get_pos() const
        {
            return Point::ConstRef(Fixed() + 12);
        }

        /// [type Point[2]; 2 elements, fixed 16 bytes]
        /// Element count.
        static constexpr size_t path_size()
        {
            return 2;
        }

        /// Element read in place. index below path_size() (asserted in debug builds).
        Point::ConstRef get_path(size_t index) const
        {
            return Point::ConstRef(Fixed() + 20 + 8 * protaco::detail::AssertIndex(index, 2));
        }

        /// [type Level]
        Level get_level() const
        {
            return static_cast<Level>(protaco::detail::Load<int32_t>(Fixed() + 36));
        }

        /// [type double[2]; 2 elements, fixed 16 bytes]
        /// Element count.
        static constexpr size_t temps_size()
        {
            return 2;
        }

        /// Element read in place. index below temps_size() (asserted in debug builds).
        double get_temps(size_t index) const
        {
            return protaco::detail::Load<double>(Fixed() + 40 + 8 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<double, 2> get_temps() const
        {
            std::array<double, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::Load<double>(Fixed() + 40 + 8 * i);
            return values;
        }

    };

    /// Read-only Slot inside another message's fixed block: fields read in place, nothing copied.
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
            return "Slot";
        }

        /// [type uint]
        uint32_t get_id() const
        {
            return protaco::detail::Load<uint32_t>(p_ + 0);
        }

        /// [type string(8); fixed 8 bytes, set rejects longer values; default "empty"]
        std::string get_name() const
        {
            return protaco::detail::LoadChars(p_ + 4, 8);
        }

        /// [type Point; fixed message, inline 8 bytes]
        /// Read in place: a view of the Point bytes inside this message.
        Point::ConstRef get_pos() const
        {
            return Point::ConstRef(p_ + 12);
        }

        /// [type Point[2]; 2 elements, fixed 16 bytes]
        /// Element count.
        static constexpr size_t path_size()
        {
            return 2;
        }

        /// Element read in place. index below path_size() (asserted in debug builds).
        Point::ConstRef get_path(size_t index) const
        {
            return Point::ConstRef(p_ + 20 + 8 * protaco::detail::AssertIndex(index, 2));
        }

        /// [type Level]
        /// Numbers this build does not know are kept as they are (ToString shows ?(n)).
        Level get_level() const
        {
            return static_cast<Level>(protaco::detail::Load<int32_t>(p_ + 36));
        }

        /// [type double[2]; 2 elements, fixed 16 bytes]
        /// Element count.
        static constexpr size_t temps_size()
        {
            return 2;
        }

        /// Element read in place. index below temps_size() (asserted in debug builds).
        double get_temps(size_t index) const
        {
            return protaco::detail::Load<double>(p_ + 40 + 8 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<double, 2> get_temps() const
        {
            std::array<double, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::Load<double>(p_ + 40 + 8 * i);
            return values;
        }

        /// The fields as a Slot object (copied).
        Slot Copy() const
        {
            Slot value{protaco::detail::NoStampTag()};
            std::memcpy(value.buf_.data() + kFixedOffset, p_, kFixedSize);
            return value;
        }

        /// Text for logs, the same as Slot::ToString without an envelope.
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
            writer.Field("id", self.get_id());
            writer.Field("name", self.get_name());
            writer.Field("pos", self.get_pos());
            writer.Field("path", protaco::detail::RefArray<Point::ConstRef, 2, 8>{ self.p_ + 20 });
            writer.Field("level", self.get_level());
            writer.Field("temps", self.get_temps());
        }
    };

    /// Slot inside another message's fixed block, written in place.
    class Ref : public ConstRef
    {
    public:
        explicit Ref(uint8_t* block)
            : ConstRef(block)
        {
        }

        /// [type uint]
        void set_id(uint32_t value)
        {
            protaco::detail::Store<uint32_t>(Data() + 0, value);
        }

        /// [type string(8); fixed 8 bytes, set rejects longer values; default "empty"]
        /// Returns false, keeps the old value and writes the reason to error if longer than 8 bytes.
        bool set_name(const std::string& value, std::string* error = nullptr)
        {
            return protaco::detail::StoreChars(Data() + 4, 8, value, "name", error);
        }

        /// [type Point; fixed message, inline 8 bytes]
        /// Written in place: the Point bytes inside this message.
        Point::Ref mutable_pos()
        {
            return Point::Ref(Data() + 12);
        }

        /// Copies the fields of value in.
        void set_pos(const Point& value)
        {
            protaco::detail::Access::CopyBlock(Data() + 12, value);
        }

        /// [type Point[2]; 2 elements, fixed 16 bytes]
        /// Element written in place. index below path_size() (asserted in debug builds).
        Point::Ref mutable_path(size_t index)
        {
            return Point::Ref(Data() + 20 + 8 * protaco::detail::AssertIndex(index, 2));
        }

        /// Copies the fields of value into the element. False and the reason in error for an index out of range.
        bool set_path(size_t index, const Point& value, std::string* error = nullptr)
        {
            if (index >= 2)
                return protaco::detail::IndexError(error, "path", index, 2);
            protaco::detail::Access::CopyBlock(Data() + 20 + 8 * index, value);
            return true;
        }

        /// [type Level]
        void set_level(Level value)
        {
            protaco::detail::Store<int32_t>(Data() + 36, static_cast<int32_t>(value));
        }

        /// [type double[2]; 2 elements, fixed 16 bytes]
        /// Element write in place. False and the reason in error for an index out of range.
        bool set_temps(size_t index, double value, std::string* error = nullptr)
        {
            if (index >= 2)
                return protaco::detail::IndexError(error, "temps", index, 2);
            protaco::detail::Store<double>(Data() + 40 + 8 * index, value);
            return true;
        }

        /// All elements.
        void set_temps(const std::array<double, 2>& values)
        {
            for (size_t i = 0; i < 2; ++i)
                protaco::detail::Store<double>(Data() + 40 + 8 * i, values[i]);
        }

        /// Copies the fields of value in.
        void Assign(const Slot& value)
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
    friend class protaco::CMessage<Slot>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Slot(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Slot::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 40,   // frame header (28 + name 4 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 56,
        kAllowExtension = 0,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x9D40CDF3u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "id", "name", "pos", "path", "level", "temps" };
        return protaco::detail::InList(key, kKeys, 6);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Slot& self, TWriter& writer)
    {
        writer.Field("id", self.get_id());
        writer.Field("name", self.get_name());
        writer.Field("pos", self.get_pos());
        writer.Field("path", protaco::detail::RefArray<Point::ConstRef, 2, 8>{ self.buf_.data() + kFixedOffset + 20 });
        writer.Field("level", self.get_level());
        writer.Field("temps", self.get_temps());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;   // stays empty: allow_extension false
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace arrays
