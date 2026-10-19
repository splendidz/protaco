// Topping.g.h. protacoc 0.5.0, source Topping.yaml (sha1 f343ff787f9f). Do not edit.
#pragma once

#include "Protaco.hpp"

namespace demo
{

/// One topping. Element of PizzaOrder.toppings.
/// [version 1; schema 0xEB8A11D8; source Topping.yaml]
class Topping
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    Topping()
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

    /// Returns false and keeps the old value if longer than 24 bytes.
    bool set_name(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 24, value);
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
    /// Fixed fields only. With an empty ext, serializing costs nothing.
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

}  // namespace demo
