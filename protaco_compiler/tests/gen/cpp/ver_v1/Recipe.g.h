// Recipe.g.h. protacoc 0.6.0, source Recipe.yaml (sha1 a8ae1c9a0807). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace v1
{

/// Version test. Pairs with ver_v2 (other version) and alt (same version, other fields).
/// [version 1; schema 0x92D7F812; source Recipe.yaml]
class Recipe
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    Recipe()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Recipe::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 16, 3);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Recipe";
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
        return 0x92D7F812u;
    }

    /// recipe name
    /// [type string(16); fixed 16 bytes, set rejects longer values]
    std::string get_name() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 16);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 16 bytes.
    bool set_name(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 16, value, "name", error);
    }

    /// [type int; default 3]
    int32_t get_step() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 16);
    }

    void set_step(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 16, value);
    }

    /// [type string; required, decoding fails if missing]
    const std::string& get_path() const
    {
        return path_;
    }

    void set_path(const std::string& value)
    {
        path_ = value;
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

    /// Writes the envelope (id, corr, ts) straight into the frame header.
    void SetEnvelope(const protaco::Envelope& env)
    {
        protaco::detail::StoreEnvelope(buf_, env);
    }

    protaco::Envelope GetEnvelope() const
    {
        return protaco::detail::LoadEnvelope(buf_.data());
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

    enum : uint32_t
    {
        kFixedOffset = 42,   // frame header (28 + name 6 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 20,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x92D7F812u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "name", "step", "path" };
        return protaco::detail::InList(key, kKeys, 3);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("path", self.path_, true);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Recipe& self, TWriter& writer)
    {
        writer.Field("name", self.get_name());
        writer.Field("step", self.get_step());
        writer.Field("path", self.path_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string path_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace v1
