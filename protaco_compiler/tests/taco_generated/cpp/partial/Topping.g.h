// Topping.g.h. protacoc 0.7.1, source Topping.yaml (sha1 73a1aea489ba). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace partial
{

/// One topping. Element of PizzaOrder.toppings.
/// [version 1; schema 0xEB8A11D8; source Topping.yaml]
class Topping : public protaco::CMessage<Topping>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Topping()
        : Topping(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Topping(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Topping::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 24, 30);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Topping";
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
        return 0xEB8A11D8u;
    }

    /// topping name
    /// [type string(24); fixed 24 bytes, set rejects longer values]
    std::string get_name() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 24);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 24 bytes.
    bool set_name(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 24, value, "name", error);
    }

    /// weight in grams
    /// [type int; default 30]
    int32_t get_grams() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 24);
    }

    void set_grams(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 24, value);
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
    const std::vector<uint8_t>& Serialize() const override
    {
        return protaco::detail::Access::Serialize(*this);
    }

    /// Text for logs: field names and values, ext entries under ext{...}. protaco::TextStyle::Pretty() for one field per line.
    std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const override
    {
        return protaco::detail::Access::ToText(*this, style);
    }

    /// Fixed fields read in place from received bytes (protaco::TryView<Topping>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// topping name
        /// [type string(24); fixed 24 bytes, set rejects longer values]
        std::string get_name() const
        {
            return protaco::detail::LoadChars(Fixed() + 0, 24);
        }

        /// weight in grams
        /// [type int; default 30]
        int32_t get_grams() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 24);
        }
    };

    /// Fixed fields read and written in place in a frame in memory the caller owns (protaco::TryMutableView<Topping>),
    /// for example shared memory. Nothing is copied. Valid after a successful TryMutableView while the memory lives.
    /// Writes are plain stores: concurrent readers and writers need the caller's synchronization.
    class MutableView : public protaco::CMutableView
    {
    public:
        /// Bytes of a new Topping frame (header and fixed block): the memory to fill with Serialize() and give TryMutableView.
        static constexpr size_t FrameSize()
        {
            return 71;
        }

        /// topping name
        /// [type string(24); fixed 24 bytes, set rejects longer values]
        std::string get_name() const
        {
            return protaco::detail::LoadChars(Fixed() + 0, 24);
        }

        /// Returns false, keeps the old value and writes the reason to error if longer than 24 bytes.
        bool set_name(const std::string& value, std::string* error = nullptr)
        {
            return protaco::detail::StoreChars(Fixed() + 0, 24, value, "name", error);
        }

        /// weight in grams
        /// [type int; default 30]
        int32_t get_grams() const
        {
            return protaco::detail::Load<int32_t>(Fixed() + 24);
        }

        void set_grams(int32_t value)
        {
            protaco::detail::Store<int32_t>(Fixed() + 24, value);
        }

    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Topping>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Topping(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Topping::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 43,   // frame header (28 + name 7 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 28,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xEB8A11D8u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "name", "grams" };
        return protaco::detail::InList(key, kKeys, 2);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Topping& self, TWriter& writer)
    {
        writer.Field("name", self.get_name());
        writer.Field("grams", self.get_grams());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace partial
