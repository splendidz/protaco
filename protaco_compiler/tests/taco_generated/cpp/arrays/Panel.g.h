// Panel.g.h. protacoc 0.7.0, source Panel.yaml (sha1 236d0ca5085f). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "Level.enum.h"

namespace arrays
{

/// Fixed-length arrays of every element kind. Every field is fixed, so TryView reads it in place.
/// [version 1; schema 0x71A7E9E9; allow_extension false; source Panel.yaml]
class Panel : public protaco::CMessage<Panel>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Panel()
        : Panel(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Panel(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Panel::IsSchemaKey);
        for (size_t i = 0; i < 4; ++i)
            protaco::detail::Store<double>(buf_.data() + kFixedOffset + 0 + 8 * i, 20.5);
        for (size_t i = 0; i < 4; ++i)
            protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 32 + 4 * i, -1);
        for (size_t i = 0; i < 2; ++i)
            protaco::detail::StoreChars(buf_.data() + kFixedOffset + 98 + 8 * i, 8, "none", "names", nullptr);
        for (size_t i = 0; i < 3; ++i)
            protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 114 + 4 * i, static_cast<int32_t>(Level::Low));
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Panel";
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
        return 0x71A7E9E9u;
    }

    /// [type double[Sizes.Slots]; 4 elements, fixed 32 bytes; default 20.5]
    /// Element count.
    static constexpr size_t temps_size()
    {
        return 4;
    }

    /// Element read in place. index below temps_size() (asserted in debug builds).
    double get_temps(size_t index) const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 0 + 8 * protaco::detail::AssertIndex(index, 4));
    }

    /// All elements, copied.
    std::array<double, 4> get_temps() const
    {
        std::array<double, 4> values;
        for (size_t i = 0; i < 4; ++i)
            values[i] = protaco::detail::Load<double>(buf_.data() + kFixedOffset + 0 + 8 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_temps(size_t index, double value, std::string* error = nullptr)
    {
        if (index >= 4)
            return protaco::detail::IndexError(error, "temps", index, 4);
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 0 + 8 * index, value);
        return true;
    }

    /// All elements.
    void set_temps(const std::array<double, 4>& values)
    {
        for (size_t i = 0; i < 4; ++i)
            protaco::detail::Store<double>(buf_.data() + kFixedOffset + 0 + 8 * i, values[i]);
    }

    /// [type int[Sizes.Slots]; 4 elements, fixed 16 bytes; default -1]
    /// Element count.
    static constexpr size_t counts_size()
    {
        return 4;
    }

    /// Element read in place. index below counts_size() (asserted in debug builds).
    int32_t get_counts(size_t index) const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 32 + 4 * protaco::detail::AssertIndex(index, 4));
    }

    /// All elements, copied.
    std::array<int32_t, 4> get_counts() const
    {
        std::array<int32_t, 4> values;
        for (size_t i = 0; i < 4; ++i)
            values[i] = protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 32 + 4 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_counts(size_t index, int32_t value, std::string* error = nullptr)
    {
        if (index >= 4)
            return protaco::detail::IndexError(error, "counts", index, 4);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 32 + 4 * index, value);
        return true;
    }

    /// All elements.
    void set_counts(const std::array<int32_t, 4>& values)
    {
        for (size_t i = 0; i < 4; ++i)
            protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 32 + 4 * i, values[i]);
    }

    /// [type uint64[3]; 3 elements, fixed 24 bytes]
    /// Element count.
    static constexpr size_t ids_size()
    {
        return 3;
    }

    /// Element read in place. index below ids_size() (asserted in debug builds).
    uint64_t get_ids(size_t index) const
    {
        return protaco::detail::Load<uint64_t>(buf_.data() + kFixedOffset + 48 + 8 * protaco::detail::AssertIndex(index, 3));
    }

    /// All elements, copied.
    std::array<uint64_t, 3> get_ids() const
    {
        std::array<uint64_t, 3> values;
        for (size_t i = 0; i < 3; ++i)
            values[i] = protaco::detail::Load<uint64_t>(buf_.data() + kFixedOffset + 48 + 8 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_ids(size_t index, uint64_t value, std::string* error = nullptr)
    {
        if (index >= 3)
            return protaco::detail::IndexError(error, "ids", index, 3);
        protaco::detail::Store<uint64_t>(buf_.data() + kFixedOffset + 48 + 8 * index, value);
        return true;
    }

    /// All elements.
    void set_ids(const std::array<uint64_t, 3>& values)
    {
        for (size_t i = 0; i < 3; ++i)
            protaco::detail::Store<uint64_t>(buf_.data() + kFixedOffset + 48 + 8 * i, values[i]);
    }

    /// [type uint[2]; 2 elements, fixed 8 bytes]
    /// Element count.
    static constexpr size_t masks_size()
    {
        return 2;
    }

    /// Element read in place. index below masks_size() (asserted in debug builds).
    uint32_t get_masks(size_t index) const
    {
        return protaco::detail::Load<uint32_t>(buf_.data() + kFixedOffset + 72 + 4 * protaco::detail::AssertIndex(index, 2));
    }

    /// All elements, copied.
    std::array<uint32_t, 2> get_masks() const
    {
        std::array<uint32_t, 2> values;
        for (size_t i = 0; i < 2; ++i)
            values[i] = protaco::detail::Load<uint32_t>(buf_.data() + kFixedOffset + 72 + 4 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_masks(size_t index, uint32_t value, std::string* error = nullptr)
    {
        if (index >= 2)
            return protaco::detail::IndexError(error, "masks", index, 2);
        protaco::detail::Store<uint32_t>(buf_.data() + kFixedOffset + 72 + 4 * index, value);
        return true;
    }

    /// All elements.
    void set_masks(const std::array<uint32_t, 2>& values)
    {
        for (size_t i = 0; i < 2; ++i)
            protaco::detail::Store<uint32_t>(buf_.data() + kFixedOffset + 72 + 4 * i, values[i]);
    }

    /// [type int64[2]; 2 elements, fixed 16 bytes]
    /// Element count.
    static constexpr size_t ticks_size()
    {
        return 2;
    }

    /// Element read in place. index below ticks_size() (asserted in debug builds).
    int64_t get_ticks(size_t index) const
    {
        return protaco::detail::Load<int64_t>(buf_.data() + kFixedOffset + 80 + 8 * protaco::detail::AssertIndex(index, 2));
    }

    /// All elements, copied.
    std::array<int64_t, 2> get_ticks() const
    {
        std::array<int64_t, 2> values;
        for (size_t i = 0; i < 2; ++i)
            values[i] = protaco::detail::Load<int64_t>(buf_.data() + kFixedOffset + 80 + 8 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_ticks(size_t index, int64_t value, std::string* error = nullptr)
    {
        if (index >= 2)
            return protaco::detail::IndexError(error, "ticks", index, 2);
        protaco::detail::Store<int64_t>(buf_.data() + kFixedOffset + 80 + 8 * index, value);
        return true;
    }

    /// All elements.
    void set_ticks(const std::array<int64_t, 2>& values)
    {
        for (size_t i = 0; i < 2; ++i)
            protaco::detail::Store<int64_t>(buf_.data() + kFixedOffset + 80 + 8 * i, values[i]);
    }

    /// [type bool[2]; 2 elements, fixed 2 bytes]
    /// Element count.
    static constexpr size_t flags_size()
    {
        return 2;
    }

    /// Element read in place. index below flags_size() (asserted in debug builds).
    bool get_flags(size_t index) const
    {
        return protaco::detail::LoadBool(buf_.data() + kFixedOffset + 96 + 1 * protaco::detail::AssertIndex(index, 2));
    }

    /// All elements, copied.
    std::array<bool, 2> get_flags() const
    {
        std::array<bool, 2> values;
        for (size_t i = 0; i < 2; ++i)
            values[i] = protaco::detail::LoadBool(buf_.data() + kFixedOffset + 96 + 1 * i);
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_flags(size_t index, bool value, std::string* error = nullptr)
    {
        if (index >= 2)
            return protaco::detail::IndexError(error, "flags", index, 2);
        protaco::detail::StoreBool(buf_.data() + kFixedOffset + 96 + 1 * index, value);
        return true;
    }

    /// All elements.
    void set_flags(const std::array<bool, 2>& values)
    {
        for (size_t i = 0; i < 2; ++i)
            protaco::detail::StoreBool(buf_.data() + kFixedOffset + 96 + 1 * i, values[i]);
    }

    /// [type string(8)[Sizes.Names]; 2 elements, fixed 16 bytes, set rejects values longer than 8 bytes; default none]
    /// Element count.
    static constexpr size_t names_size()
    {
        return 2;
    }

    /// Element read in place. index below names_size() (asserted in debug builds).
    std::string get_names(size_t index) const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 98 + 8 * protaco::detail::AssertIndex(index, 2), 8);
    }

    /// All elements, copied.
    std::array<std::string, 2> get_names() const
    {
        std::array<std::string, 2> values;
        for (size_t i = 0; i < 2; ++i)
            values[i] = protaco::detail::LoadChars(buf_.data() + kFixedOffset + 98 + 8 * i, 8);
        return values;
    }

    /// Element write in place. False, the old value kept and the reason in error for an index out of range or a value longer than 8 bytes.
    bool set_names(size_t index, const std::string& value, std::string* error = nullptr)
    {
        if (index >= 2)
            return protaco::detail::IndexError(error, "names", index, 2);
        return protaco::detail::StoreCharsAt(buf_.data() + kFixedOffset + 98 + 8 * index, 8, value, "names", index, error);
    }

    /// All elements. False and nothing written if any value is longer than 8 bytes.
    bool set_names(const std::array<std::string, 2>& values, std::string* error = nullptr)
    {
        for (size_t i = 0; i < 2; ++i)
        {
            if (values[i].size() > 8)
                return protaco::detail::StoreCharsAt(buf_.data() + kFixedOffset + 98 + 8 * i, 8, values[i], "names", i, error);
        }
        for (size_t i = 0; i < 2; ++i)
            protaco::detail::StoreChars(buf_.data() + kFixedOffset + 98 + 8 * i, 8, values[i], "names", nullptr);
        return true;
    }

    /// [type Level[Level.Count]; 3 elements, fixed 12 bytes]
    /// Element count.
    static constexpr size_t levels_size()
    {
        return 3;
    }

    /// Element read in place. index below levels_size() (asserted in debug builds).
    Level get_levels(size_t index) const
    {
        return static_cast<Level>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 114 + 4 * protaco::detail::AssertIndex(index, 3)));
    }

    /// All elements, copied.
    std::array<Level, 3> get_levels() const
    {
        std::array<Level, 3> values;
        for (size_t i = 0; i < 3; ++i)
            values[i] = static_cast<Level>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 114 + 4 * i));
        return values;
    }

    /// Element write in place. False and the reason in error for an index out of range.
    bool set_levels(size_t index, Level value, std::string* error = nullptr)
    {
        if (index >= 3)
            return protaco::detail::IndexError(error, "levels", index, 3);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 114 + 4 * index, static_cast<int32_t>(value));
        return true;
    }

    /// All elements.
    void set_levels(const std::array<Level, 3>& values)
    {
        for (size_t i = 0; i < 3; ++i)
            protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 114 + 4 * i, static_cast<int32_t>(values[i]));
    }

    /// [type int]
    int32_t get_total() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 126);
    }

    void set_total(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 126, value);
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Panel>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// [type double[Sizes.Slots]; 4 elements, fixed 32 bytes; default 20.5]
        /// Element count.
        static constexpr size_t temps_size()
        {
            return 4;
        }

        /// Element read in place. index below temps_size() (asserted in debug builds).
        double get_temps(size_t index) const
        {
            return protaco::detail::Load<double>(Fixed() + 0 + 8 * protaco::detail::AssertIndex(index, 4));
        }

        /// All elements, copied.
        std::array<double, 4> get_temps() const
        {
            std::array<double, 4> values;
            for (size_t i = 0; i < 4; ++i)
                values[i] = protaco::detail::Load<double>(Fixed() + 0 + 8 * i);
            return values;
        }

        /// [type int[Sizes.Slots]; 4 elements, fixed 16 bytes; default -1]
        /// Element count.
        static constexpr size_t counts_size()
        {
            return 4;
        }

        /// Element read in place. index below counts_size() (asserted in debug builds).
        int32_t get_counts(size_t index) const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 32 + 4 * protaco::detail::AssertIndex(index, 4));
        }

        /// All elements, copied.
        std::array<int32_t, 4> get_counts() const
        {
            std::array<int32_t, 4> values;
            for (size_t i = 0; i < 4; ++i)
                values[i] = protaco::detail::Load<int32_t>(Fixed() + 32 + 4 * i);
            return values;
        }

        /// [type uint64[3]; 3 elements, fixed 24 bytes]
        /// Element count.
        static constexpr size_t ids_size()
        {
            return 3;
        }

        /// Element read in place. index below ids_size() (asserted in debug builds).
        uint64_t get_ids(size_t index) const
        {
            return protaco::detail::Load<uint64_t>(Fixed() + 48 + 8 * protaco::detail::AssertIndex(index, 3));
        }

        /// All elements, copied.
        std::array<uint64_t, 3> get_ids() const
        {
            std::array<uint64_t, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = protaco::detail::Load<uint64_t>(Fixed() + 48 + 8 * i);
            return values;
        }

        /// [type uint[2]; 2 elements, fixed 8 bytes]
        /// Element count.
        static constexpr size_t masks_size()
        {
            return 2;
        }

        /// Element read in place. index below masks_size() (asserted in debug builds).
        uint32_t get_masks(size_t index) const
        {
            return protaco::detail::Load<uint32_t>(Fixed() + 72 + 4 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<uint32_t, 2> get_masks() const
        {
            std::array<uint32_t, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::Load<uint32_t>(Fixed() + 72 + 4 * i);
            return values;
        }

        /// [type int64[2]; 2 elements, fixed 16 bytes]
        /// Element count.
        static constexpr size_t ticks_size()
        {
            return 2;
        }

        /// Element read in place. index below ticks_size() (asserted in debug builds).
        int64_t get_ticks(size_t index) const
        {
            return protaco::detail::Load<int64_t>(Fixed() + 80 + 8 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<int64_t, 2> get_ticks() const
        {
            std::array<int64_t, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::Load<int64_t>(Fixed() + 80 + 8 * i);
            return values;
        }

        /// [type bool[2]; 2 elements, fixed 2 bytes]
        /// Element count.
        static constexpr size_t flags_size()
        {
            return 2;
        }

        /// Element read in place. index below flags_size() (asserted in debug builds).
        bool get_flags(size_t index) const
        {
            return protaco::detail::LoadBool(Fixed() + 96 + 1 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<bool, 2> get_flags() const
        {
            std::array<bool, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::LoadBool(Fixed() + 96 + 1 * i);
            return values;
        }

        /// [type string(8)[Sizes.Names]; 2 elements, fixed 16 bytes, set rejects values longer than 8 bytes; default none]
        /// Element count.
        static constexpr size_t names_size()
        {
            return 2;
        }

        /// Element read in place. index below names_size() (asserted in debug builds).
        std::string get_names(size_t index) const
        {
            return protaco::detail::LoadChars(Fixed() + 98 + 8 * protaco::detail::AssertIndex(index, 2), 8);
        }

        /// All elements, copied.
        std::array<std::string, 2> get_names() const
        {
            std::array<std::string, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::LoadChars(Fixed() + 98 + 8 * i, 8);
            return values;
        }

        /// [type Level[Level.Count]; 3 elements, fixed 12 bytes]
        /// Element count.
        static constexpr size_t levels_size()
        {
            return 3;
        }

        /// Element read in place. index below levels_size() (asserted in debug builds).
        Level get_levels(size_t index) const
        {
            return static_cast<Level>(protaco::detail::Load<int32_t>(Fixed() + 114 + 4 * protaco::detail::AssertIndex(index, 3)));
        }

        /// All elements, copied.
        std::array<Level, 3> get_levels() const
        {
            std::array<Level, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = static_cast<Level>(protaco::detail::Load<int32_t>(Fixed() + 114 + 4 * i));
            return values;
        }

        /// [type int]
        int32_t get_total() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 126);
        }
    };

    /// Fixed fields read and written in place in a frame in memory the caller owns (protaco::TryMutableView<Panel>),
    /// for example shared memory. Nothing is copied. Valid after a successful TryMutableView while the memory lives.
    /// Writes are plain stores: concurrent readers and writers need the caller's synchronization.
    class MutableView : public protaco::CMutableView
    {
    public:
        /// Bytes of a new Panel frame (header and fixed block): the memory to fill with Serialize() and give TryMutableView.
        static constexpr size_t FrameSize()
        {
            return 171;
        }

        /// [type double[Sizes.Slots]; 4 elements, fixed 32 bytes; default 20.5]
        /// Element count.
        static constexpr size_t temps_size()
        {
            return 4;
        }

        /// Element read in place. index below temps_size() (asserted in debug builds).
        double get_temps(size_t index) const
        {
            return protaco::detail::Load<double>(Fixed() + 0 + 8 * protaco::detail::AssertIndex(index, 4));
        }

        /// All elements, copied.
        std::array<double, 4> get_temps() const
        {
            std::array<double, 4> values;
            for (size_t i = 0; i < 4; ++i)
                values[i] = protaco::detail::Load<double>(Fixed() + 0 + 8 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_temps(size_t index, double value, std::string* error = nullptr)
        {
            if (index >= 4)
                return protaco::detail::IndexError(error, "temps", index, 4);
            protaco::detail::Store<double>(Fixed() + 0 + 8 * index, value);
            return true;
        }

        /// All elements.
        void set_temps(const std::array<double, 4>& values)
        {
            for (size_t i = 0; i < 4; ++i)
                protaco::detail::Store<double>(Fixed() + 0 + 8 * i, values[i]);
        }

        /// [type int[Sizes.Slots]; 4 elements, fixed 16 bytes; default -1]
        /// Element count.
        static constexpr size_t counts_size()
        {
            return 4;
        }

        /// Element read in place. index below counts_size() (asserted in debug builds).
        int32_t get_counts(size_t index) const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 32 + 4 * protaco::detail::AssertIndex(index, 4));
        }

        /// All elements, copied.
        std::array<int32_t, 4> get_counts() const
        {
            std::array<int32_t, 4> values;
            for (size_t i = 0; i < 4; ++i)
                values[i] = protaco::detail::Load<int32_t>(Fixed() + 32 + 4 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_counts(size_t index, int32_t value, std::string* error = nullptr)
        {
            if (index >= 4)
                return protaco::detail::IndexError(error, "counts", index, 4);
            protaco::detail::Store<int32_t>(Fixed() + 32 + 4 * index, value);
            return true;
        }

        /// All elements.
        void set_counts(const std::array<int32_t, 4>& values)
        {
            for (size_t i = 0; i < 4; ++i)
                protaco::detail::Store<int32_t>(Fixed() + 32 + 4 * i, values[i]);
        }

        /// [type uint64[3]; 3 elements, fixed 24 bytes]
        /// Element count.
        static constexpr size_t ids_size()
        {
            return 3;
        }

        /// Element read in place. index below ids_size() (asserted in debug builds).
        uint64_t get_ids(size_t index) const
        {
            return protaco::detail::Load<uint64_t>(Fixed() + 48 + 8 * protaco::detail::AssertIndex(index, 3));
        }

        /// All elements, copied.
        std::array<uint64_t, 3> get_ids() const
        {
            std::array<uint64_t, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = protaco::detail::Load<uint64_t>(Fixed() + 48 + 8 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_ids(size_t index, uint64_t value, std::string* error = nullptr)
        {
            if (index >= 3)
                return protaco::detail::IndexError(error, "ids", index, 3);
            protaco::detail::Store<uint64_t>(Fixed() + 48 + 8 * index, value);
            return true;
        }

        /// All elements.
        void set_ids(const std::array<uint64_t, 3>& values)
        {
            for (size_t i = 0; i < 3; ++i)
                protaco::detail::Store<uint64_t>(Fixed() + 48 + 8 * i, values[i]);
        }

        /// [type uint[2]; 2 elements, fixed 8 bytes]
        /// Element count.
        static constexpr size_t masks_size()
        {
            return 2;
        }

        /// Element read in place. index below masks_size() (asserted in debug builds).
        uint32_t get_masks(size_t index) const
        {
            return protaco::detail::Load<uint32_t>(Fixed() + 72 + 4 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<uint32_t, 2> get_masks() const
        {
            std::array<uint32_t, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::Load<uint32_t>(Fixed() + 72 + 4 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_masks(size_t index, uint32_t value, std::string* error = nullptr)
        {
            if (index >= 2)
                return protaco::detail::IndexError(error, "masks", index, 2);
            protaco::detail::Store<uint32_t>(Fixed() + 72 + 4 * index, value);
            return true;
        }

        /// All elements.
        void set_masks(const std::array<uint32_t, 2>& values)
        {
            for (size_t i = 0; i < 2; ++i)
                protaco::detail::Store<uint32_t>(Fixed() + 72 + 4 * i, values[i]);
        }

        /// [type int64[2]; 2 elements, fixed 16 bytes]
        /// Element count.
        static constexpr size_t ticks_size()
        {
            return 2;
        }

        /// Element read in place. index below ticks_size() (asserted in debug builds).
        int64_t get_ticks(size_t index) const
        {
            return protaco::detail::Load<int64_t>(Fixed() + 80 + 8 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<int64_t, 2> get_ticks() const
        {
            std::array<int64_t, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::Load<int64_t>(Fixed() + 80 + 8 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_ticks(size_t index, int64_t value, std::string* error = nullptr)
        {
            if (index >= 2)
                return protaco::detail::IndexError(error, "ticks", index, 2);
            protaco::detail::Store<int64_t>(Fixed() + 80 + 8 * index, value);
            return true;
        }

        /// All elements.
        void set_ticks(const std::array<int64_t, 2>& values)
        {
            for (size_t i = 0; i < 2; ++i)
                protaco::detail::Store<int64_t>(Fixed() + 80 + 8 * i, values[i]);
        }

        /// [type bool[2]; 2 elements, fixed 2 bytes]
        /// Element count.
        static constexpr size_t flags_size()
        {
            return 2;
        }

        /// Element read in place. index below flags_size() (asserted in debug builds).
        bool get_flags(size_t index) const
        {
            return protaco::detail::LoadBool(Fixed() + 96 + 1 * protaco::detail::AssertIndex(index, 2));
        }

        /// All elements, copied.
        std::array<bool, 2> get_flags() const
        {
            std::array<bool, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::LoadBool(Fixed() + 96 + 1 * i);
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_flags(size_t index, bool value, std::string* error = nullptr)
        {
            if (index >= 2)
                return protaco::detail::IndexError(error, "flags", index, 2);
            protaco::detail::StoreBool(Fixed() + 96 + 1 * index, value);
            return true;
        }

        /// All elements.
        void set_flags(const std::array<bool, 2>& values)
        {
            for (size_t i = 0; i < 2; ++i)
                protaco::detail::StoreBool(Fixed() + 96 + 1 * i, values[i]);
        }

        /// [type string(8)[Sizes.Names]; 2 elements, fixed 16 bytes, set rejects values longer than 8 bytes; default none]
        /// Element count.
        static constexpr size_t names_size()
        {
            return 2;
        }

        /// Element read in place. index below names_size() (asserted in debug builds).
        std::string get_names(size_t index) const
        {
            return protaco::detail::LoadChars(Fixed() + 98 + 8 * protaco::detail::AssertIndex(index, 2), 8);
        }

        /// All elements, copied.
        std::array<std::string, 2> get_names() const
        {
            std::array<std::string, 2> values;
            for (size_t i = 0; i < 2; ++i)
                values[i] = protaco::detail::LoadChars(Fixed() + 98 + 8 * i, 8);
            return values;
        }

        /// Element write in place. False, the old value kept and the reason in error for an index out of range or a value longer than 8 bytes.
        bool set_names(size_t index, const std::string& value, std::string* error = nullptr)
        {
            if (index >= 2)
                return protaco::detail::IndexError(error, "names", index, 2);
            return protaco::detail::StoreCharsAt(Fixed() + 98 + 8 * index, 8, value, "names", index, error);
        }

        /// All elements. False and nothing written if any value is longer than 8 bytes.
        bool set_names(const std::array<std::string, 2>& values, std::string* error = nullptr)
        {
            for (size_t i = 0; i < 2; ++i)
            {
                if (values[i].size() > 8)
                    return protaco::detail::StoreCharsAt(Fixed() + 98 + 8 * i, 8, values[i], "names", i, error);
            }
            for (size_t i = 0; i < 2; ++i)
                protaco::detail::StoreChars(Fixed() + 98 + 8 * i, 8, values[i], "names", nullptr);
            return true;
        }

        /// [type Level[Level.Count]; 3 elements, fixed 12 bytes]
        /// Element count.
        static constexpr size_t levels_size()
        {
            return 3;
        }

        /// Element read in place. index below levels_size() (asserted in debug builds).
        Level get_levels(size_t index) const
        {
            return static_cast<Level>(protaco::detail::Load<int32_t>(Fixed() + 114 + 4 * protaco::detail::AssertIndex(index, 3)));
        }

        /// All elements, copied.
        std::array<Level, 3> get_levels() const
        {
            std::array<Level, 3> values;
            for (size_t i = 0; i < 3; ++i)
                values[i] = static_cast<Level>(protaco::detail::Load<int32_t>(Fixed() + 114 + 4 * i));
            return values;
        }

        /// Element write in place. False and the reason in error for an index out of range.
        bool set_levels(size_t index, Level value, std::string* error = nullptr)
        {
            if (index >= 3)
                return protaco::detail::IndexError(error, "levels", index, 3);
            protaco::detail::Store<int32_t>(Fixed() + 114 + 4 * index, static_cast<int32_t>(value));
            return true;
        }

        /// All elements.
        void set_levels(const std::array<Level, 3>& values)
        {
            for (size_t i = 0; i < 3; ++i)
                protaco::detail::Store<int32_t>(Fixed() + 114 + 4 * i, static_cast<int32_t>(values[i]));
        }

        /// [type int]
        int32_t get_total() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 126);
        }

        void set_total(int32_t value)
        {
            protaco::detail::Store<int32_t>(Fixed() + 126, value);
        }

    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Panel>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Panel(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Panel::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 41,   // frame header (28 + name 5 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 130,
        kAllowExtension = 0,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x71A7E9E9u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "temps", "counts", "ids", "masks", "ticks", "flags", "names", "levels", "total" };
        return protaco::detail::InList(key, kKeys, 9);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Panel& self, TWriter& writer)
    {
        writer.Field("temps", self.get_temps());
        writer.Field("counts", self.get_counts());
        writer.Field("ids", self.get_ids());
        writer.Field("masks", self.get_masks());
        writer.Field("ticks", self.get_ticks());
        writer.Field("flags", self.get_flags());
        writer.Field("names", self.get_names());
        writer.Field("levels", self.get_levels());
        writer.Field("total", self.get_total());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;   // stays empty: allow_extension false
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace arrays
