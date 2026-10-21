// Board.g.h. protacoc 0.7.0, source Board.yaml (sha1 9f54d74d5bc3). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "PizzaSize.enum.h"
#include "Spot.g.h"

namespace demo
{

/// Dispatch board kept in shared memory. Every field is fixed, so programs read and write it in place.
/// [version 1; schema 0xB785B631; allow_extension false; source Board.yaml]
class Board : public protaco::CMessage<Board>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Board()
        : Board(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Board(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Board::IsSchemaKey);
        for (size_t i = 0; i < 3; ++i)
            protaco::detail::Store<double>(buf_.data() + kFixedOffset + 160 + 8 * i, 220.5);
        for (size_t i = 0; i < 3; ++i)
            protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 184 + 4 * i, static_cast<int32_t>(PizzaSize::Small));
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Board";
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
        return 0xB785B631u;
    }

    /// where riders start
    /// [type Spot; fixed message, inline 32 bytes]
    /// Read in place: a view of the Spot bytes inside this message.
    Spot::ConstRef get_depot() const
    {
        return Spot::ConstRef(buf_.data() + kFixedOffset + 0);
    }

    /// Written in place: the Spot bytes inside this message.
    Spot::Ref mutable_depot()
    {
        return Spot::Ref(buf_.data() + kFixedOffset + 0);
    }

    /// Copies the fields of value in.
    void set_depot(const Spot& value)
    {
        protaco::detail::Access::CopyBlock(buf_.data() + kFixedOffset + 0, value);
    }

    /// delivery spots
    /// [type Spot[Limits.MaxSpots]; 4 elements, fixed 128 bytes]
    /// Element count.
    static constexpr size_t spots_size()
    {
        return 4;
    }

    /// Element read in place. index below spots_size() (asserted in debug builds).
    Spot::ConstRef get_spots(size_t index) const
    {
        return Spot::ConstRef(buf_.data() + kFixedOffset + 32 + 32 * protaco::detail::AssertIndex(index, 4));
    }

    /// Element written in place. index below spots_size() (asserted in debug builds).
    Spot::Ref mutable_spots(size_t index)
    {
        return Spot::Ref(buf_.data() + kFixedOffset + 32 + 32 * protaco::detail::AssertIndex(index, 4));
    }

    /// Copies the fields of value into the element. False and the reason in error for an index out of range.
    bool set_spots(size_t index, const Spot& value, std::string* error = nullptr)
    {
        if (index >= 4)
            return protaco::detail::IndexError(error, "spots", index, 4);
        protaco::detail::Access::CopyBlock(buf_.data() + kFixedOffset + 32 + 32 * index, value);
        return true;
    }

    /// oven temperatures
    /// [type double[3]; 3 elements, fixed 24 bytes; default 220.5]
    /// Element count.
    static constexpr size_t ovens_size()
    {
        return 3;
    }

    /// Element read in place. index below ovens_size() (asserted in debug builds).
    double get_ovens(size_t index) const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 160 + 8 * protaco::detail::AssertIndex(index, 3));
    }

    /// All elements, copied.
    std::array<double, 3> get_ovens() const
    {
        std::array<double, 3> values;
        for (size_t i = 0; i < 3; ++i)
            values[i] = protaco::detail::Load<double>(buf_.data() + kFixedOffset + 160 + 8 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_ovens(size_t index, double value, std::string* error = nullptr)
    {
        if (index >= 3)
            return protaco::detail::IndexError(error, "ovens", index, 3);
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 160 + 8 * index, value);
        return true;
    }

    /// All elements.
    void set_ovens(const std::array<double, 3>& values)
    {
        for (size_t i = 0; i < 3; ++i)
            protaco::detail::Store<double>(buf_.data() + kFixedOffset + 160 + 8 * i, values[i]);
    }

    /// The length comes from an enum value: PizzaSize.Large is 3.
    /// sizes of the next pizzas
    /// [type PizzaSize[PizzaSize.Large]; 3 elements, fixed 12 bytes]
    /// Element count.
    static constexpr size_t queue_size()
    {
        return 3;
    }

    /// Element read in place. index below queue_size() (asserted in debug builds).
    PizzaSize get_queue(size_t index) const
    {
        return static_cast<PizzaSize>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 184 + 4 * protaco::detail::AssertIndex(index, 3)));
    }

    /// All elements, copied.
    std::array<PizzaSize, 3> get_queue() const
    {
        std::array<PizzaSize, 3> values;
        for (size_t i = 0; i < 3; ++i)
            values[i] = static_cast<PizzaSize>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 184 + 4 * i));
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_queue(size_t index, PizzaSize value, std::string* error = nullptr)
    {
        if (index >= 3)
            return protaco::detail::IndexError(error, "queue", index, 3);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 184 + 4 * index, static_cast<int32_t>(value));
        return true;
    }

    /// All elements.
    void set_queue(const std::array<PizzaSize, 3>& values)
    {
        for (size_t i = 0; i < 3; ++i)
            protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 184 + 4 * i, static_cast<int32_t>(values[i]));
    }

    /// riders out on a delivery
    /// [type bool[Limits.MaxRiders]; 8 elements, fixed 8 bytes]
    /// Element count.
    static constexpr size_t busy_size()
    {
        return 8;
    }

    /// Element read in place. index below busy_size() (asserted in debug builds).
    bool get_busy(size_t index) const
    {
        return protaco::detail::LoadBool(buf_.data() + kFixedOffset + 196 + 1 * protaco::detail::AssertIndex(index, 8));
    }

    /// All elements, copied.
    std::array<bool, 8> get_busy() const
    {
        std::array<bool, 8> values;
        for (size_t i = 0; i < 8; ++i)
            values[i] = protaco::detail::LoadBool(buf_.data() + kFixedOffset + 196 + 1 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_busy(size_t index, bool value, std::string* error = nullptr)
    {
        if (index >= 8)
            return protaco::detail::IndexError(error, "busy", index, 8);
        protaco::detail::StoreBool(buf_.data() + kFixedOffset + 196 + 1 * index, value);
        return true;
    }

    /// All elements.
    void set_busy(const std::array<bool, 8>& values)
    {
        for (size_t i = 0; i < 8; ++i)
            protaco::detail::StoreBool(buf_.data() + kFixedOffset + 196 + 1 * i, values[i]);
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Board>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// where riders start
        /// [type Spot; fixed message, inline 32 bytes]
        /// Read in place: a view of the Spot bytes inside this message.
        Spot::ConstRef get_depot() const
        {
            return Spot::ConstRef(Fixed() + 0);
        }

        /// delivery spots
        /// [type Spot[Limits.MaxSpots]; 4 elements, fixed 128 bytes]
        /// Element count.
        static constexpr size_t spots_size()
        {
            return 4;
        }

        /// Element read in place. index below spots_size() (asserted in debug builds).
        Spot::ConstRef get_spots(size_t index) const
        {
            return Spot::ConstRef(Fixed() + 32 + 32 * protaco::detail::AssertIndex(index, 4));
        }

        /// oven temperatures
        /// [type double[3]; 3 elements, fixed 24 bytes; default 220.5]
        /// Element count.
        static constexpr size_t ovens_size()
        {
            return 3;
        }

        /// Element read in place. index below ovens_size() (asserted in debug builds).
        double get_ovens(size_t index) const
        {
            return protaco::detail::Load<double>(Fixed() + 160 + 8 * protaco::detail::AssertIndex(index, 3));
        }

        /// All elements, copied.
        std::array<double, 3> get_ovens() const
        {
            std::array<double, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = protaco::detail::Load<double>(Fixed() + 160 + 8 * i);
            return values;
        }

        /// The length comes from an enum value: PizzaSize.Large is 3.
        /// sizes of the next pizzas
        /// [type PizzaSize[PizzaSize.Large]; 3 elements, fixed 12 bytes]
        /// Element count.
        static constexpr size_t queue_size()
        {
            return 3;
        }

        /// Element read in place. index below queue_size() (asserted in debug builds).
        PizzaSize get_queue(size_t index) const
        {
            return static_cast<PizzaSize>(protaco::detail::Load<int32_t>(Fixed() + 184 + 4 * protaco::detail::AssertIndex(index, 3)));
        }

        /// All elements, copied.
        std::array<PizzaSize, 3> get_queue() const
        {
            std::array<PizzaSize, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = static_cast<PizzaSize>(protaco::detail::Load<int32_t>(Fixed() + 184 + 4 * i));
            return values;
        }

        /// riders out on a delivery
        /// [type bool[Limits.MaxRiders]; 8 elements, fixed 8 bytes]
        /// Element count.
        static constexpr size_t busy_size()
        {
            return 8;
        }

        /// Element read in place. index below busy_size() (asserted in debug builds).
        bool get_busy(size_t index) const
        {
            return protaco::detail::LoadBool(Fixed() + 196 + 1 * protaco::detail::AssertIndex(index, 8));
        }

        /// All elements, copied.
        std::array<bool, 8> get_busy() const
        {
            std::array<bool, 8> values;
            for (size_t i = 0; i < 8; ++i)
                values[i] = protaco::detail::LoadBool(Fixed() + 196 + 1 * i);
            return values;
        }

    };

    /// Fixed fields read and written in place in a frame in memory the caller owns (protaco::TryMutableView<Board>),
    /// for example shared memory. Nothing is copied. Valid after a successful TryMutableView while the memory lives.
    /// Writes are plain stores: concurrent readers and writers need the caller's synchronization.
    class MutableView : public protaco::CMutableView
    {
    public:
        /// Bytes of a new Board frame (header and fixed block): the memory to fill with Serialize() and give TryMutableView.
        static constexpr size_t FrameSize()
        {
            return 245;
        }

        /// where riders start
        /// [type Spot; fixed message, inline 32 bytes]
        /// Read in place: a view of the Spot bytes inside this message.
        Spot::ConstRef get_depot() const
        {
            return Spot::ConstRef(Fixed() + 0);
        }

        /// Written in place: the Spot bytes inside this message.
        Spot::Ref mutable_depot()
        {
            return Spot::Ref(Fixed() + 0);
        }

        /// Copies the fields of value in.
        void set_depot(const Spot& value)
        {
            protaco::detail::Access::CopyBlock(Fixed() + 0, value);
        }

        /// delivery spots
        /// [type Spot[Limits.MaxSpots]; 4 elements, fixed 128 bytes]
        /// Element count.
        static constexpr size_t spots_size()
        {
            return 4;
        }

        /// Element read in place. index below spots_size() (asserted in debug builds).
        Spot::ConstRef get_spots(size_t index) const
        {
            return Spot::ConstRef(Fixed() + 32 + 32 * protaco::detail::AssertIndex(index, 4));
        }

        /// Element written in place. index below spots_size() (asserted in debug builds).
        Spot::Ref mutable_spots(size_t index)
        {
            return Spot::Ref(Fixed() + 32 + 32 * protaco::detail::AssertIndex(index, 4));
        }

        /// Copies the fields of value into the element. False and the reason in error for an index out of range.
        bool set_spots(size_t index, const Spot& value, std::string* error = nullptr)
        {
            if (index >= 4)
                return protaco::detail::IndexError(error, "spots", index, 4);
            protaco::detail::Access::CopyBlock(Fixed() + 32 + 32 * index, value);
            return true;
        }

        /// oven temperatures
        /// [type double[3]; 3 elements, fixed 24 bytes; default 220.5]
        /// Element count.
        static constexpr size_t ovens_size()
        {
            return 3;
        }

        /// Element read in place. index below ovens_size() (asserted in debug builds).
        double get_ovens(size_t index) const
        {
            return protaco::detail::Load<double>(Fixed() + 160 + 8 * protaco::detail::AssertIndex(index, 3));
        }

        /// All elements, copied.
        std::array<double, 3> get_ovens() const
        {
            std::array<double, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = protaco::detail::Load<double>(Fixed() + 160 + 8 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_ovens(size_t index, double value, std::string* error = nullptr)
        {
            if (index >= 3)
                return protaco::detail::IndexError(error, "ovens", index, 3);
            protaco::detail::Store<double>(Fixed() + 160 + 8 * index, value);
            return true;
        }

        /// All elements.
        void set_ovens(const std::array<double, 3>& values)
        {
            for (size_t i = 0; i < 3; ++i)
                protaco::detail::Store<double>(Fixed() + 160 + 8 * i, values[i]);
        }

        /// The length comes from an enum value: PizzaSize.Large is 3.
        /// sizes of the next pizzas
        /// [type PizzaSize[PizzaSize.Large]; 3 elements, fixed 12 bytes]
        /// Element count.
        static constexpr size_t queue_size()
        {
            return 3;
        }

        /// Element read in place. index below queue_size() (asserted in debug builds).
        PizzaSize get_queue(size_t index) const
        {
            return static_cast<PizzaSize>(protaco::detail::Load<int32_t>(Fixed() + 184 + 4 * protaco::detail::AssertIndex(index, 3)));
        }

        /// All elements, copied.
        std::array<PizzaSize, 3> get_queue() const
        {
            std::array<PizzaSize, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = static_cast<PizzaSize>(protaco::detail::Load<int32_t>(Fixed() + 184 + 4 * i));
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_queue(size_t index, PizzaSize value, std::string* error = nullptr)
        {
            if (index >= 3)
                return protaco::detail::IndexError(error, "queue", index, 3);
            protaco::detail::Store<int32_t>(Fixed() + 184 + 4 * index, static_cast<int32_t>(value));
            return true;
        }

        /// All elements.
        void set_queue(const std::array<PizzaSize, 3>& values)
        {
            for (size_t i = 0; i < 3; ++i)
                protaco::detail::Store<int32_t>(Fixed() + 184 + 4 * i, static_cast<int32_t>(values[i]));
        }

        /// riders out on a delivery
        /// [type bool[Limits.MaxRiders]; 8 elements, fixed 8 bytes]
        /// Element count.
        static constexpr size_t busy_size()
        {
            return 8;
        }

        /// Element read in place. index below busy_size() (asserted in debug builds).
        bool get_busy(size_t index) const
        {
            return protaco::detail::LoadBool(Fixed() + 196 + 1 * protaco::detail::AssertIndex(index, 8));
        }

        /// All elements, copied.
        std::array<bool, 8> get_busy() const
        {
            std::array<bool, 8> values;
            for (size_t i = 0; i < 8; ++i)
                values[i] = protaco::detail::LoadBool(Fixed() + 196 + 1 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_busy(size_t index, bool value, std::string* error = nullptr)
        {
            if (index >= 8)
                return protaco::detail::IndexError(error, "busy", index, 8);
            protaco::detail::StoreBool(Fixed() + 196 + 1 * index, value);
            return true;
        }

        /// All elements.
        void set_busy(const std::array<bool, 8>& values)
        {
            for (size_t i = 0; i < 8; ++i)
                protaco::detail::StoreBool(Fixed() + 196 + 1 * i, values[i]);
        }

    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Board>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Board(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Board::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 41,   // frame header (28 + name 5 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 204,
        kAllowExtension = 0,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xB785B631u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "depot", "spots", "ovens", "queue", "busy" };
        return protaco::detail::InList(key, kKeys, 5);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Board& self, TWriter& writer)
    {
        writer.Field("depot", self.get_depot());
        writer.Field("spots", protaco::detail::RefArray<Spot::ConstRef, 4, 32>{ self.buf_.data() + kFixedOffset + 32 });
        writer.Field("ovens", self.get_ovens());
        writer.Field("queue", self.get_queue());
        writer.Field("busy", self.get_busy());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;   // stays empty: allow_extension false
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace demo
