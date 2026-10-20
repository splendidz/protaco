// Trail.g.h. protacoc 0.6.0, source Trail.yaml (sha1 4eba086e08ee). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "Point.g.h"

namespace arrays
{

/// A fixed message in a list stays a nested body; inline next to variable fields.
/// [version 1; schema 0x13BA591D; source Trail.yaml]
class Trail : public protaco::CMessage<Trail>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Trail()
        : Trail(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Trail(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Trail::IsSchemaKey);
        protaco::detail::Access::InitBlock<Point>(buf_.data() + kFixedOffset + 0);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Trail";
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
        return 0x13BA591Du;
    }

    /// [type Point; fixed message, inline 8 bytes]
    /// Read in place: a view of the Point bytes inside this message.
    Point::ConstRef get_start() const
    {
        return Point::ConstRef(buf_.data() + kFixedOffset + 0);
    }

    /// Written in place: the Point bytes inside this message.
    Point::Ref mutable_start()
    {
        return Point::Ref(buf_.data() + kFixedOffset + 0);
    }

    /// Copies the fields of value in.
    void set_start(const Point& value)
    {
        protaco::detail::Access::CopyBlock(buf_.data() + kFixedOffset + 0, value);
    }

    /// [type list<Point>]
    const std::vector<Point>& get_points() const
    {
        return points_;
    }

    std::vector<Point>& mutable_points()
    {
        cache_.exposed = true;
        return points_;
    }

    void set_points(const std::vector<Point>& value)
    {
        points_ = value;
        cache_.valid = false;
    }

    /// [type string]
    const std::string& get_note() const
    {
        return note_;
    }

    void set_note(const std::string& value)
    {
        note_ = value;
        cache_.valid = false;
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

private:
    friend struct protaco::detail::Access;
    friend class protaco::CMessage<Trail>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Trail(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Trail::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 41,   // frame header (28 + name 5 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 8,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x13BA591Du };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "start", "points", "note" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("points", self.points_, false);
        visitor.Field("note", self.note_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Trail& self, TWriter& writer)
    {
        writer.Field("start", self.get_start());
        writer.Field("points", self.points_);
        writer.Field("note", self.note_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::vector<Point> points_;
    std::string note_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace arrays
