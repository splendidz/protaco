// bench.cpp. protaco and Protocol Buffers on the same three messages (schemas/*.yaml, bench.proto).
// Both libraries name setters set_<field>, so one template fills either message with the same values.
// Build: protacoc -I schemas --cpp_out gen, protoc --cpp_out=gen bench.proto, then compile Release x64 and link
// libprotobuf. Each case runs a warm-up, then kRounds x kCalls calls, and prints the mean ns per call.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "Fixed1K.g.h"
#include "Fixed512.g.h"
#include "Mixed256.g.h"
#include "bench.pb.h"

namespace
{

const std::string kName = "sensor-node-0001";                                                     // 16 bytes
const std::string kLongName = "sensor-node-0001-line-07";                                         // 24 bytes
const std::string kTag = "site=north;zone=a;rack=12;unit=03;rev=07";                              // 40 bytes
const std::string kTag1K = "site=north;zone=a;rack=12;unit=03;rev=07;owner=ops;tier=gold;shift=night;ok=1";  // 77 bytes
const std::string kNote = "calibrated today";                                                     // 16 bytes
const std::string kCode = "QC-PASS-2026";                                                         // 12 bytes

const int kWarmup = 10000;
const int kRounds = 20;
const int kCalls = 100000;

volatile int g_seed = 100000;   // read at run time, so values are not constants
volatile size_t g_sink = 0;     // results go here, so calls are not optimized away

// Mean ns per call over kRounds x kCalls calls, after kWarmup calls.
template <class F>
double MeanNs(F call)
{
    for (int i = 0; i < kWarmup; ++i)
        call(i);
    double total = 0;
    for (int round = 0; round < kRounds; ++round)
    {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < kCalls; ++i)
            call(i);
        total += std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count();
    }
    return total / (static_cast<double>(kRounds) * kCalls);
}

template <class M>
void FillFixed512(M& m, int v)
{
    m.set_name(kName); m.set_id(v * 1000003LL); m.set_seq(v * 1000003LL); m.set_ts(v * 1000003LL);
    m.set_flags(v * 1000003LL); m.set_i0(v + 0); m.set_i1(v + 1); m.set_i2(v + 2); m.set_i3(v + 3); m.set_i4(v + 4);
    m.set_i5(v + 5); m.set_i6(v + 6); m.set_i7(v + 7); m.set_i8(v + 8); m.set_i9(v + 9); m.set_i10(v + 10);
    m.set_i11(v + 11); m.set_i12(v + 12); m.set_i13(v + 13); m.set_i14(v + 14); m.set_i15(v + 15);
    m.set_i16(v + 16); m.set_i17(v + 17); m.set_i18(v + 18); m.set_i19(v + 19); m.set_i20(v + 20);
    m.set_i21(v + 21); m.set_i22(v + 22); m.set_i23(v + 23); m.set_i24(v + 24); m.set_i25(v + 25);
    m.set_i26(v + 26); m.set_i27(v + 27); m.set_i28(v + 28); m.set_i29(v + 29); m.set_i30(v + 30);
    m.set_i31(v + 31); m.set_d0(v * 0.25 + 0); m.set_d1(v * 0.25 + 1); m.set_d2(v * 0.25 + 2);
    m.set_d3(v * 0.25 + 3); m.set_d4(v * 0.25 + 4); m.set_d5(v * 0.25 + 5); m.set_d6(v * 0.25 + 6);
    m.set_d7(v * 0.25 + 7); m.set_d8(v * 0.25 + 8); m.set_d9(v * 0.25 + 9); m.set_d10(v * 0.25 + 10);
    m.set_d11(v * 0.25 + 11); m.set_d12(v * 0.25 + 12); m.set_d13(v * 0.25 + 13); m.set_d14(v * 0.25 + 14);
    m.set_d15(v * 0.25 + 15); m.set_d16(v * 0.25 + 16); m.set_d17(v * 0.25 + 17); m.set_d18(v * 0.25 + 18);
    m.set_d19(v * 0.25 + 19); m.set_d20(v * 0.25 + 20); m.set_d21(v * 0.25 + 21); m.set_d22(v * 0.25 + 22);
    m.set_d23(v * 0.25 + 23); m.set_d24(v * 0.25 + 24); m.set_d25(v * 0.25 + 25); m.set_d26(v * 0.25 + 26);
    m.set_d27(v * 0.25 + 27); m.set_d28(v * 0.25 + 28); m.set_d29(v * 0.25 + 29); m.set_d30(v * 0.25 + 30);
    m.set_d31(v * 0.25 + 31); m.set_b0(((v + 0) & 1) != 0); m.set_b1(((v + 1) & 1) != 0);
    m.set_b2(((v + 2) & 1) != 0); m.set_b3(((v + 3) & 1) != 0); m.set_b4(((v + 4) & 1) != 0);
    m.set_b5(((v + 5) & 1) != 0); m.set_b6(((v + 6) & 1) != 0); m.set_b7(((v + 7) & 1) != 0); m.set_tag(kTag);
}

template <class M>
void FillFixed1K(M& m, int v)
{
    m.set_name(kName); m.set_id(v * 1000003LL); m.set_seq(v * 1000003LL); m.set_ts(v * 1000003LL);
    m.set_flags(v * 1000003LL); m.set_total(v * 1000003LL); m.set_count(v * 1000003LL); m.set_start(v * 1000003LL);
    m.set_stop(v * 1000003LL); m.set_i0(v + 0); m.set_i1(v + 1); m.set_i2(v + 2); m.set_i3(v + 3); m.set_i4(v + 4);
    m.set_i5(v + 5); m.set_i6(v + 6); m.set_i7(v + 7); m.set_i8(v + 8); m.set_i9(v + 9); m.set_i10(v + 10);
    m.set_i11(v + 11); m.set_i12(v + 12); m.set_i13(v + 13); m.set_i14(v + 14); m.set_i15(v + 15);
    m.set_i16(v + 16); m.set_i17(v + 17); m.set_i18(v + 18); m.set_i19(v + 19); m.set_i20(v + 20);
    m.set_i21(v + 21); m.set_i22(v + 22); m.set_i23(v + 23); m.set_i24(v + 24); m.set_i25(v + 25);
    m.set_i26(v + 26); m.set_i27(v + 27); m.set_i28(v + 28); m.set_i29(v + 29); m.set_i30(v + 30);
    m.set_i31(v + 31); m.set_i32(v + 32); m.set_i33(v + 33); m.set_i34(v + 34); m.set_i35(v + 35);
    m.set_i36(v + 36); m.set_i37(v + 37); m.set_i38(v + 38); m.set_i39(v + 39); m.set_i40(v + 40);
    m.set_i41(v + 41); m.set_i42(v + 42); m.set_i43(v + 43); m.set_i44(v + 44); m.set_i45(v + 45);
    m.set_i46(v + 46); m.set_i47(v + 47); m.set_i48(v + 48); m.set_i49(v + 49); m.set_i50(v + 50);
    m.set_i51(v + 51); m.set_i52(v + 52); m.set_i53(v + 53); m.set_i54(v + 54); m.set_i55(v + 55);
    m.set_i56(v + 56); m.set_i57(v + 57); m.set_i58(v + 58); m.set_i59(v + 59); m.set_i60(v + 60);
    m.set_i61(v + 61); m.set_i62(v + 62); m.set_i63(v + 63); m.set_d0(v * 0.25 + 0); m.set_d1(v * 0.25 + 1);
    m.set_d2(v * 0.25 + 2); m.set_d3(v * 0.25 + 3); m.set_d4(v * 0.25 + 4); m.set_d5(v * 0.25 + 5);
    m.set_d6(v * 0.25 + 6); m.set_d7(v * 0.25 + 7); m.set_d8(v * 0.25 + 8); m.set_d9(v * 0.25 + 9);
    m.set_d10(v * 0.25 + 10); m.set_d11(v * 0.25 + 11); m.set_d12(v * 0.25 + 12); m.set_d13(v * 0.25 + 13);
    m.set_d14(v * 0.25 + 14); m.set_d15(v * 0.25 + 15); m.set_d16(v * 0.25 + 16); m.set_d17(v * 0.25 + 17);
    m.set_d18(v * 0.25 + 18); m.set_d19(v * 0.25 + 19); m.set_d20(v * 0.25 + 20); m.set_d21(v * 0.25 + 21);
    m.set_d22(v * 0.25 + 22); m.set_d23(v * 0.25 + 23); m.set_d24(v * 0.25 + 24); m.set_d25(v * 0.25 + 25);
    m.set_d26(v * 0.25 + 26); m.set_d27(v * 0.25 + 27); m.set_d28(v * 0.25 + 28); m.set_d29(v * 0.25 + 29);
    m.set_d30(v * 0.25 + 30); m.set_d31(v * 0.25 + 31); m.set_d32(v * 0.25 + 32); m.set_d33(v * 0.25 + 33);
    m.set_d34(v * 0.25 + 34); m.set_d35(v * 0.25 + 35); m.set_d36(v * 0.25 + 36); m.set_d37(v * 0.25 + 37);
    m.set_d38(v * 0.25 + 38); m.set_d39(v * 0.25 + 39); m.set_d40(v * 0.25 + 40); m.set_d41(v * 0.25 + 41);
    m.set_d42(v * 0.25 + 42); m.set_d43(v * 0.25 + 43); m.set_d44(v * 0.25 + 44); m.set_d45(v * 0.25 + 45);
    m.set_d46(v * 0.25 + 46); m.set_d47(v * 0.25 + 47); m.set_d48(v * 0.25 + 48); m.set_d49(v * 0.25 + 49);
    m.set_d50(v * 0.25 + 50); m.set_d51(v * 0.25 + 51); m.set_d52(v * 0.25 + 52); m.set_d53(v * 0.25 + 53);
    m.set_d54(v * 0.25 + 54); m.set_d55(v * 0.25 + 55); m.set_d56(v * 0.25 + 56); m.set_d57(v * 0.25 + 57);
    m.set_d58(v * 0.25 + 58); m.set_d59(v * 0.25 + 59); m.set_d60(v * 0.25 + 60); m.set_d61(v * 0.25 + 61);
    m.set_d62(v * 0.25 + 62); m.set_d63(v * 0.25 + 63); m.set_b0(((v + 0) & 1) != 0); m.set_b1(((v + 1) & 1) != 0);
    m.set_b2(((v + 2) & 1) != 0); m.set_b3(((v + 3) & 1) != 0); m.set_b4(((v + 4) & 1) != 0);
    m.set_b5(((v + 5) & 1) != 0); m.set_b6(((v + 6) & 1) != 0); m.set_b7(((v + 7) & 1) != 0);
    m.set_b8(((v + 8) & 1) != 0); m.set_b9(((v + 9) & 1) != 0); m.set_b10(((v + 10) & 1) != 0);
    m.set_b11(((v + 11) & 1) != 0); m.set_b12(((v + 12) & 1) != 0); m.set_b13(((v + 13) & 1) != 0);
    m.set_b14(((v + 14) & 1) != 0); m.set_b15(((v + 15) & 1) != 0); m.set_tag(kTag1K);
}

template <class M>
void FillMixed256(M& m, int v)
{
    m.set_id(v * 1000003LL); m.set_ts(v * 1000003LL); m.set_i0(v + 0); m.set_i1(v + 1); m.set_i2(v + 2);
    m.set_i3(v + 3); m.set_i4(v + 4); m.set_i5(v + 5); m.set_i6(v + 6); m.set_i7(v + 7); m.set_i8(v + 8);
    m.set_i9(v + 9); m.set_i10(v + 10); m.set_i11(v + 11); m.set_i12(v + 12); m.set_i13(v + 13); m.set_i14(v + 14);
    m.set_i15(v + 15); m.set_name(kLongName); m.set_d0(v * 0.25 + 0); m.set_d1(v * 0.25 + 1);
    m.set_d2(v * 0.25 + 2); m.set_d3(v * 0.25 + 3); m.set_d4(v * 0.25 + 4); m.set_d5(v * 0.25 + 5);
    m.set_d6(v * 0.25 + 6); m.set_d7(v * 0.25 + 7); m.set_d8(v * 0.25 + 8); m.set_d9(v * 0.25 + 9);
    m.set_d10(v * 0.25 + 10); m.set_d11(v * 0.25 + 11); m.set_note(kNote); m.set_b0(((v + 0) & 1) != 0);
    m.set_b1(((v + 1) & 1) != 0); m.set_b2(((v + 2) & 1) != 0); m.set_b3(((v + 3) & 1) != 0); m.set_code(kCode);
}

// Decoding must give back the same message before anything is timed.
void Check(bool ok, const char* what)
{
    if (ok)
        return;
    std::printf("round trip failed: %s\n", what);
    std::exit(1);
}

struct Result
{
    size_t bytes;
    double encode;   // fill every field, then serialize
    double again;    // serialize again, nothing changed
    double decode;   // bytes to object
};

// P: protaco message, B: protobuf message. Objects are reused across calls, as both libraries recommend.
template <class P>
Result RunProtaco(void (*fill)(P&, int))
{
    Result result;
    P m;
    result.encode = MeanNs([&](int i)
    {
        fill(m, g_seed + (i & 1023));
        g_sink = g_sink + m.Serialize().size();
    });
    result.again = MeanNs([&](int)
    {
        g_sink = g_sink + m.Serialize().size();
    });
    const std::vector<uint8_t> bytes = m.Serialize();
    result.bytes = bytes.size();
    P out;
    Check(protaco::TryToObject(bytes.data(), bytes.size(), &out) && out.Serialize() == bytes, P::MsgName());
    result.decode = MeanNs([&](int)
    {
        protaco::TryToObject(bytes.data(), bytes.size(), &out);
        g_sink = g_sink + static_cast<size_t>(out.get_i0());
    });
    return result;
}

template <class B>
Result RunProtobuf(void (*fill)(B&, int))
{
    Result result;
    B m;
    std::vector<uint8_t> buffer(4096);
    result.encode = MeanNs([&](int i)
    {
        fill(m, g_seed + (i & 1023));
        m.SerializeToArray(buffer.data(), static_cast<int>(buffer.size()));
        g_sink = g_sink + static_cast<size_t>(m.GetCachedSize());
    });
    result.again = MeanNs([&](int)
    {
        m.SerializeToArray(buffer.data(), static_cast<int>(buffer.size()));
        g_sink = g_sink + static_cast<size_t>(m.GetCachedSize());
    });
    std::string bytes;
    m.SerializeToString(&bytes);
    result.bytes = bytes.size();
    B out;
    Check(out.ParseFromArray(bytes.data(), static_cast<int>(bytes.size())) && out.SerializeAsString() == bytes, B::descriptor()->name().c_str());
    result.decode = MeanNs([&](int)
    {
        out.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()));
        g_sink = g_sink + static_cast<size_t>(out.i0());
    });
    return result;
}

// Positive: protaco takes that much less time than protobuf.
double Faster(double protaco, double protobuf)
{
    return 100.0 * (protobuf - protaco) / protobuf;
}

void Print(const char* name, const Result& p, const Result& b)
{
    std::printf("%-9s protaco  %5zu B  encode %8.1f ns  again %8.1f ns  decode %8.1f ns\n", name, p.bytes, p.encode, p.again, p.decode);
    std::printf("%-9s protobuf %5zu B  encode %8.1f ns  again %8.1f ns  decode %8.1f ns\n", name, b.bytes, b.encode, b.again, b.decode);
    std::printf("%-9s protaco faster by   encode %7.1f %%   again %7.1f %%   decode %7.1f %%\n\n", name,
        Faster(p.encode, b.encode), Faster(p.again, b.again), Faster(p.decode, b.decode));
}

}  // namespace

int main()
{
    std::printf("%d rounds x %d calls after %d warm-up calls, mean ns per call\n\n", kRounds, kCalls, kWarmup);
    Print("Fixed512", RunProtaco(&FillFixed512<bench::Fixed512>), RunProtobuf(&FillFixed512<bench::pb::Fixed512>));
    Print("Fixed1K", RunProtaco(&FillFixed1K<bench::Fixed1K>), RunProtobuf(&FillFixed1K<bench::pb::Fixed1K>));
    Print("Mixed256", RunProtaco(&FillMixed256<bench::Mixed256>), RunProtobuf(&FillMixed256<bench::pb::Mixed256>));
    return 0;
}
