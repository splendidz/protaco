// Menu.g.h. protacoc 0.7.1, source Menu.yaml (sha1 95302b23b98e). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "Grade.enum.h"
#include "Dish.g.h"

namespace maps
{

/// Every map key kind and value kind. Map types contain ',', so they are quoted inside {...}.
/// [version 1; schema 0x0EC85362; source Menu.yaml]
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
        return 0x0EC85362u;
    }

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

    /// [type map<int, uint>; required, decoding fails if missing]
    const std::unordered_map<int32_t, uint32_t>& get_stock() const
    {
        return stock_;
    }

    std::unordered_map<int32_t, uint32_t>& mutable_stock()
    {
        cache_.exposed = true;
        return stock_;
    }

    void set_stock(const std::unordered_map<int32_t, uint32_t>& value)
    {
        stock_ = value;
        cache_.valid = false;
    }

    /// [type map<Grade, Dish>]
    const std::unordered_map<Grade, Dish>& get_specials() const
    {
        return specials_;
    }

    std::unordered_map<Grade, Dish>& mutable_specials()
    {
        cache_.exposed = true;
        return specials_;
    }

    void set_specials(const std::unordered_map<Grade, Dish>& value)
    {
        specials_ = value;
        cache_.valid = false;
    }

    /// [type map<uint64, string>]
    const std::unordered_map<uint64_t, std::string>& get_notes() const
    {
        return notes_;
    }

    std::unordered_map<uint64_t, std::string>& mutable_notes()
    {
        cache_.exposed = true;
        return notes_;
    }

    void set_notes(const std::unordered_map<uint64_t, std::string>& value)
    {
        notes_ = value;
        cache_.valid = false;
    }

    /// [type map<string, bool>]
    const std::unordered_map<std::string, bool>& get_vegan() const
    {
        return vegan_;
    }

    std::unordered_map<std::string, bool>& mutable_vegan()
    {
        cache_.exposed = true;
        return vegan_;
    }

    void set_vegan(const std::unordered_map<std::string, bool>& value)
    {
        vegan_ = value;
        cache_.valid = false;
    }

    /// [type map<string, bytes>]
    const std::unordered_map<std::string, std::vector<uint8_t>>& get_photos() const
    {
        return photos_;
    }

    std::unordered_map<std::string, std::vector<uint8_t>>& mutable_photos()
    {
        cache_.exposed = true;
        return photos_;
    }

    void set_photos(const std::unordered_map<std::string, std::vector<uint8_t>>& value)
    {
        photos_ = value;
        cache_.valid = false;
    }

    /// [type map<string, Grade>]
    const std::unordered_map<std::string, Grade>& get_grades() const
    {
        return grades_;
    }

    std::unordered_map<std::string, Grade>& mutable_grades()
    {
        cache_.exposed = true;
        return grades_;
    }

    void set_grades(const std::unordered_map<std::string, Grade>& value)
    {
        grades_ = value;
        cache_.valid = false;
    }

    /// [type map<string, list<string>>]
    const std::unordered_map<std::string, std::vector<std::string>>& get_tags() const
    {
        return tags_;
    }

    std::unordered_map<std::string, std::vector<std::string>>& mutable_tags()
    {
        cache_.exposed = true;
        return tags_;
    }

    void set_tags(const std::unordered_map<std::string, std::vector<std::string>>& value)
    {
        tags_ = value;
        cache_.valid = false;
    }

    /// [type map<int64, map<string, int>>]
    const std::unordered_map<int64_t, std::unordered_map<std::string, int32_t>>& get_daily() const
    {
        return daily_;
    }

    std::unordered_map<int64_t, std::unordered_map<std::string, int32_t>>& mutable_daily()
    {
        cache_.exposed = true;
        return daily_;
    }

    void set_daily(const std::unordered_map<int64_t, std::unordered_map<std::string, int32_t>>& value)
    {
        daily_ = value;
        cache_.valid = false;
    }

    /// [type string]
    const std::string& get_title() const
    {
        return title_;
    }

    void set_title(const std::string& value)
    {
        title_ = value;
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
    const std::vector<uint8_t>& Serialize() const override
    {
        return protaco::detail::Access::Serialize(*this);
    }

    /// Text for logs: field names and values, ext entries under ext{...}. protaco::TextStyle::Pretty() for one field per line.
    std::string ToString(const protaco::TextStyle& style = protaco::TextStyle()) const override
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
        kFixedSize = 0,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x0EC85362u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "prices", "stock", "specials", "notes", "vegan", "photos", "grades", "tags", "daily", "title" };
        return protaco::detail::InList(key, kKeys, 10);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("prices", self.prices_, false);
        visitor.Field("stock", self.stock_, true);
        visitor.Field("specials", self.specials_, false);
        visitor.Field("notes", self.notes_, false);
        visitor.Field("vegan", self.vegan_, false);
        visitor.Field("photos", self.photos_, false);
        visitor.Field("grades", self.grades_, false);
        visitor.Field("tags", self.tags_, false);
        visitor.Field("daily", self.daily_, false);
        visitor.Field("title", self.title_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const Menu& self, TWriter& writer)
    {
        writer.Field("prices", self.prices_);
        writer.Field("stock", self.stock_);
        writer.Field("specials", self.specials_);
        writer.Field("notes", self.notes_);
        writer.Field("vegan", self.vegan_);
        writer.Field("photos", self.photos_);
        writer.Field("grades", self.grades_);
        writer.Field("tags", self.tags_);
        writer.Field("daily", self.daily_);
        writer.Field("title", self.title_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::unordered_map<std::string, double> prices_;
    std::unordered_map<int32_t, uint32_t> stock_;
    std::unordered_map<Grade, Dish> specials_;
    std::unordered_map<uint64_t, std::string> notes_;
    std::unordered_map<std::string, bool> vegan_;
    std::unordered_map<std::string, std::vector<uint8_t>> photos_;
    std::unordered_map<std::string, Grade> grades_;
    std::unordered_map<std::string, std::vector<std::string>> tags_;
    std::unordered_map<int64_t, std::unordered_map<std::string, int32_t>> daily_;
    std::string title_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace maps
