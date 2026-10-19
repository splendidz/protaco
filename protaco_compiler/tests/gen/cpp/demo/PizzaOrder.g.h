// PizzaOrder.g.h. protacoc 0.5.0, source PizzaOrder.yaml (sha1 a1af26cb1afe). Do not edit.
#pragma once

#include "Protaco.hpp"
#include "Delivery.g.h"
#include "Topping.g.h"

namespace demo
{

/// Pizza order. One message for every store; fields used by one store only say so in their comment.
/// pizza order
/// [version 1; schema 0x9D0CF242; source PizzaOrder.yaml]
class PizzaOrder
{
public:
    /// Initialized to defaults. Fixed fields live in the frame buffer.
    PizzaOrder()
    {
        protaco::detail::InitFrame(buf_, MsgName(), MsgVersion(), MsgMinVersion(), SchemaHashes(), kFixedSize);
        protaco::detail::Access::BindExt(ext_, &PizzaOrder::IsSchemaKey);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, 12);
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 68, 8);
        protaco::detail::StoreBool(buf_.data() + kFixedOffset + 72, true);
    }

    /// Message name. Stored in the frame header and checked by ToObject.
    static const char* MsgName()
    {
        return "PizzaOrder";
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
        return 0x9D0CF242u;
    }

    /// Order number. Fixed length to fit the receipt column.
    /// order number
    /// [type string(32); fixed 32 bytes, set rejects longer values]
    std::string get_orderId() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 0, 32);
    }

    /// Returns false and keeps the old value if longer than 32 bytes.
    bool set_orderId(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 0, 32, value);
    }

    /// store name
    /// [type string(32); fixed 32 bytes, set rejects longer values]
    std::string get_store() const
    {
        return protaco::detail::LoadChars(buf_.data() + kFixedOffset + 32, 32);
    }

    /// Returns false and keeps the old value if longer than 32 bytes.
    bool set_store(const std::string& value)
    {
        return protaco::detail::StoreChars(buf_.data() + kFixedOffset + 32, 32, value);
    }

    /// customer name
    /// [type string; default "anonymous"]
    const std::string& get_customer() const
    {
        return customer_;
    }

    void set_customer(const std::string& value)
    {
        customer_ = value;
        cache_.valid = false;
    }

    /// diameter in inches
    /// [type int; default 12]
    int32_t get_size() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 64);
    }

    void set_size(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 64, value);
    }

    /// number of slices
    /// [type int; default 8]
    int32_t get_slices() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 68);
    }

    void set_slices(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 68, value);
    }

    /// Wood-fired oven. NAPOLI store only.
    /// baked in a wood-fired oven
    /// [type bool; default true]
    bool get_woodFired() const
    {
        return protaco::detail::LoadBool(buf_.data() + kFixedOffset + 72);
    }

    void set_woodFired(bool value)
    {
        protaco::detail::StoreBool(buf_.data() + kFixedOffset + 72, value);
    }

    /// Deep dish depth in cm. CHICAGO store only. 0 means regular crust.
    /// deep dish depth
    /// [type int; default 0]
    int32_t get_deepDishCm() const
    {
        return protaco::detail::Load<int32_t>(buf_.data() + kFixedOffset + 73);
    }

    void set_deepDishCm(int32_t value)
    {
        protaco::detail::Store<int32_t>(buf_.data() + kFixedOffset + 73, value);
    }

    /// toppings
    /// [type list<Topping>; required, decoding fails if missing]
    const std::vector<Topping>& get_toppings() const
    {
        return toppings_;
    }

    std::vector<Topping>& get_mutable_toppings()
    {
        cache_.exposed = true;
        return toppings_;
    }

    void set_toppings(const std::vector<Topping>& value)
    {
        toppings_ = value;
        cache_.valid = false;
    }

    /// special requests (e.g. no onions)
    /// [type list<string>]
    const std::vector<std::string>& get_notes() const
    {
        return notes_;
    }

    std::vector<std::string>& get_mutable_notes()
    {
        cache_.exposed = true;
        return notes_;
    }

    void set_notes(const std::vector<std::string>& value)
    {
        notes_ = value;
        cache_.valid = false;
    }

    /// delivery details
    /// [type Delivery]
    const Delivery& get_delivery() const
    {
        return delivery_;
    }

    Delivery& get_mutable_delivery()
    {
        cache_.exposed = true;
        return delivery_;
    }

    void set_delivery(const Delivery& value)
    {
        delivery_ = value;
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
        kFixedOffset = 46,   // frame header (28 + name 10 + fingerprints 4 x 1) + FixedLen (4)
        kFixedSize = 77,
        kAllowExtension = 1,
    };

    // Schema fingerprints of versions MsgMinVersion()..MsgVersion(), carried in the frame header.
    static const uint32_t* SchemaHashes()
    {
        static const uint32_t kHashes[] = { 0x9D0CF242u };
        return kHashes;
    }

    // Keys not allowed in ext: schema keys of every version
    static bool IsSchemaKey(const char* key)
    {
        static const char* const kKeys[] = { "orderId", "store", "customer", "size", "slices", "woodFired", "deepDishCm", "toppings", "notes", "delivery" };
        return protaco::detail::InList(key, kKeys, 10);
    }

    // The single list of variable fields. Shared by size, write and read.
    template <class TSelf, class TVisitor>
    static void VisitVar(TSelf& self, TVisitor& visitor)
    {
        visitor.Field("customer", self.customer_, false);
        visitor.Field("toppings", self.toppings_, true);
        visitor.Field("notes", self.notes_, false);
        visitor.Field("delivery", self.delivery_, false);
    }

    // All fields in yaml order, for ToString.
    template <class TWriter>
    static void VisitText(const PizzaOrder& self, TWriter& writer)
    {
        writer.Field("orderId", self.get_orderId());
        writer.Field("store", self.get_store());
        writer.Field("customer", self.customer_);
        writer.Field("size", self.get_size());
        writer.Field("slices", self.get_slices());
        writer.Field("woodFired", self.get_woodFired());
        writer.Field("deepDishCm", self.get_deepDishCm());
        writer.Field("toppings", self.toppings_);
        writer.Field("notes", self.notes_);
        writer.Field("delivery", self.delivery_);
    }

    mutable std::vector<uint8_t> buf_;   // frame: header + FixedLen + fixed block (+ variable block after Serialize)
    std::string customer_ = "anonymous";
    std::vector<Topping> toppings_;
    std::vector<std::string> notes_;
    Delivery delivery_;
    protaco::CExt ext_;
    mutable protaco::detail::VarCache cache_;   // Serialize() reuse of the variable block
};

}  // namespace demo
