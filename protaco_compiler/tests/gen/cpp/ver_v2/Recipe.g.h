// Recipe.g.h. protacoc 0.5.0, source Recipe.yaml (sha1 f97dfa2fc24c). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace v2
{

/// Version test. Version 2 of ver_v1/Recipe.yaml with two more fields.
/// [version 2; schema 0xAC2B89A3; source Recipe.yaml]
class Recipe
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    Recipe()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Recipe::IsSchemaKey);
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 16, 25.5);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 24, 3);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Recipe";
    }

    /// Version of this type. ToObject reads frames of this version or newer as this version.
    static uint16_t MsgVersion()
    {
        return 2;
    }

    /// Oldest supported version (min_version). Frames carry one schema fingerprint per version from here up.
    static uint16_t MsgMinVersion()
    {
        return 2;
    }

    /// Schema fingerprint of this version. A frame whose fingerprint for this version differs is rejected by ToObject.
    static uint32_t MsgSchemaHash()
    {
        return 0xAC2B89A3u;
    }

    /// recipe name
    /// [type string(16); fixed 16 bytes, set rejects longer values]
    std::string get_name() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 16);
    }

    /// Returns false and keeps the old value if longer than 16 bytes.
    bool set_name(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 16, value);
    }

    /// temperature
    /// [type double; default 25.5]
    double get_temp() const
    {
        return protaco::detail::Load<double>(buf_.data() + kFixedOffset + 16);
    }

    void set_temp(double value)
    {
        protaco::detail::Store<double>(buf_.data() + kFixedOffset + 16, value);
    }

    /// [type int; default 3]
    int32_t get_step() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 24);
    }

    void set_step(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 24, value);
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

    /// [type string; default "none"]
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
    /// Unchanged variable fields and ext reuse the last bytes. After a get_mutable_ call the variable block is rebuilt every time.
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
        kFixedSize = 28,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0xAC2B89A3u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "name", "temp", "step", "path", "note" };
        return protaco::detail::InList(key, kKeys, 5);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("path", self.path_, true);
        visitor.Field("note", self.note_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Recipe& self, TWriter& writer)
    {
        writer.Field("name", self.get_name());
        writer.Field("temp", self.get_temp());
        writer.Field("step", self.get_step());
        writer.Field("path", self.path_);
        writer.Field("note", self.note_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string path_;
    std::string note_ = "none";
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace v2
