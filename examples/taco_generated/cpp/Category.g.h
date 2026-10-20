// Category.g.h. protacoc 0.6.0, source Category.yaml (sha1 9b036f2023ea). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// A menu section. Sections nest: a section holds its subsections, so the message refers to itself.
/// [version 1; schema 0x6B7C6E97; source Category.yaml]
class Category : public protaco::CMessage<Category>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Category()
        : Category(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Category(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Category::IsSchemaKey);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Category";
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
        return 0x6B7C6E97u;
    }

    /// section name
    /// [type string]
    const std::string& get_name() const
    {
        return name_;
    }

    void set_name(const std::string& value)
    {
        name_ = value;
        cache_.valid = false;
    }

    /// dish names
    /// [type list<string>]
    const std::vector<std::string>& get_dishes() const
    {
        return dishes_;
    }

    std::vector<std::string>& mutable_dishes()
    {
        cache_.exposed = true;
        return dishes_;
    }

    void set_dishes(const std::vector<std::string>& value)
    {
        dishes_ = value;
        cache_.valid = false;
    }

    /// sections inside this one
    /// [type list<Category>]
    const std::vector<Category>& get_subsections() const
    {
        return subsections_;
    }

    std::vector<Category>& mutable_subsections()
    {
        cache_.exposed = true;
        return subsections_;
    }

    void set_subsections(const std::vector<Category>& value)
    {
        subsections_ = value;
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
    friend class protaco::CMessage<Category>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Category(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Category::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 44,   // frame header (28 + name 8 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 0,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x6B7C6E97u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "name", "dishes", "subsections" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("name", self.name_, false);
        visitor.Field("dishes", self.dishes_, false);
        visitor.Field("subsections", self.subsections_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Category& self, TWriter& writer)
    {
        writer.Field("name", self.name_);
        writer.Field("dishes", self.dishes_);
        writer.Field("subsections", self.subsections_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string name_;
    std::vector<std::string> dishes_;
    std::vector<Category> subsections_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace demo
