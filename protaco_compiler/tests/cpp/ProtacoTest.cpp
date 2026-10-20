// ProtacoTest.cpp. Unit tests (gtest) for the Protaco.hpp runtime and protacoc generated code.
// Covers: round trip, in-place fixed fields, Serialize() reuse, defaults, version and schema checks, supported versions,
// closed messages, nesting depth, error paths, ext, ToString, corrupt input, golden bytes, cost of the contract checks.
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "DeliveryProof.g.h"
#include "Heartbeat.g.h"
#include "PizzaOrder.g.h"
#include "DeliveryRoute.g.h"
#include "alt/Recipe.g.h"
#include "build1/Order.g.h"
#include "build1/Ping.g.h"
#include "build2/Order.g.h"
#include "build2/Ping.g.h"
#include "build3/Order.g.h"
#include "build4/Order.g.h"
#include "deep/D8.g.h"
#include "partial/PizzaOrder.g.h"
#include "partial/DeliveryRoute.g.h"
#include "enums/Pie.g.h"
#include "maps/Menu.g.h"
#include "recursive/Node.g.h"
#include "scalars/Numbers.g.h"
#include "scalars/Tick.g.h"
#include "ver_v1/Recipe.g.h"
#include "ver_v2/Recipe.g.h"

using namespace demo;

namespace
{

RouteStop MakeStop(const std::string& orderId, const std::string& address, int32_t etaMin)
{
    RouteStop stop;
    stop.set_orderId(orderId);
    stop.set_address(address);
    stop.set_etaMin(etaMin);
    return stop;
}

// Golden values. Same as the C# test (tests/cs/Program.cs MakeDeliveryRoute).
DeliveryRoute MakeDeliveryRoute()
{
    DeliveryRoute route;
    route.set_rider("rider-7");
    route.mutable_stops().push_back(MakeStop("A-100", "1 Pizza St", 10));
    route.mutable_stops().push_back(MakeStop("A-101", "2 Pasta Ave", 25));
    route.set_note("closest-first");
    route.GetMutableExt().SetValue("priority", 5);
    protaco::Envelope env;
    env.id = 7;
    env.corr = 0;
    env.ts = 1000;
    route.SetEnvelope(env);
    return route;
}

// Golden values. Same as the C# test MakePizzaOrder.
PizzaOrder MakePizzaOrder()
{
    PizzaOrder order;
    order.set_orderId("A-100");
    order.set_store("NAPOLI-1");
    order.set_customer("mario");
    order.set_size(14);
    order.set_woodFired(true);
    Topping basil;
    basil.set_name("basil");
    basil.set_grams(5);
    order.mutable_toppings().push_back(basil);
    Topping cheese;
    cheese.set_name("mozzarella");
    order.mutable_toppings().push_back(cheese);
    order.mutable_notes().push_back("no onion");
    order.mutable_delivery().set_address("1 Pizza St");
    order.mutable_delivery().set_floor(3);
    order.mutable_delivery().mutable_doorCode() = { 4, 2, 4, 2 };
    order.mutable_delivery().mutable_contacts().push_back("010-0000-0000");
    order.SetEnvelope(protaco::Envelope());   // golden bytes: no id, no time
    return order;
}

Heartbeat MakeHeartbeat()
{
    Heartbeat beat;
    beat.set_node("oven-1");
    beat.set_seq(123456789);
    beat.set_cpuLoad(0.25);
    beat.set_uptimeSec(3600);
    beat.SetEnvelope(protaco::Envelope());   // golden bytes: no id, no time
    return beat;
}

// Golden values. Same as the C# test MakeNumbers.
scalars::Numbers MakeNumbers()
{
    scalars::Numbers numbers;
    numbers.set_u32(0);
    numbers.mutable_us() = { 0u, 1u, 4294967295u };
    numbers.mutable_u64s().push_back(18446744073709551615ull);
    numbers.set_name(u8"한글 ✓");
    numbers.set_text(u8"café 寿司");
    numbers.SetEnvelope(protaco::Envelope());   // golden bytes: no id, no time
    return numbers;
}

// Golden values. Same as the C# test MakePie. 5 is a removed value, 9 was never declared.
enums::Pie MakePie()
{
    enums::Pie pie;
    pie.set_size(enums::PizzaSize::Family);
    pie.set_crust(static_cast<enums::PizzaSize>(9));
    pie.mutable_sizes() = { enums::PizzaSize::Small, enums::PizzaSize::Family, static_cast<enums::PizzaSize>(5), static_cast<enums::PizzaSize>(9) };
    pie.SetEnvelope(protaco::Envelope());   // golden bytes: no id, no time
    return pie;
}

// Golden values. Same as the C# test MakeMenu. One entry per map: map bytes follow the container's iteration order.
maps::Menu MakeMenu()
{
    maps::Menu menu;
    menu.mutable_prices()["dupA"] = 9.5;
    menu.mutable_stock()[-1] = 4294967295u;
    maps::Dish dish;
    dish.set_name("ragu");
    dish.set_price(12.25);
    menu.mutable_specials().emplace(maps::Grade::Silver, dish);
    menu.mutable_notes()[18446744073709551615ull] = u8"매운맛";
    menu.mutable_vegan()["ragu"] = false;
    menu.mutable_photos()["ragu"] = { 0xFF, 0x00 };
    menu.mutable_grades()["ragu"] = static_cast<maps::Grade>(9);
    menu.mutable_tags()["ragu"] = { "hot", "" };
    menu.mutable_daily()[20261020] = { { "ragu", 3 } };
    menu.set_title("lunch");
    menu.SetEnvelope(protaco::Envelope());   // golden bytes: no id, no time
    return menu;
}

// Golden values. Same as the C# test MakeTree. root{children=[a{next=b}, c]}
recursive::Node MakeTree()
{
    recursive::Node root;
    root.set_name("root");
    root.set_value(1);
    recursive::Node a;
    a.set_name("a");
    a.set_value(2);
    a.mutable_next().set_name("b");
    a.mutable_next().set_value(3);
    root.mutable_children().push_back(a);
    recursive::Node c;
    c.set_name("c");
    c.set_value(4);
    root.mutable_children().push_back(c);
    root.SetEnvelope(protaco::Envelope());   // golden bytes: no id, no time
    return root;
}

// Golden values. Same as the C# test MakeDeliveryProof. Zeros, 0xFF and an empty element included.
DeliveryProof MakeDeliveryProof()
{
    DeliveryProof proof;
    proof.set_orderId("A-1001");
    proof.set_photo({0xFF, 0xD8, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00, 0xFF, 0xD9});
    proof.mutable_signatures().push_back({0x00});
    proof.mutable_signatures().push_back({});
    proof.mutable_signatures().push_back({0x01, 0x02});
    proof.SetEnvelope(protaco::Envelope());   // golden bytes: no id, no time
    return proof;
}

// Frame header offsets (README.md "Serialization format").
size_t VersionAt(const std::vector<uint8_t>& frame)
{
    return 4 + frame[3];
}

size_t BodySizeAt(const std::vector<uint8_t>& frame)
{
    return 24 + frame[3];
}

// Rewrites the fingerprint of the frame's own version so a frame from another yaml reaches the body decoder.
std::vector<uint8_t> WithSchemaHash(std::vector<uint8_t> frame, uint32_t hash)
{
    uint16_t version = 0;
    uint16_t minVersion = 0;
    std::memcpy(&version, frame.data() + VersionAt(frame), sizeof(version));
    std::memcpy(&minVersion, frame.data() + VersionAt(frame) + 2, sizeof(minVersion));
    std::memcpy(frame.data() + 28 + frame[3] + 4 * (version - minVersion), &hash, sizeof(hash));
    return frame;
}

// Appends one TLV to the body and fixes BodyLen.
std::vector<uint8_t> WithExtraTlv(std::vector<uint8_t> frame, const std::string& key, int32_t value)
{
    frame.push_back(static_cast<uint8_t>(key.size()));
    frame.insert(frame.end(), key.begin(), key.end());
    frame.push_back(static_cast<uint8_t>(protaco::TacoType::Int));
    const uint32_t length = sizeof(value);
    const uint8_t* lengthBytes = reinterpret_cast<const uint8_t*>(&length);
    frame.insert(frame.end(), lengthBytes, lengthBytes + sizeof(length));
    const uint8_t* valueBytes = reinterpret_cast<const uint8_t*>(&value);
    frame.insert(frame.end(), valueBytes, valueBytes + sizeof(value));
    uint8_t* bodySizeAt = frame.data() + BodySizeAt(frame);
    uint32_t bodySize = 0;
    std::memcpy(&bodySize, bodySizeAt, sizeof(bodySize));
    bodySize += static_cast<uint32_t>(1 + key.size() + 1 + 4 + sizeof(value));
    std::memcpy(bodySizeAt, &bodySize, sizeof(bodySize));
    return frame;
}

template <class T>
bool Decode(const std::vector<uint8_t>& bytes, T* out, std::string* error)
{
    return protaco::TryToObject(bytes.data(), bytes.size(), out, error);
}

volatile uint32_t g_sink = 0;

// ns per call, best of 5 rounds.
template <class F>
double BestNs(int count, F call)
{
    double best = 1e30;
    for (int round = 0; round < 5; ++round)
    {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < count; ++i)
            call();
        const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / count;
        if (ns < best)
            best = ns;
    }
    return best;
}

// Version and schema checks timed alone against a whole decode. The check alone is an upper bound:
// it repeats the header reads that PeekHeader does anyway. The bound holds in Release; Debug timings only print.
template <class T>
void ExpectCheckCostNegligible(const T& message, int count)
{
    const std::vector<uint8_t> bytes = message.Serialize();
    const uint8_t* const volatile frame = bytes.data();   // reloaded every call, so the reads are not hoisted
    const double checkNs = BestNs(count, [&]()
    {
        const uint8_t* p = frame;
        const size_t nameLength = p[3];
        uint16_t version = 0;
        uint16_t minVersion = 0;
        uint32_t hash = 0;
        std::memcpy(&version, p + 4 + nameLength, sizeof(version));
        std::memcpy(&minVersion, p + 6 + nameLength, sizeof(minVersion));
        if (version >= T::MsgVersion() && minVersion <= T::MsgVersion())
        {
            std::memcpy(&hash, p + 28 + nameLength + 4 * (T::MsgVersion() - minVersion), sizeof(hash));
            if (hash == T::MsgSchemaHash())
                g_sink = g_sink + 1;
        }
    });
    const double decodeNs = BestNs(count, [&]()
    {
        T out;
        if (protaco::TryToObject(bytes.data(), bytes.size(), &out))
            g_sink = g_sink + 1;
    });
    std::printf("[ PERF     ] %s %zu B: decode %.1f ns, version + schema check %.2f ns (%.2f%%)\n",
        T::MsgName(), bytes.size(), decodeNs, checkNs, 100.0 * checkNs / decodeNs);
#ifdef NDEBUG
    EXPECT_LT(checkNs, decodeNs * 0.1) << T::MsgName();
#endif
}

// Empty string if unset.
std::string GetEnv(const char* name)
{
#ifdef _MSC_VER
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
        return std::string();
    const std::string text(value);
    std::free(value);
    return text;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::string() : std::string(value);
#endif
}

std::string GoldenDir()
{
    const std::string dir = GetEnv("PROTACO_GOLDEN_DIR");
    return dir.empty() ? std::string("protaco_compiler/tests/golden") : dir;
}

std::vector<uint8_t> ReadFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Compares against a golden file. PROTACO_UPDATE_GOLDEN=1 rewrites it (only when the format changes).
void ExpectGolden(const std::string& name, const std::vector<uint8_t>& bytes)
{
    const std::string path = GoldenDir() + "/" + name;
    if (GetEnv("PROTACO_UPDATE_GOLDEN") == "1")
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    const std::vector<uint8_t> golden = ReadFile(path);
    ASSERT_FALSE(golden.empty()) << "golden file missing: " << path;
    EXPECT_EQ(golden, bytes) << name;
}

}  // namespace

TEST(Protaco, RoundTripDeliveryRoute)
{
    const DeliveryRoute sent = MakeDeliveryRoute();
    const std::vector<uint8_t> bytes = sent.Serialize();

    const DeliveryRoute got = protaco::ToObject<DeliveryRoute>(bytes);

    EXPECT_EQ("rider-7", got.get_rider());
    ASSERT_EQ(2u, got.get_stops().size());
    EXPECT_EQ("A-100", got.get_stops()[0].get_orderId());
    EXPECT_EQ("1 Pizza St", got.get_stops()[0].get_address());
    EXPECT_EQ(10, got.get_stops()[0].get_etaMin());
    EXPECT_EQ("closest-first", got.get_note());
    int32_t priority = 0;
    EXPECT_TRUE(got.GetExt().TryGetValue("priority", &priority));
    EXPECT_EQ(5, priority);
    EXPECT_EQ(7u, got.GetEnvelope().id);
    EXPECT_EQ(1000, got.GetEnvelope().ts);
    EXPECT_EQ(bytes, got.Serialize());
}

TEST(Protaco, RoundTripNestedAndVariantFields)
{
    const PizzaOrder sent = MakePizzaOrder();
    const PizzaOrder got = protaco::ToObject<PizzaOrder>(sent.Serialize());

    EXPECT_EQ("A-100", got.get_orderId());
    EXPECT_EQ("mario", got.get_customer());
    EXPECT_TRUE(got.get_woodFired());
    ASSERT_EQ(2u, got.get_toppings().size());
    EXPECT_EQ(5, got.get_toppings()[0].get_grams());
    EXPECT_EQ(30, got.get_toppings()[1].get_grams());
    EXPECT_EQ(std::vector<int32_t>({ 4, 2, 4, 2 }), got.get_delivery().get_doorCode());
    EXPECT_EQ(sent.Serialize(), got.Serialize());
}

// Fixed-only message: Serialize returns the internal buffer and setters write straight into it.
TEST(Protaco, FixedOnlyMessageSerializesInPlace)
{
    Heartbeat beat;
    beat.set_node("oven-1");
    beat.set_uptimeSec(10);
    const std::vector<uint8_t>& bytes = beat.Serialize();
    const uint8_t* before = bytes.data();

    beat.set_uptimeSec(11);   // no second Serialize call

    EXPECT_EQ(11, protaco::ToObject<Heartbeat>(bytes).get_uptimeSec());
    EXPECT_EQ(&bytes, &beat.Serialize());
    EXPECT_EQ(before, beat.Serialize().data());
}

// Generated messages stay nothrow-movable, so a growing std::vector moves them instead of copying.
static_assert(std::is_nothrow_move_constructible<DeliveryRoute>::value, "DeliveryRoute move must be noexcept");
static_assert(std::is_nothrow_move_constructible<RouteStop>::value, "RouteStop move must be noexcept");

// Serialize() reuses the variable block until a variable field or ext changes. Every change below must show up.
TEST(Protaco, SerializeReusesUntilChange)
{
    DeliveryRoute route;
    route.set_rider("rider-7");
    route.set_stops({ MakeStop("A-100", "1 Pizza St", 10) });
    const std::vector<uint8_t> first = route.Serialize();
    EXPECT_EQ(first, route.Serialize());

    route.set_note("rain");
    EXPECT_EQ("rain", protaco::ToObject<DeliveryRoute>(route.Serialize()).get_note());

    route.GetMutableExt().SetValue("priority", 1);
    int32_t priority = 0;
    EXPECT_TRUE(protaco::ToObject<DeliveryRoute>(route.Serialize()).GetExt().TryGetValue("priority", &priority));
    DeliveryRoute other;
    other.GetMutableExt().SetValue("channel", "web");
    route.GetMutableExt() = other.GetExt();   // ext replaced as a whole
    EXPECT_FALSE(protaco::ToObject<DeliveryRoute>(route.Serialize()).GetExt().Has("priority"));

    // Decoding into a used object replaces its bytes too.
    ASSERT_TRUE(Decode(MakeDeliveryRoute().Serialize(), &route, nullptr));
    EXPECT_EQ(MakeDeliveryRoute().Serialize(), route.Serialize());

    // A reference from mutable_ held across Serialize(): later writes through it still show up.
    std::vector<RouteStop>& stops = route.mutable_stops();
    route.Serialize();
    stops.push_back(MakeStop("A-102", "3 Olive Rd", 30));
    EXPECT_EQ(3u, protaco::ToObject<DeliveryRoute>(route.Serialize()).get_stops().size());
    stops.pop_back();
    EXPECT_EQ(2u, protaco::ToObject<DeliveryRoute>(route.Serialize()).get_stops().size());

    // A copy starts from the same bytes and changes on its own.
    DeliveryRoute copy = route;
    EXPECT_EQ(route.Serialize(), copy.Serialize());
    copy.set_rider("rider-8");
    EXPECT_EQ("rider-8", protaco::ToObject<DeliveryRoute>(copy.Serialize()).get_rider());
    EXPECT_EQ("rider-7", protaco::ToObject<DeliveryRoute>(route.Serialize()).get_rider());

    // Fixed fields are written into the buffer, next to a reused variable block.
    PizzaOrder order;
    order.set_customer("mario");
    order.Serialize();
    order.set_size(16);
    EXPECT_EQ(16, protaco::ToObject<PizzaOrder>(order.Serialize()).get_size());
    EXPECT_EQ("mario", protaco::ToObject<PizzaOrder>(order.Serialize()).get_customer());
}

TEST(Protaco, DefaultsApplied)
{
    EXPECT_EQ(15, RouteStop().get_etaMin());
    EXPECT_TRUE(Heartbeat().get_healthy());
    EXPECT_DOUBLE_EQ(25.5, v2::Recipe().get_temp());
    EXPECT_EQ("none", v2::Recipe().get_note());
    EXPECT_EQ(8, PizzaOrder().get_slices());
    EXPECT_EQ("anonymous", PizzaOrder().get_customer());
}

TEST(Protaco, FixedStringOverflowIsRejected)
{
    RouteStop stop;
    EXPECT_TRUE(stop.set_orderId("A-100"));
    EXPECT_FALSE(stop.set_orderId(std::string(17, 'x')));
    EXPECT_EQ("A-100", stop.get_orderId());
    EXPECT_TRUE(stop.set_orderId(std::string(16, 'y')));
    EXPECT_EQ(std::string(16, 'y'), stop.get_orderId());
}

TEST(Protaco, NameMismatchFails)
{
    DeliveryRoute got;
    std::string error;
    EXPECT_FALSE(Decode(Heartbeat().Serialize(), &got, &error));
    EXPECT_EQ("message name mismatch: expected DeliveryRoute, got Heartbeat", error);
}

// Same name, other version, one supported version each: an older frame is too old, a newer one has no fingerprint for the reader.
TEST(Protaco, VersionMismatchFails)
{
    v2::Recipe newer;
    newer.set_path("/menu/a");
    v1::Recipe older;
    std::string error;
    EXPECT_FALSE(Decode(newer.Serialize(), &older, &error));
    EXPECT_EQ("message version too new: Recipe reads version 1, sender supports 2..2", error);

    older.set_path("/menu/b");
    EXPECT_FALSE(Decode(older.Serialize(), &newer, &error));
    EXPECT_EQ("message version too old: Recipe reads version 2 or newer, got 1", error);
    EXPECT_THROW(protaco::ToObject<v2::Recipe>(older.Serialize()), protaco::CParseError);
}

// Same name, version and fixed block size, fields in another order: caught by the fingerprint.
TEST(Protaco, SameVersionDifferentSchemaFails)
{
    alt::Recipe sent;
    sent.set_step(5);
    sent.set_name("R1");
    sent.set_path("/menu/a");
    ASSERT_NE(alt::Recipe::MsgSchemaHash(), v1::Recipe::MsgSchemaHash());

    v1::Recipe got;
    std::string error;
    EXPECT_FALSE(Decode(sent.Serialize(), &got, &error));
    EXPECT_EQ("schema mismatch: Recipe version 1 expected " + protaco::detail::Hex32(v1::Recipe::MsgSchemaHash()) + ", got "
        + protaco::detail::Hex32(alt::Recipe::MsgSchemaHash()), error);
}

// Adding versions leaves the fingerprints of older versions unchanged: a frame from an old build passes.
TEST(Protaco, NewVersionKeepsOldFingerprints)
{
    EXPECT_EQ(build1::Order::MsgSchemaHash(), build3::v1::Order::MsgSchemaHash());
    EXPECT_EQ(build2::Order::MsgSchemaHash(), build3::v2::Order::MsgSchemaHash());
    EXPECT_EQ(build2::Order::MsgSchemaHash(), build4::v2::Order::MsgSchemaHash());
    EXPECT_EQ(build3::Order::MsgSchemaHash(), build4::v3::Order::MsgSchemaHash());
    EXPECT_EQ(1, build3::v1::Order::MsgMinVersion());
    EXPECT_EQ(2, build4::Order::MsgMinVersion());
}

static build3::Order MakeOrderV3()
{
    build3::Order order;
    order.set_id("A1");
    order.set_weight(2.5);
    order.set_color("red");
    order.mutable_tags().push_back("x");
    order.set_label("L");
    order.set_rush(true);
    return order;
}

// A newer frame is read as the reader's version: its own prefix of the fixed block, its own variable fields.
TEST(Protaco, NewerFrameReadAsOwnVersion)
{
    const std::vector<uint8_t> bytes = MakeOrderV3().Serialize();

    const build1::Order reader1 = protaco::ToObject<build1::Order>(bytes);
    EXPECT_EQ("A1", reader1.get_id());
    EXPECT_EQ(1, reader1.get_qty());     // removed in 3: the sender keeps the bytes with the default
    EXPECT_EQ("", reader1.get_note());   // removed in 3: absent, optional
    EXPECT_EQ("red", reader1.get_color());
    std::vector<std::string> extKeys = reader1.GetExt().Keys();   // open message: newer variable fields kept in ext
    std::sort(extKeys.begin(), extKeys.end());
    EXPECT_EQ((std::vector<std::string>{"label", "tags"}), extKeys);

    const build2::Order reader2 = protaco::ToObject<build2::Order>(bytes);
    EXPECT_DOUBLE_EQ(2.5, reader2.get_weight());
    EXPECT_EQ(std::vector<std::string>{"x"}, reader2.get_tags());
    EXPECT_EQ(1, reader2.get_qty());
    EXPECT_EQ(bytes, protaco::ToObject<build3::Order>(bytes).Serialize());
}

// An older frame needs the type of its version; the latest type says which one.
TEST(Protaco, OlderFrameNeedsOldType)
{
    build1::Order sent;
    sent.set_id("B2");
    sent.set_qty(4);
    sent.set_note("n");
    sent.set_color("blue");
    const std::vector<uint8_t> bytes = sent.Serialize();

    build3::Order latest;
    std::string error;
    EXPECT_FALSE(Decode(bytes, &latest, &error));
    EXPECT_EQ("message version too old: Order reads version 3 or newer, got 1. Use the v1 type", error);

    const build3::v1::Order got = protaco::ToObject<build3::v1::Order>(bytes);
    EXPECT_EQ(4, got.get_qty());
    EXPECT_EQ("n", got.get_note());
    EXPECT_EQ("Order{id=\"B2\", qty=4, note=\"n\", color=\"blue\"}", got.ToString());

    build2::Order sent2;
    sent2.set_color("c");
    sent2.set_weight(9.0);
    EXPECT_DOUBLE_EQ(9.0, protaco::ToObject<build3::v2::Order>(sent2.Serialize()).get_weight());
}

// A sender whose min_version is above the reader's version: rejected with both ranges.
TEST(Protaco, SenderMinVersionAboveReader)
{
    build4::Order sent;
    sent.set_id("C3");
    const std::vector<uint8_t> bytes = sent.Serialize();
    build1::Order old;
    std::string error;
    EXPECT_FALSE(Decode(bytes, &old, &error));
    EXPECT_EQ("message version too new: Order reads version 1, sender supports 2..4", error);

    // Version 2 is still served, but color, required there, was removed in 4.
    build2::Order reader2;
    EXPECT_FALSE(Decode(bytes, &reader2, &error));
    EXPECT_EQ("color: missing required (frame version 4 read as 2)", error);

    // build4 relays a version 3 frame with its v3 type; version 2 readers still get color.
    build3::Order sent3;
    sent3.set_color("green");
    EXPECT_EQ("green", protaco::ToObject<build2::Order>(protaco::ToObject<build4::v3::Order>(sent3.Serialize()).Serialize()).get_color());
}

// A closed message drops the fields of a newer version; an unknown key in a same-version frame is still an error.
TEST(Protaco, ClosedMessageDropsNewerFields)
{
    build2::Ping sent;
    sent.set_seq(7);
    sent.set_who("w");
    sent.set_text("hi");
    const build1::Ping got = protaco::ToObject<build1::Ping>(sent.Serialize());
    EXPECT_EQ(7, got.get_seq());
    EXPECT_EQ("w", got.get_who());

    build1::Ping same;
    std::string error;
    EXPECT_FALSE(Decode(WithExtraTlv(got.Serialize(), "text", 1), &same, &error));
    EXPECT_EQ("text: unknown key (allow_extension false)", error);
}

// The fingerprint covers what is serialized only: namespace, comments and desc do not change it.
TEST(Protaco, FingerprintIgnoresNamespace)
{
    EXPECT_EQ(partial::Topping::MsgSchemaHash(), Topping::MsgSchemaHash());
    partial::Topping sent;
    sent.set_name("basil");
    EXPECT_EQ("basil", protaco::ToObject<Topping>(sent.Serialize()).get_name());
}

// allow_extension false: a key not in the yaml is an error, not ext data.
TEST(Protaco, ClosedMessageRejectsUnknownKey)
{
    Heartbeat got;
    std::string error;
    EXPECT_FALSE(Decode(WithExtraTlv(MakeHeartbeat().Serialize(), "abc", 7), &got, &error));
    EXPECT_EQ("abc: unknown key (allow_extension false)", error);

    // An open message keeps the same key in ext.
    DeliveryRoute open;
    ASSERT_TRUE(Decode(WithExtraTlv(MakeDeliveryRoute().Serialize(), "abc", 7), &open, &error));
    int32_t value = 0;
    EXPECT_TRUE(open.GetExt().TryGetValue("abc", &value));
    EXPECT_EQ(7, value);
}

// The following tests patch the schema hash to exercise the body decoder on a foreign body.
TEST(Protaco, MissingRequiredFails)
{
    partial::DeliveryRoute sender;
    sender.set_rider("rider-7");
    const std::vector<uint8_t> bytes = WithSchemaHash(sender.Serialize(), DeliveryRoute::MsgSchemaHash());

    DeliveryRoute got;
    std::string error;
    EXPECT_FALSE(Decode(bytes, &got, &error));
    EXPECT_EQ("stops: missing required", error);
    EXPECT_THROW(protaco::ToObject<DeliveryRoute>(bytes), protaco::CParseError);
}

TEST(Protaco, NestedErrorHasPath)
{
    partial::PizzaOrder sender;
    sender.mutable_toppings().push_back(partial::Topping());
    sender.mutable_delivery().mutable_doorCode().push_back("x");   // the receiver expects list<int>

    PizzaOrder got;
    std::string error;
    EXPECT_FALSE(Decode(WithSchemaHash(sender.Serialize(), PizzaOrder::MsgSchemaHash()), &got, &error));
    EXPECT_EQ("delivery.doorCode: list element type mismatch", error);
}

TEST(Protaco, FixedBlockSizeMismatchFails)
{
    v1::Recipe sender;
    sender.set_path("/p");
    std::vector<uint8_t> bytes = sender.Serialize();
    bytes[VersionAt(bytes)] = 2;       // version 1 -> 2
    bytes[VersionAt(bytes) + 2] = 2;   // min version 2, still one fingerprint
    bytes = WithSchemaHash(bytes, v2::Recipe::MsgSchemaHash());

    v2::Recipe got;
    std::string error;
    EXPECT_FALSE(Decode(bytes, &got, &error));
    EXPECT_EQ("fixed block size mismatch: expected 28, got 20", error);
}

// 8 nested levels is the limit (generator PC406, runtime kMaxDepth).
TEST(Protaco, EightNestedLevelsRoundTrip)
{
    deep::D8 sent;
    sent.mutable_d().mutable_d().mutable_d().mutable_d().mutable_d().mutable_d().mutable_d().mutable_d().set_v(42);
    const deep::D8 got = protaco::ToObject<deep::D8>(sent.Serialize());
    EXPECT_EQ(42, got.get_d().get_d().get_d().get_d().get_d().get_d().get_d().get_d().get_v());
    EXPECT_EQ("D8{d=D7{d=D6{d=D5{d=D4{d=D3{d=D2{d=D1{d=D0{v=42}}}}}}}}}", got.ToString());
}

TEST(Protaco, ExtRejectsSchemaAndReservedKeys)
{
    DeliveryRoute route;
    std::string error;
    EXPECT_FALSE(route.GetMutableExt().SetValue("rider", 1, &error));   // schema key
    EXPECT_EQ("ext key is a field of the message: rider", error);
    EXPECT_FALSE(route.GetMutableExt().SetValue("_x", 1, &error));      // reserved
    EXPECT_EQ("ext key starts with '_' (reserved): _x", error);
    EXPECT_FALSE(route.GetMutableExt().SetValue("", 1, &error));
    EXPECT_EQ("ext key is empty", error);
    EXPECT_TRUE(route.GetMutableExt().SetValue("channel", "web"));
    std::string value;
    EXPECT_TRUE(route.GetExt().TryGetValue("channel", &value));
    EXPECT_EQ("web", value);
    int32_t wrongType = 7;
    EXPECT_FALSE(route.GetExt().TryGetValue("channel", &wrongType, &error));
    EXPECT_EQ("ext key channel holds string, not int", error);
    EXPECT_EQ(7, wrongType);
    EXPECT_FALSE(route.GetExt().TryGetValue("missing", &value, &error));
    EXPECT_EQ("ext key not found: missing", error);
    EXPECT_FALSE(route.GetExt().TryGetValue("missing", &value));   // error text is optional
}

// Unsigned types at their limits, and UTF-8 text in variable and fixed strings. Same values as the C# test MakeNumbers.
TEST(Protaco, UnsignedTypesAndUtf8RoundTrip)
{
    const scalars::Numbers defaults;
    EXPECT_EQ(4000000000u, defaults.get_u32());
    EXPECT_EQ(18446744073709551615ull, defaults.get_u64());
    EXPECT_EQ(-9000000000ll, defaults.get_i64());

    const scalars::Numbers sent = MakeNumbers();
    const scalars::Numbers got = protaco::ToObject<scalars::Numbers>(sent.Serialize());
    EXPECT_EQ(0u, got.get_u32());
    EXPECT_EQ(std::vector<uint32_t>({ 0u, 1u, 4294967295u }), got.get_us());
    EXPECT_EQ(std::vector<uint64_t>({ 18446744073709551615ull }), got.get_u64s());
    EXPECT_EQ(u8"한글 ✓", got.get_name());
    EXPECT_EQ(u8"café 寿司", got.get_text());
    EXPECT_EQ(u8"Numbers{u32=0, u64=18446744073709551615, i64=-9000000000, us=[0, 1, 4294967295], u64s=[18446744073709551615], "
              u8"name=\"한글 ✓\", text=\"café 寿司\"}",
        got.ToString());
    ExpectGolden("Numbers.bin", sent.Serialize());

    std::string error;
    scalars::Numbers fixed;
    EXPECT_FALSE(fixed.set_name(std::string(17, 'x'), &error));
    EXPECT_EQ("name: 17 bytes, longer than string(16)", error);
    EXPECT_TRUE(fixed.set_name(u8"한글한글한", &error));   // 15 UTF-8 bytes
}

static_assert(protaco::kMaxFrameBytes == 0xFFFFFFFFu, "frames up to 4 GiB - 1");

// Enums: defaults, lists, numbers this build does not know, names of removed values, parsing. Same as the C# test.
TEST(Protaco, EnumFields)
{
    using enums::PizzaSize;
    const enums::Pie defaults;
    EXPECT_EQ(PizzaSize::Large, defaults.get_size());
    EXPECT_EQ(PizzaSize::Small, defaults.get_crust());   // no default: first value

    const enums::Pie sent = MakePie();
    const enums::Pie got = protaco::ToObject<enums::Pie>(sent.Serialize());
    EXPECT_EQ(PizzaSize::Family, got.get_size());
    EXPECT_EQ(9, static_cast<int32_t>(got.get_crust()));
    EXPECT_EQ(sent.get_sizes(), got.get_sizes());
    EXPECT_EQ("Pie{size=Family(4), crust=?(9), sizes=[Small(1), Family(4), Kids(5), ?(9)], name=\"\"}", got.ToString());
    ExpectGolden("Pie.bin", sent.Serialize());

    PizzaSize parsed = PizzaSize::Small;
    EXPECT_TRUE(enums::TryParseEnum("Medium", &parsed));
    EXPECT_EQ(PizzaSize::Medium, parsed);
    std::string error;
    EXPECT_FALSE(enums::TryParseEnum("Kids", &parsed, &error));   // removed in version 3
    EXPECT_EQ("unknown PizzaSize name: Kids", error);
    EXPECT_EQ(PizzaSize::Medium, parsed);
    EXPECT_STREQ("Kids", enums::EnumName(static_cast<PizzaSize>(5)));
    EXPECT_EQ(nullptr, enums::EnumName(static_cast<PizzaSize>(9)));
}

// Maps: every key and value kind, nested list and map values, sorted ToString, duplicate keys. Same as the C# test.
TEST(Protaco, MapFields)
{
    EXPECT_EQ(u8"Menu{prices={\"dupA\": 9.5}, stock={-1: 4294967295}, specials={Silver(2): Dish{name=\"ragu\", price=12.25}}, notes={18446744073709551615: \"매운맛\"}, vegan={\"ragu\": false}, photos={\"ragu\": <2 bytes: FF 00>}, grades={\"ragu\": ?(9)}, tags={\"ragu\": [\"hot\", \"\"]}, daily={20261020: {\"ragu\": 3}}, title=\"lunch\"}", MakeMenu().ToString());
    ExpectGolden("Menu.bin", MakeMenu().Serialize());

    maps::Menu sent = MakeMenu();
    sent.mutable_prices()["dupB"] = 7.0;
    sent.mutable_prices()["coffee"] = 3.5;
    sent.mutable_stock()[2] = 0u;
    const maps::Menu got = protaco::ToObject<maps::Menu>(sent.Serialize());
    EXPECT_EQ(sent.get_prices(), got.get_prices());
    EXPECT_EQ(sent.get_stock(), got.get_stock());
    EXPECT_EQ(sent.get_notes(), got.get_notes());
    EXPECT_EQ(sent.get_vegan(), got.get_vegan());
    EXPECT_EQ(sent.get_photos(), got.get_photos());
    EXPECT_EQ(sent.get_tags(), got.get_tags());
    EXPECT_EQ(sent.get_daily(), got.get_daily());
    EXPECT_EQ(9, static_cast<int32_t>(got.get_grades().at("ragu")));
    EXPECT_EQ("Dish{name=\"ragu\", price=12.25}", got.get_specials().at(maps::Grade::Silver).ToString());
    EXPECT_EQ(0u, got.GetExt().Count());
    EXPECT_EQ(std::string(u8"Menu{prices={\"coffee\": 3.5, \"dupA\": 9.5, \"dupB\": 7}, stock={-1: 4294967295, 2: 0}, specials={Silver(2): Dish{name=\"ragu\", price=12.25}}, "), got.ToString().substr(0, got.ToString().find("notes=")));

    // Same key twice: "dupB" patched to "dupA". The entry index depends on the iteration order.
    std::vector<uint8_t> bytes = sent.Serialize();
    const uint8_t dupB[] = { 4, 0, 0, 0, 'd', 'u', 'p', 'B' };
    const auto at = std::search(bytes.begin(), bytes.end(), std::begin(dupB), std::end(dupB));
    ASSERT_NE(bytes.end(), at);
    at[7] = 'A';
    maps::Menu bad;
    std::string error;
    EXPECT_FALSE(protaco::TryToObject(bytes, &bad, &error));
    EXPECT_EQ(0u, error.find("prices["));
    EXPECT_NE(std::string::npos, error.find("]: duplicate map key"));
}

// A message that refers to itself: list<Node> and an optional Node. Same as the C# test.
TEST(Protaco, RecursiveMessages)
{
    const recursive::Node sent = MakeTree();
    const recursive::Node got = protaco::ToObject<recursive::Node>(sent.Serialize());
    EXPECT_EQ("Node{name=\"root\", value=1, children=[Node{name=\"a\", value=2, children=[], next=Node{name=\"b\", value=3, children=[], next=null}}, Node{name=\"c\", value=4, children=[], next=null}], next=null}", got.ToString());
    ExpectGolden("Node.bin", sent.Serialize());
    EXPECT_FALSE(got.has_next());
    EXPECT_EQ("", got.get_next().get_name());   // absent: empty node
    EXPECT_TRUE(got.get_children()[0].has_next());
    EXPECT_EQ("b", got.get_children()[0].get_next().get_name());

    recursive::Node copy = got;
    copy.mutable_children()[0].mutable_next().set_name("changed");
    EXPECT_EQ("b", got.get_children()[0].get_next().get_name());   // deep copy
    copy.set_next(copy);   // a node into its own field
    EXPECT_EQ("root", copy.get_next().get_name());
    EXPECT_FALSE(copy.get_next().has_next());
    copy.clear_next();
    EXPECT_FALSE(copy.has_next());
    EXPECT_EQ(sent.Serialize().size(), copy.Serialize().size() - std::string("changed").size() + 1);
}

// Self references have no static depth: the decoder stops at kMaxDepth (100) nested levels.
TEST(Protaco, RecursionDepthLimit)
{
    static_assert(protaco::kMaxDepth == 100, "runtime nesting limit");
    for (int levels : { 100, 101 })
    {
        recursive::Node root;
        recursive::Node* node = &root;
        for (int i = 0; i < levels; ++i)
            node = &node->mutable_next();
        node->set_value(levels);
        recursive::Node got;
        std::string error;
        const bool ok = protaco::TryToObject(root.Serialize(), &got, &error);
        if (levels == 100)
        {
            EXPECT_TRUE(ok) << error;
        }
        else
        {
            EXPECT_FALSE(ok);
            EXPECT_NE(std::string::npos, error.find(": nesting too deep")) << error;
            EXPECT_EQ(0u, error.find("next.next.next.")) << error;
        }
    }
}

// Fixed-only messages: TryView reads the frame in place. Same acceptance and error text as TryToObject.
TEST(Protaco, ViewReadsFixedFieldsInPlace)
{
    Heartbeat sent = MakeHeartbeat();
    sent.SetCorr(77);
    const std::vector<uint8_t> bytes = sent.Serialize();
    Heartbeat::View view;
    ASSERT_TRUE(protaco::TryView<Heartbeat>(bytes, &view));
    EXPECT_EQ("oven-1", view.get_node());
    EXPECT_EQ(123456789, view.get_seq());
    EXPECT_TRUE(view.get_healthy());
    EXPECT_DOUBLE_EQ(0.25, view.get_cpuLoad());
    EXPECT_EQ(3600, view.get_uptimeSec());
    EXPECT_EQ(77u, view.GetCorr());

    std::string viewError;
    std::string objectError;
    Heartbeat object;
    for (size_t length = 0; length < bytes.size(); ++length)
    {
        EXPECT_FALSE(protaco::TryView<Heartbeat>(bytes.data(), length, &view, &viewError)) << length;
        EXPECT_FALSE(protaco::TryToObject(bytes.data(), length, &object, &objectError)) << length;
        EXPECT_EQ(objectError, viewError) << length;
    }
    const std::vector<uint8_t> extra = WithExtraTlv(bytes, "abc", 7);
    EXPECT_FALSE(protaco::TryView<Heartbeat>(extra, &view, &viewError));
    EXPECT_EQ("abc: unknown key (allow_extension false)", viewError);
    EXPECT_FALSE(protaco::TryView<Heartbeat>(MakeDeliveryRoute().Serialize(), &view, &viewError));
    EXPECT_EQ("message name mismatch: expected Heartbeat, got DeliveryRoute", viewError);
}

// ToObject with std::vector&&: a frame of the type's own layout becomes the object's buffer, Serialize() returns it.
TEST(Protaco, ToObjectAdoptsOwnFrame)
{
    std::vector<uint8_t> bytes = MakeDeliveryRoute().Serialize();
    const std::vector<uint8_t> copy = bytes;
    const uint8_t* data = bytes.data();
    DeliveryRoute got = protaco::ToObject<DeliveryRoute>(std::move(bytes));
    EXPECT_EQ(data, got.Serialize().data());   // no copy, no re-encode
    EXPECT_EQ(copy, got.Serialize());
    EXPECT_EQ("rider-7", got.get_rider());
    EXPECT_EQ(7u, got.GetId());
    got.set_note("changed");
    EXPECT_EQ("changed", protaco::ToObject<DeliveryRoute>(got.Serialize()).get_note());

    std::vector<uint8_t> again = copy;
    data = again.data();
    DeliveryRoute out;
    std::string error;
    ASSERT_TRUE(protaco::TryToObject(std::move(again), &out, &error)) << error;
    EXPECT_EQ(data, out.Serialize().data());
    EXPECT_EQ(copy, out.Serialize());

    // A newer frame is copied and read as the older type.
    std::vector<uint8_t> newer = MakeOrderV3().Serialize();
    const build1::Order older = protaco::ToObject<build1::Order>(std::move(newer));
    EXPECT_EQ("A1", older.get_id());
    EXPECT_EQ("red", older.get_color());

    std::vector<uint8_t> broken = copy;
    broken.pop_back();
    EXPECT_FALSE(protaco::TryToObject(std::move(broken), &out, &error));
    EXPECT_EQ("frame truncated", error);
}

// More TLVs than the decoder keeps on the stack (16): the rest goes to the heap, nothing is lost.
TEST(Protaco, ManyExtEntriesRoundTrip)
{
    DeliveryRoute sent = MakeDeliveryRoute();
    for (int i = 0; i < 40; ++i)
        ASSERT_TRUE(sent.GetMutableExt().SetValue("k" + std::to_string(i), i));
    const DeliveryRoute got = protaco::ToObject<DeliveryRoute>(sent.Serialize());
    EXPECT_EQ(41u, got.GetExt().Count());
    int32_t value = 0;
    EXPECT_TRUE(got.GetExt().TryGetValue("k39", &value));
    EXPECT_EQ(39, value);
    EXPECT_EQ(2u, got.get_stops().size());
    EXPECT_EQ(sent.Serialize(), got.Serialize());
}

// New messages take the next id and the clock time; decoded messages and nested fields take neither.
TEST(Protaco, EnvelopeIsStampedOnCreation)
{
    protaco::SetClock([]() -> int64_t { return 1760000000123456; });
    const DeliveryRoute first;
    const DeliveryRoute second;
    EXPECT_EQ(first.GetId() + 1, second.GetId());
    EXPECT_EQ(1760000000123456, second.GetTimestamp());
    EXPECT_EQ(0u, second.GetCorr());

    const PizzaOrder order;   // one id; its nested Delivery field takes none
    const DeliveryRoute decoded = protaco::ToObject<DeliveryRoute>(first.Serialize());
    const DeliveryRoute third;
    EXPECT_EQ(second.GetId() + 2, third.GetId());
    EXPECT_EQ(first.GetId(), decoded.GetId());
    EXPECT_EQ(first.GetTimestamp(), decoded.GetTimestamp());
    protaco::SetClock(nullptr);

    DeliveryRoute reply;
    reply.SetCorr(first.GetId());
    reply.SetId(77);
    reply.SetTimestampAsNow();
    EXPECT_EQ(first.GetId(), reply.GetCorr());
    EXPECT_EQ(77u, reply.GetId());
    EXPECT_GT(reply.GetTimestamp(), protaco::kTimestampMin);
    EXPECT_EQ(first.GetId(), protaco::ToObject<DeliveryRoute>(reply.Serialize()).GetCorr());
}

// auto_envelope false (Tick) and DisableAutoEnvelope() leave id and ts at 0 and take no id from the counter.
TEST(Protaco, AutoEnvelopeCanBeTurnedOff)
{
    const DeliveryRoute before;
    const scalars::Tick tick;
    EXPECT_EQ(0u, tick.GetId());
    EXPECT_EQ(0, tick.GetTimestamp());

    protaco::DisableAutoEnvelope();
    const DeliveryRoute off;
    protaco::EnableAutoEnvelope();
    EXPECT_EQ(0u, off.GetId());
    EXPECT_EQ(0, off.GetTimestamp());

    const DeliveryRoute after;
    EXPECT_EQ(before.GetId() + 1, after.GetId());

    scalars::Tick sent;
    sent.SetId(protaco::NextId());
    sent.SetTimestampAsNow();
    const scalars::Tick got = protaco::ToObject<scalars::Tick>(sent.Serialize());
    EXPECT_EQ(sent.GetId(), got.GetId());
    EXPECT_EQ(sent.GetTimestamp(), got.GetTimestamp());
}

// Timestamps are UTC microseconds; TryGetTimeFromTimestamp and Pretty() show local time. Same as the C# test.
TEST(Protaco, TimestampConvertsToLocalTime)
{
    std::tm local = std::tm();
    local.tm_year = 2026 - 1900;
    local.tm_mon = 9;
    local.tm_mday = 20;
    local.tm_hour = 14;
    local.tm_min = 3;
    local.tm_sec = 22;
    local.tm_isdst = -1;
    const int64_t timestamp = static_cast<int64_t>(std::mktime(&local)) * 1000000 + 123456;

    DeliveryRoute route;
    route.SetId(9);
    route.SetTimestamp(timestamp);
    std::string date;
    std::string time;
    ASSERT_TRUE(route.TryGetTimeFromTimestamp(&date, &time));
    EXPECT_EQ("2026-10-20", date);
    EXPECT_EQ("14:03:22.123456", time);
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millisecond = 0;
    int microsecond = 0;
    ASSERT_TRUE(route.TryGetTimeFromTimestamp(&year, &month, &day, &hour, &minute, &second, &millisecond, &microsecond));
    EXPECT_EQ(2026, year);
    EXPECT_EQ(10, month);
    EXPECT_EQ(20, day);
    EXPECT_EQ(14, hour);
    EXPECT_EQ(3, minute);
    EXPECT_EQ(22, second);
    EXPECT_EQ(123, millisecond);
    EXPECT_EQ(456, microsecond);
    EXPECT_EQ("DeliveryRoute{\n  envelope{id=9, corr=0, ts=2026-10-20 14:03:22.123456},\n  rider=\"\",\n  stops=[],\n  note=\"\"\n}",
        route.ToString(protaco::TextStyle::Pretty()));

    route.SetTimestamp(1000);
    std::string error;
    EXPECT_FALSE(route.TryGetTimeFromTimestamp(&date, &time, &error));
    EXPECT_EQ("timestamp 1000 is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)", error);
    EXPECT_EQ("DeliveryRoute{\n  envelope{id=9, corr=0, ts=1000},\n  rider=\"\",\n  stops=[],\n  note=\"\"\n}",
        route.ToString(protaco::TextStyle::Pretty()));
}

TEST(Protaco, PeekHeaderReadsEnvelopeWithoutBody)
{
    const DeliveryRoute route = MakeDeliveryRoute();
    protaco::Header header;
    ASSERT_TRUE(protaco::PeekHeader(route.Serialize().data(), route.Serialize().size(), &header));
    EXPECT_EQ("DeliveryRoute", header.name);
    EXPECT_EQ(1, header.version);
    EXPECT_EQ(1, header.minVersion);
    EXPECT_EQ(DeliveryRoute::MsgSchemaHash(), header.schemaHash);
    EXPECT_EQ(7u, header.env.id);

    const std::vector<uint8_t> versions = MakeOrderV3().Serialize();
    ASSERT_TRUE(protaco::PeekHeader(versions.data(), versions.size(), &header));
    EXPECT_EQ(3, header.version);
    EXPECT_EQ(1, header.minVersion);
    EXPECT_EQ(build3::Order::MsgSchemaHash(), header.schemaHash);
}

// ToString: same text as C# (tests/cs/Program.cs ToStringMatchesCpp).
TEST(Protaco, ToStringCompact)
{
    EXPECT_EQ("DeliveryRoute{rider=\"rider-7\", stops=[RouteStop{orderId=\"A-100\", address=\"1 Pizza St\", etaMin=10}, "
              "RouteStop{orderId=\"A-101\", address=\"2 Pasta Ave\", etaMin=25}], note=\"closest-first\", ext{priority=5}}",
        MakeDeliveryRoute().ToString());
    EXPECT_EQ("Heartbeat{node=\"oven-1\", seq=123456789, healthy=true, cpuLoad=0.25, uptimeSec=3600}", MakeHeartbeat().ToString());
    EXPECT_EQ("DeliveryProof{orderId=\"A-1001\", photo=<11 bytes: FF D8 00 10 4A 46 49 46 ...>, "
              "signatures=[<1 bytes: 00>, <0 bytes>, <2 bytes: 01 02>]}",
        MakeDeliveryProof().ToString());
    EXPECT_EQ("Recipe{name=\"\", temp=25.5, step=3, path=\"\", note=\"none\"}", v2::Recipe().ToString());
}

TEST(Protaco, ToStringPretty)
{
    EXPECT_EQ("DeliveryRoute{\n"
              "  envelope{id=7, corr=0, ts=1000},\n"
              "  rider=\"rider-7\",\n"
              "  stops=[\n"
              "    RouteStop{\n"
              "      orderId=\"A-100\",\n"
              "      address=\"1 Pizza St\",\n"
              "      etaMin=10\n"
              "    },\n"
              "    RouteStop{\n"
              "      orderId=\"A-101\",\n"
              "      address=\"2 Pasta Ave\",\n"
              "      etaMin=25\n"
              "    }\n"
              "  ],\n"
              "  note=\"closest-first\",\n"
              "  ext{\n"
              "    priority=5\n"
              "  }\n"
              "}",
        MakeDeliveryRoute().ToString(protaco::TextStyle::Pretty()));
}

// ext entries of every kind, custom brackets and separator, escaping and cut.
TEST(Protaco, ToStringExtStyleAndEscape)
{
    RouteStop stop = MakeStop("a\"b\\c\n", "Main", 3);
    protaco::TextStyle style;
    style.separator = "; ";
    style.open = "(";
    style.close = ")";
    style.maxText = 4;
    EXPECT_EQ("RouteStop(orderId=\"a\\\"b\\\\\"...(+2); address=\"Main\"; etaMin=3)", stop.ToString(style));

    DeliveryRoute route;
    route.set_rider("rider-7");
    protaco::CExt& ext = route.GetMutableExt();
    ext.SetValue("priority", 5);
    ext.SetValue("channel", "web");
    ext.SetValue("hot", true);
    ext.SetValue("ratio", 0.5);
    ext.SetValue("big", int64_t(1234567890123));
    ext.SetValue("blob", std::vector<uint8_t>{0x00, 0xFF});
    EXPECT_EQ("DeliveryRoute{rider=\"rider-7\", stops=[], note=\"\", "
              "ext{priority=5, channel=\"web\", hot=true, ratio=0.5, big=1234567890123, blob=<2 bytes: 00 FF>}}",
        route.ToString());
}

// Truncated input fails at every length without crashing.
TEST(Protaco, TruncatedInputNeverCrashes)
{
    const std::vector<uint8_t> bytes = MakePizzaOrder().Serialize();
    for (size_t length = 0; length < bytes.size(); ++length)
    {
        PizzaOrder got;
        EXPECT_FALSE(protaco::TryToObject(bytes.data(), length, &got)) << "length " << length;
    }
}

// Random bit flips never crash. When decoding succeeds, the re-serialized bytes decode too.
TEST(Protaco, BitFlipsNeverCrash)
{
    const std::vector<uint8_t> original = MakePizzaOrder().Serialize();
    std::mt19937 random(20261015);
    for (int round = 0; round < 5000; ++round)
    {
        std::vector<uint8_t> bytes = original;
        const int flips = 1 + static_cast<int>(random() % 4);
        for (int i = 0; i < flips; ++i)
            bytes[random() % bytes.size()] ^= static_cast<uint8_t>(1u << (random() % 8));
        PizzaOrder got;
        if (protaco::TryToObject(bytes.data(), bytes.size(), &got))
        {
            EXPECT_TRUE(protaco::TryToObject(got.Serialize().data(), got.Serialize().size(), &got));
        }
    }
}

// bytes: binary values survive a round trip, in fields, lists and ext.
TEST(Protaco, RoundTripBytes)
{
    DeliveryProof sent = MakeDeliveryProof();
    const std::vector<uint8_t> blob = {0x00, 0x00, 0xFF};
    ASSERT_TRUE(sent.GetMutableExt().SetValue("thumbnail", blob));
    const DeliveryProof got = protaco::ToObject<DeliveryProof>(sent.Serialize());
    EXPECT_EQ(sent.get_photo(), got.get_photo());
    EXPECT_EQ(sent.get_signatures(), got.get_signatures());
    EXPECT_TRUE(got.get_signatures()[1].empty());
    std::vector<uint8_t> thumbnail;
    ASSERT_TRUE(got.GetExt().TryGetValue("thumbnail", &thumbnail));
    EXPECT_EQ(blob, thumbnail);
    std::string asString;
    EXPECT_FALSE(got.GetExt().TryGetValue("thumbnail", &asString));
}

// The contract checks on receive cost under 10% of a decode, smallest message (fixed only) included.
TEST(Protaco, ContractCheckCostIsNegligible)
{
    ExpectCheckCostNegligible(MakeHeartbeat(), 200000);
    ExpectCheckCostNegligible(MakePizzaOrder(), 50000);
}

// Golden bytes: the format must not change, and C# must produce the same bytes.
TEST(Protaco, GoldenBytes)
{
    ExpectGolden("DeliveryRoute.bin", MakeDeliveryRoute().Serialize());
    ExpectGolden("PizzaOrder.bin", MakePizzaOrder().Serialize());
    ExpectGolden("Heartbeat.bin", MakeHeartbeat().Serialize());
    ExpectGolden("DeliveryProof.bin", MakeDeliveryProof().Serialize());
}
