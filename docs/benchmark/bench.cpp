// bench.cpp. protaco and Protocol Buffers on the same two messages (schemas/*.yaml, bench.proto).
// Both libraries name setters set_<field>, so one template fills either message with the same values.
// Build: protacoc -I schemas --cpp_out gen, protoc --cpp_out=gen bench.proto, then compile Release x64 and link
// libprotobuf. Each case runs a warm-up, then kRounds x kCalls calls, and prints the mean and standard deviation
// of the per-round ns per call. Lines starting with "csv," go to results.csv (plot.py draws the figure from it).
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "Fixed512.g.h"
#include "Mixed256.g.h"
#include "bench.pb.h"

namespace
{

const std::string kName = "sensor-node-0001";                          // 16 bytes
const std::string kLongName = "sensor-node-0001-line-07";              // 24 bytes
const std::string kTag = "site=north;zone=a;rack=12;unit=03;rev=07";   // 40 bytes
const std::string kNote = "calibrated today";                          // 16 bytes
const std::string kCode = "QC-PASS-2026";                              // 12 bytes

const int kWarmup = 10000;
const int kRounds = 20;
const int kCalls = 100000;

volatile int g_seed = 100000;   // read at run time, so values are not constants
volatile size_t g_sink = 0;     // results go here, so calls are not optimized away

struct Timing
{
    double mean;   // ns per call, mean of the rounds
    double sd;     // standard deviation of the rounds
};

// ns per call over kRounds rounds of kCalls calls, after kWarmup calls.
template <class F>
Timing Measure(F call)
{
    for (int i = 0; i < kWarmup; ++i)
        call(i);
    std::vector<double> rounds;
    for (int round = 0; round < kRounds; ++round)
    {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < kCalls; ++i)
            call(i);
        rounds.push_back(std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / kCalls);
    }
    double sum = 0;
    for (double value : rounds)
        sum += value;
    const double mean = sum / kRounds;
    double squares = 0;
    for (double value : rounds)
        squares += (value - mean) * (value - mean);
    return Timing{ mean, std::sqrt(squares / (kRounds - 1)) };
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

void Print(const char* message, const char* operation, const char* library, const Timing& timing, size_t bytes)
{
    std::printf("%-9s %-7s %-17s %8.1f ns  sd %5.1f  (%zu B)\n", message, operation, library, timing.mean, timing.sd, bytes);
    std::printf("csv,%s,%s,%s,%.2f,%.2f,%zu\n", message, operation, library, timing.mean, timing.sd, bytes);
}

// Objects are reused across calls, as both libraries recommend.
// Encode: set every field, then serialize. Decode: bytes to an object, then read one field.
template <class P>
void RunProtaco(const char* message, void (*fill)(P&, int))
{
    P m;
    const Timing encode = Measure([&](int i)
    {
        fill(m, g_seed + (i & 1023));
        g_sink = g_sink + m.Serialize().size();
    });
    const std::vector<uint8_t> bytes = m.Serialize();
    P out;
    Check(protaco::TryToObject(bytes.data(), bytes.size(), &out) && out.Serialize() == bytes, P::MsgName());
    const Timing decode = Measure([&](int)
    {
        protaco::TryToObject(bytes.data(), bytes.size(), &out);
        g_sink = g_sink + static_cast<size_t>(out.get_i0());
    });
    Print(message, "encode", "protaco", encode, bytes.size());
    Print(message, "decode", "protaco", decode, bytes.size());
}

// Fixed-only messages: TryView checks the frame like TryToObject and reads fields in place.
template <class P>
void RunProtacoView(const char* message)
{
    P m;
    FillFixed512(m, g_seed);
    const std::vector<uint8_t> bytes = m.Serialize();
    typename P::View view;
    Check(protaco::TryView<P>(bytes, &view) && view.get_i0() == m.get_i0(), P::MsgName());
    const Timing decode = Measure([&](int)
    {
        protaco::TryView<P>(bytes, &view);
        g_sink = g_sink + static_cast<size_t>(view.get_i0());
    });
    Print(message, "decode", "protaco TryView", decode, bytes.size());
}

template <class B>
void RunProtobuf(const char* message, void (*fill)(B&, int))
{
    B m;
    std::vector<uint8_t> buffer(4096);
    const Timing encode = Measure([&](int i)
    {
        fill(m, g_seed + (i & 1023));
        m.SerializeToArray(buffer.data(), static_cast<int>(buffer.size()));
        g_sink = g_sink + static_cast<size_t>(m.GetCachedSize());
    });
    std::string bytes;
    m.SerializeToString(&bytes);
    B out;
    Check(out.ParseFromArray(bytes.data(), static_cast<int>(bytes.size())) && out.SerializeAsString() == bytes, message);
    const Timing decode = Measure([&](int)
    {
        out.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()));
        g_sink = g_sink + static_cast<size_t>(out.i0());
    });
    Print(message, "encode", "protobuf", encode, bytes.size());
    Print(message, "decode", "protobuf", decode, bytes.size());
}

}  // namespace

int main()
{
    std::printf("%d rounds x %d calls after %d warm-up calls, ns per call\n", kRounds, kCalls, kWarmup);
    RunProtaco("Fixed512", &FillFixed512<bench::Fixed512>);
    RunProtacoView<bench::Fixed512>("Fixed512");
    RunProtobuf("Fixed512", &FillFixed512<bench::pb::Fixed512>);
    RunProtaco("Mixed256", &FillMixed256<bench::Mixed256>);
    RunProtobuf("Mixed256", &FillMixed256<bench::pb::Mixed256>);
    return 0;
}
