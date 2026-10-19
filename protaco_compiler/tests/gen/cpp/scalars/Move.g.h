// Move.g.h. protacoc 0.6.0, source Move.yaml (sha1 a06bba8225f5). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace scalars
{

/// Keys that are Python keywords: attributes from_ and pass_ in Python, the yaml key everywhere else.
/// [version 1; schema 0x97FD0219; source Move.yaml]
class Move : public protaco::CMessage<Move>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Move()
        : Move(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Move(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Move::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Move";
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
        return 0x97FD0219u;
    }

    /// [type string(8); fixed 8 bytes, set rejects longer values]
    std::string get_from() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 8);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 8 bytes.
    bool set_from(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 8, value, "from", error);
    }

    /// [type string(8); fixed 8 bytes, set rejects longer values]
    std::string get_to() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 8, 8);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 8 bytes.
    bool set_to(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 8, 8, value, "to", error);
    }

    /// [type bool]
    bool get_pass() const
    {
        return protaco::detail::LoadBool(buf_.data() + kFixedOffset + 16);
    }

    void set_pass(bool value)
    {
        protaco::detail::StoreBool(buf_.data() + kFixedOffset + 16, value);
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

    /// Fixed fields read in place from received bytes (protaco::TryView<Move>): the bytes are not copied.
    /// Valid after a successful TryView while the bytes live and stay unchanged.
    class View : public protaco::CView
    {
    public:
        /// [type string(8); fixed 8 bytes, set rejects longer values]
        std::string get_from() const
        {
            return protaco::detail::LoadChars(Fixed() + 0, 8);
        }

        /// [type string(8); fixed 8 bytes, set rejects longer values]
        std::string get_to() const
        {
            return protaco::detail::LoadChars(Fixed() + 8, 8);
        }

        /// [type bool]
        bool get_pass() const
        {
            return protaco::detail::LoadBool(Fixed() + 16);
        }
    };

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Move>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Move(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Move::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 40,   // frame header (28 + name 4 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 17,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x97FD0219u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "from", "to", "pass" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf&, TVisitor&)
    {
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Move& self, TWriter& writer)
    {
        writer.Field("from", self.get_from());
        writer.Field("to", self.get_to());
        writer.Field("pass", self.get_pass());
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace scalars
