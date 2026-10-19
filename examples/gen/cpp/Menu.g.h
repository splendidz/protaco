// Menu.g.h. protacoc 0.6.0, source Menu.yaml (sha1 886d288a340b). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "PizzaSize.enum.h"
#include "Category.g.h"

namespace demo
{

/// A store menu: an enum field, a map and a tree of sections.
/// [version 1; schema 0x1A3924BC; source Menu.yaml]
class Menu : public protaco::CMessage<Menu>
{
public:
    /// Initialized to defaults. The envelope gets a new id (protaco::NextId) and the current time (protaco::NowUs).
    Menu()
        : Menu(protaco::detail::NoStampTag())
    {
        protaco::detail::StampEnvelope(buf_);
    }

    /// Runtime use: no envelope id or time (decoded messages, nested fields). Fixed fields live in the frame buffer.
    explicit Menu(protaco::detail::NoStampTag)
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &Menu::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 16, static_cast<int32_t>(PizzaSize::Large));
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "Menu";
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
        return 0x1A3924BCu;
    }

    /// store code
    /// [type string(16); fixed 16 bytes, set rejects longer values]
    std::string get_store() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 16);
    }

    /// Returns false, keeps the old value and writes the reason to error if longer than 16 bytes.
    bool set_store(const std::string& value, std::string* error = nullptr)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 16, value, "store", error);
    }

    /// size when an order does not say
    /// [type PizzaSize; default Large]
    /// Numbers this build does not know are kept as they are (ToString shows ?(n)).
    PizzaSize get_defaultSize() const
    {
        return static_cast<PizzaSize>(protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 16));
    }

    void set_defaultSize(PizzaSize value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 16, static_cast<int32_t>(value));
    }

    /// Map types contain ',', so they are quoted inside {...}.
    /// price by dish name
    /// [type map<string, double>]
    const std::unordered_map<std::string, double>& get_prices() const
    {
        return prices_;
    }

    std::unordered_map<std::string, double>& mutable_prices()
    {
        cache_.exposed = true;
        return prices_;
    }

    void set_prices(const std::unordered_map<std::string, double>& value)
    {
        prices_ = value;
        cache_.valid = false;
    }

    /// top-level sections
    /// [type list<Category>]
    const std::vector<Category>& get_sections() const
    {
        return sections_;
    }

    std::vector<Category>& mutable_sections()
    {
        cache_.exposed = true;
        return sections_;
    }

    void set_sections(const std::vector<Category>& value)
    {
        sections_ = value;
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
    friend class protaco::CMessage<Menu>;

    // Runtime use: a received frame of exactly this version becomes the buffer (protaco::ToObject with std::vector&&).
    Menu(protaco::detail::AdoptTag, std::vector<uint8_t>&& frame)
        : buf_(std::move(frame))
    {
        protaco::detail::Access::BindExt(ext_, &Menu::IsSchemaKey);
    }

    enum : uint32_t
    {
        kFixedOffset = 40,   // frame header (28 + name 4 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 20,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x1A3924BCu };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "store", "defaultSize", "prices", "sections" };
        return protaco::detail::InList(key, kKeys, 4);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("prices", self.prices_, false);
        visitor.Field("sections", self.sections_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Menu& self, TWriter& writer)
    {
        writer.Field("store", self.get_store());
        writer.Field("defaultSize", self.get_defaultSize());
        writer.Field("prices", self.prices_);
        writer.Field("sections", self.sections_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::unordered_map<std::string, double> prices_;
    std::vector<Category> sections_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace demo
