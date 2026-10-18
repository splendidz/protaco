// ProtacoTest.cpp. Unit tests (gtest) for the Protaco.hpp runtime and protacoc generated code.
// Covers: round trip, in-place fixed fields, defaults, version and schema checks, version window, closed messages,
// nesting depth, error paths, ext, ToString, corrupt input, golden bytes, cost of the contract checks.
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "DeliveryProof.g.h"
#include "Heartbeat.g.h"
#include "PizzaOrder.g.h"
#include "TransferRequest.g.h"
#include "alt/Recipe.g.h"
#include "build1/Order.g.h"
#include "build1/Ping.g.h"
#include "build2/Order.g.h"
#include "build2/Ping.g.h"
#include "build3/Order.g.h"
#include "build4/Order.g.h"
#include "deep/D8.g.h"
#include "partial/PizzaOrder.g.h"
#include "partial/TransferRequest.g.h"
#include "ver_v1/Recipe.g.h"
#include "ver_v2/Recipe.g.h"

using namespace demo;

namespace
{

MoveInfo MakeMove(const std::string& from, const std::string& to, int32_t slot)
{
    MoveInfo move;
    move.set_from(from);
    move.set_to(to);
    move.set_slot(slot);
    return move;
}

// Golden values. Same as the C# test (tests/cs/Program.cs MakeTransferRequest).
TransferRequest MakeTransferRequest()
{
    TransferRequest request;
    request.set_tm("TM1");
    request.get_mutable_moves().push_back(MakeMove("LP1", "PM1", 3));
    request.get_mutable_moves().push_back(MakeMove("PM1", "LP1", 3));
    request.set_reason("rule-1");
    request.get_mutable_ext().SetInt("priority", 5);
    protaco::Envelope env;
    env.id = 7;
    env.corr = 0;
    env.ts = 1000;
    request.SetEnvelope(env);
    return request;
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
    order.get_mutable_toppings().push_back(basil);
    Topping cheese;
    cheese.set_name("mozzarella");
    order.get_mutable_toppings().push_back(cheese);
    order.get_mutable_notes().push_back("no onion");
    order.get_mutable_delivery().set_address("1 Pizza St");
    order.get_mutable_delivery().set_floor(3);
    order.get_mutable_delivery().get_mutable_doorCode() = { 4, 2, 4, 2 };
    order.get_mutable_delivery().get_mutable_contacts().push_back("010-0000-0000");
    return order;
}

Heartbeat MakeHeartbeat()
{
    Heartbeat beat;
    beat.set_node("oven-1");
    beat.set_seq(123456789);
    beat.set_cpuLoad(0.25);
    beat.set_uptimeSec(3600);
    return beat;
}

// Golden values. Same as the C# test MakeDeliveryProof. Zeros, 0xFF and an empty element included.
DeliveryProof MakeDeliveryProof()
{
    DeliveryProof proof;
    proof.set_orderId("A-1001");
    proof.set_photo({0xFF, 0xD8, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00, 0xFF, 0xD9});
    proof.get_mutable_signatures().push_back({0x00});
    proof.get_mutable_signatures().push_back({});
    proof.get_mutable_signatures().push_back({0x01, 0x02});
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
bool Decode(const std::vector<uint8_t>& bytes, T* out, protaco::Error* error)
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
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
        return std::string();
    const std::string text(value);
    std::free(value);
    return text;
}

std::string GoldenDir()
{
    const std::string dir = GetEnv("PROTACO_GOLDEN_DIR");
    return dir.empty() ? std::string("tests/golden") : dir;
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
    if (!GetEnv("PROTACO_UPDATE_GOLDEN").empty())
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    const std::vector<uint8_t> golden = ReadFile(path);
    ASSERT_FALSE(golden.empty()) << "golden file missing: " << path;
    EXPECT_EQ(golden, bytes) << name;
}

}  // namespace

TEST(Protaco, RoundTripTransferRequest)
{
    const TransferRequest sent = MakeTransferRequest();
    const std::vector<uint8_t> bytes = sent.Serialize();

    const TransferRequest got = protaco::ToObject<TransferRequest>(bytes);

    EXPECT_EQ("TM1", got.get_tm());
    ASSERT_EQ(2u, got.get_moves().size());
    EXPECT_EQ("LP1", got.get_moves()[0].get_from());
    EXPECT_EQ("PM1", got.get_moves()[0].get_to());
    EXPECT_EQ(3, got.get_moves()[0].get_slot());
    EXPECT_EQ("rule-1", got.get_reason());
    int32_t priority = 0;
    EXPECT_TRUE(got.get_ext().GetInt("priority", &priority));
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

TEST(Protaco, DefaultsApplied)
{
    EXPECT_EQ(1, MoveInfo().get_slot());
    EXPECT_TRUE(Heartbeat().get_healthy());
    EXPECT_DOUBLE_EQ(25.5, v2::Recipe().get_temp());
    EXPECT_EQ("none", v2::Recipe().get_note());
    EXPECT_EQ(8, PizzaOrder().get_slices());
    EXPECT_EQ("anonymous", PizzaOrder().get_customer());
}

TEST(Protaco, FixedStringOverflowIsRejected)
{
    MoveInfo move;
    EXPECT_TRUE(move.set_from("LP1"));
    EXPECT_FALSE(move.set_from(std::string(33, 'x')));
    EXPECT_EQ("LP1", move.get_from());
    EXPECT_TRUE(move.set_from(std::string(32, 'y')));
    EXPECT_EQ(std::string(32, 'y'), move.get_from());
}

TEST(Protaco, NameMismatchFails)
{
    TransferRequest got;
    protaco::Error error;
    EXPECT_FALSE(Decode(Heartbeat().Serialize(), &got, &error));
    EXPECT_EQ("message name mismatch: expected TransferRequest, got Heartbeat", error.what);
}

// Same name, other version without a window: an older frame is too old, a newer one has no fingerprint for the reader.
TEST(Protaco, VersionMismatchFails)
{
    v2::Recipe newer;
    newer.set_path("/rcp/a");
    v1::Recipe older;
    protaco::Error error;
    EXPECT_FALSE(Decode(newer.Serialize(), &older, &error));
    EXPECT_EQ("message version too new: Recipe reads version 1, sender supports 2..2", error.what);

    older.set_path("/rcp/b");
    EXPECT_FALSE(Decode(older.Serialize(), &newer, &error));
    EXPECT_EQ("message version too old: Recipe reads version 2 or newer, got 1", error.what);
    EXPECT_THROW(protaco::ToObject<v2::Recipe>(older.Serialize()), protaco::CParseError);
}

// Same name, version and fixed block size, fields in another order: caught by the fingerprint.
TEST(Protaco, SameVersionDifferentSchemaFails)
{
    alt::Recipe sent;
    sent.set_step(5);
    sent.set_name("R1");
    sent.set_path("/rcp/a");
    ASSERT_NE(alt::Recipe::MsgSchemaHash(), v1::Recipe::MsgSchemaHash());

    v1::Recipe got;
    protaco::Error error;
    EXPECT_FALSE(Decode(sent.Serialize(), &got, &error));
    EXPECT_EQ("schema mismatch: Recipe version 1 expected " + protaco::detail::Hex32(v1::Recipe::MsgSchemaHash()) + ", got "
        + protaco::detail::Hex32(alt::Recipe::MsgSchemaHash()), error.what);
}

// Adding versions leaves the fingerprints of older versions unchanged: a frame from an old build passes.
TEST(Protaco, WindowKeepsOldFingerprints)
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
    order.get_mutable_tags().push_back("x");
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
    std::vector<std::string> extKeys = reader1.get_ext().Keys();   // open message: newer variable fields kept in ext
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
    protaco::Error error;
    EXPECT_FALSE(Decode(bytes, &latest, &error));
    EXPECT_EQ("message version too old: Order reads version 3 or newer, got 1. Use the v1 type", error.what);

    const build3::v1::Order got = protaco::ToObject<build3::v1::Order>(bytes);
    EXPECT_EQ(4, got.get_qty());
    EXPECT_EQ("n", got.get_note());
    EXPECT_EQ("Order{id=\"B2\", qty=4, note=\"n\", color=\"blue\"}", got.ToString());

    build2::Order sent2;
    sent2.set_color("c");
    sent2.set_weight(9.0);
    EXPECT_DOUBLE_EQ(9.0, protaco::ToObject<build3::v2::Order>(sent2.Serialize()).get_weight());
}

// A sender whose window no longer starts at the reader's version: rejected with both ranges.
TEST(Protaco, SenderWindowAboveReader)
{
    build4::Order sent;
    sent.set_id("C3");
    const std::vector<uint8_t> bytes = sent.Serialize();
    build1::Order old;
    protaco::Error error;
    EXPECT_FALSE(Decode(bytes, &old, &error));
    EXPECT_EQ("message version too new: Order reads version 1, sender supports 2..4", error.what);

    // Version 2 is still served, but color, required there, was removed in 4.
    build2::Order reader2;
    EXPECT_FALSE(Decode(bytes, &reader2, &error));
    EXPECT_EQ("color", error.path);
    EXPECT_EQ("missing required (frame version 4 read as 2)", error.what);

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
    protaco::Error error;
    EXPECT_FALSE(Decode(WithExtraTlv(got.Serialize(), "text", 1), &same, &error));
    EXPECT_EQ("unknown key (allow_extension false)", error.what);
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
    protaco::Error error;
    EXPECT_FALSE(Decode(WithExtraTlv(MakeHeartbeat().Serialize(), "abc", 7), &got, &error));
    EXPECT_EQ("abc", error.path);
    EXPECT_EQ("unknown key (allow_extension false)", error.what);

    // An open message keeps the same key in ext.
    TransferRequest open;
    ASSERT_TRUE(Decode(WithExtraTlv(MakeTransferRequest().Serialize(), "abc", 7), &open, &error));
    int32_t value = 0;
    EXPECT_TRUE(open.get_ext().GetInt("abc", &value));
    EXPECT_EQ(7, value);
}

// The following tests patch the schema hash to exercise the body decoder on a foreign body.
TEST(Protaco, MissingRequiredFails)
{
    partial::TransferRequest sender;
    sender.set_tm("TM1");
    const std::vector<uint8_t> bytes = WithSchemaHash(sender.Serialize(), TransferRequest::MsgSchemaHash());

    TransferRequest got;
    protaco::Error error;
    EXPECT_FALSE(Decode(bytes, &got, &error));
    EXPECT_EQ("moves", error.path);
    EXPECT_EQ("missing required", error.what);
    EXPECT_THROW(protaco::ToObject<TransferRequest>(bytes), protaco::CParseError);
}

TEST(Protaco, NestedErrorHasPath)
{
    partial::PizzaOrder sender;
    sender.get_mutable_toppings().push_back(partial::Topping());
    sender.get_mutable_delivery().get_mutable_doorCode().push_back("x");   // the receiver expects list<int>

    PizzaOrder got;
    protaco::Error error;
    EXPECT_FALSE(Decode(WithSchemaHash(sender.Serialize(), PizzaOrder::MsgSchemaHash()), &got, &error));
    EXPECT_EQ("delivery.doorCode", error.path);
    EXPECT_EQ("list element type mismatch", error.what);
}

TEST(Protaco, FixedBlockSizeMismatchFails)
{
    v1::Recipe sender;
    sender.set_path("/p");
    std::vector<uint8_t> bytes = sender.Serialize();
    bytes[VersionAt(bytes)] = 2;       // version 1 -> 2
    bytes[VersionAt(bytes) + 2] = 2;   // window 2..2, still one fingerprint
    bytes = WithSchemaHash(bytes, v2::Recipe::MsgSchemaHash());

    v2::Recipe got;
    protaco::Error error;
    EXPECT_FALSE(Decode(bytes, &got, &error));
    EXPECT_EQ("fixed block size mismatch: expected 28, got 20", error.what);
}

// 8 nested levels is the limit (generator PC406, runtime kMaxDepth).
TEST(Protaco, EightNestedLevelsRoundTrip)
{
    deep::D8 sent;
    sent.get_mutable_d().get_mutable_d().get_mutable_d().get_mutable_d().get_mutable_d().get_mutable_d().get_mutable_d().get_mutable_d().set_v(42);
    const deep::D8 got = protaco::ToObject<deep::D8>(sent.Serialize());
    EXPECT_EQ(42, got.get_d().get_d().get_d().get_d().get_d().get_d().get_d().get_d().get_v());
    EXPECT_EQ("D8{d=D7{d=D6{d=D5{d=D4{d=D3{d=D2{d=D1{d=D0{v=42}}}}}}}}}", got.ToString());
}

TEST(Protaco, ExtRejectsSchemaAndReservedKeys)
{
    TransferRequest request;
    EXPECT_FALSE(request.get_mutable_ext().SetInt("tm", 1));   // schema key
    EXPECT_FALSE(request.get_mutable_ext().SetInt("_x", 1));   // reserved
    EXPECT_TRUE(request.get_mutable_ext().SetString("lotType", "A"));
    std::string value;
    EXPECT_TRUE(request.get_ext().GetString("lotType", &value));
    EXPECT_EQ("A", value);
    int32_t wrongType = 0;
    EXPECT_FALSE(request.get_ext().GetInt("lotType", &wrongType));
}

TEST(Protaco, PeekHeaderReadsEnvelopeWithoutBody)
{
    const TransferRequest request = MakeTransferRequest();
    protaco::Header header;
    ASSERT_TRUE(protaco::PeekHeader(request.Serialize().data(), request.Serialize().size(), &header));
    EXPECT_EQ("TransferRequest", header.name);
    EXPECT_EQ(2, header.version);
    EXPECT_EQ(2, header.minVersion);
    EXPECT_EQ(TransferRequest::MsgSchemaHash(), header.schemaHash);
    EXPECT_EQ(7u, header.env.id);

    const std::vector<uint8_t> window = MakeOrderV3().Serialize();
    ASSERT_TRUE(protaco::PeekHeader(window.data(), window.size(), &header));
    EXPECT_EQ(3, header.version);
    EXPECT_EQ(1, header.minVersion);
    EXPECT_EQ(build3::Order::MsgSchemaHash(), header.schemaHash);
}

// ToString: same text as C# (tests/cs/Program.cs ToStringMatchesCpp).
TEST(Protaco, ToStringCompact)
{
    EXPECT_EQ("TransferRequest{tm=\"TM1\", moves=[MoveInfo{from=\"LP1\", to=\"PM1\", slot=3}, MoveInfo{from=\"PM1\", to=\"LP1\", slot=3}], "
              "reason=\"rule-1\", ext{priority=5}}",
        MakeTransferRequest().ToString());
    EXPECT_EQ("Heartbeat{node=\"oven-1\", seq=123456789, healthy=true, cpuLoad=0.25, uptimeSec=3600}", MakeHeartbeat().ToString());
    EXPECT_EQ("DeliveryProof{orderId=\"A-1001\", photo=<11 bytes: FF D8 00 10 4A 46 49 46 ...>, "
              "signatures=[<1 bytes: 00>, <0 bytes>, <2 bytes: 01 02>]}",
        MakeDeliveryProof().ToString());
    EXPECT_EQ("Recipe{name=\"\", temp=25.5, step=3, path=\"\", note=\"none\"}", v2::Recipe().ToString());
}

TEST(Protaco, ToStringPretty)
{
    EXPECT_EQ("TransferRequest{\n"
              "  tm=\"TM1\",\n"
              "  moves=[\n"
              "    MoveInfo{\n"
              "      from=\"LP1\",\n"
              "      to=\"PM1\",\n"
              "      slot=3\n"
              "    },\n"
              "    MoveInfo{\n"
              "      from=\"PM1\",\n"
              "      to=\"LP1\",\n"
              "      slot=3\n"
              "    }\n"
              "  ],\n"
              "  reason=\"rule-1\",\n"
              "  ext{\n"
              "    priority=5\n"
              "  }\n"
              "}",
        MakeTransferRequest().ToString(protaco::TextStyle::Pretty()));
}

// ext entries of every kind, custom brackets and separator, escaping and cut.
TEST(Protaco, ToStringExtStyleAndEscape)
{
    MoveInfo move = MakeMove("a\"b\\c\n", "PM1", 3);
    protaco::TextStyle style;
    style.separator = "; ";
    style.open = "(";
    style.close = ")";
    style.maxText = 4;
    EXPECT_EQ("MoveInfo(from=\"a\\\"b\\\\\"...(+2); to=\"PM1\"; slot=3)", move.ToString(style));

    TransferRequest request;
    request.set_tm("TM1");
    protaco::CExt& ext = request.get_mutable_ext();
    ext.SetInt("priority", 5);
    ext.SetString("lot", "A");
    ext.SetBool("hot", true);
    ext.SetDouble("ratio", 0.5);
    ext.SetInt64("big", 1234567890123);
    ext.SetBytes("blob", {0x00, 0xFF});
    EXPECT_EQ("TransferRequest{tm=\"TM1\", moves=[], reason=\"\", "
              "ext{priority=5, lot=\"A\", hot=true, ratio=0.5, big=1234567890123, blob=<2 bytes: 00 FF>}}",
        request.ToString());
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
            EXPECT_TRUE(protaco::TryToObject(got.Serialize().data(), got.Serialize().size(), &got));
    }
}

// bytes: binary values survive a round trip, in fields, lists and ext.
TEST(Protaco, RoundTripBytes)
{
    DeliveryProof sent = MakeDeliveryProof();
    const std::vector<uint8_t> blob = {0x00, 0x00, 0xFF};
    ASSERT_TRUE(sent.get_mutable_ext().SetBytes("thumbnail", blob));
    const DeliveryProof got = protaco::ToObject<DeliveryProof>(sent.Serialize());
    EXPECT_EQ(sent.get_photo(), got.get_photo());
    EXPECT_EQ(sent.get_signatures(), got.get_signatures());
    EXPECT_TRUE(got.get_signatures()[1].empty());
    std::vector<uint8_t> thumbnail;
    ASSERT_TRUE(got.get_ext().GetBytes("thumbnail", &thumbnail));
    EXPECT_EQ(blob, thumbnail);
    std::string asString;
    EXPECT_FALSE(got.get_ext().GetString("thumbnail", &asString));
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
    ExpectGolden("TransferRequest.bin", MakeTransferRequest().Serialize());
    ExpectGolden("PizzaOrder.bin", MakePizzaOrder().Serialize());
    ExpectGolden("Heartbeat.bin", MakeHeartbeat().Serialize());
    ExpectGolden("DeliveryProof.bin", MakeDeliveryProof().Serialize());
}
