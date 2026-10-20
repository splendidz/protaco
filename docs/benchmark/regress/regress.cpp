// regress.cpp. Regression check of the C++ runtime: the same protaco-only cases, built once against an older commit
// and once against the working tree (regress.cmd). Each case prints "name ns", the median of the per-round ns per call.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "bench/Fixed512.g.h"
#include "bench/Mixed256.g.h"
#include "demo/DeliveryRoute.g.h"
#include "demo/Heartbeat.g.h"

namespace
{

const std::string kName = "sensor-node-0001";
const std::string kLongName = "sensor-node-0001-line-07";
const std::string kTag = "site=north;zone=a;rack=12;unit=03;rev=07";
const std::string kNote = "calibrated today";
const std::string kCode = "QC-PASS-2026";

const int kWarmup = 10000;
const int kRounds = 20;
const int kCalls = 100000;

volatile int g_seed = 100000;   // read at run time, so values are not constants
volatile size_t g_sink = 0;     // results go here, so calls are not optimized away

template <class F>
double Measure(F call)
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
    std::sort(rounds.begin(), rounds.end());
    return rounds[kRounds / 2];
}

// Same values as bench.cpp.
void FillFixed512(bench::Fixed512& m, int v)
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

void FillMixed256(bench::Mixed256& m, int v)
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

// A new route with two stops: object creation (envelope stamp), variable fields and a list of messages.
demo::DeliveryRoute MakeRoute(int v)
{
    demo::DeliveryRoute route;
    route.set_rider("rider-7");
    demo::RouteStop stop;
    stop.set_orderId("A-100");
    stop.set_address("1 Pizza St");
    stop.set_etaMin(v & 63);
    route.set_stops(std::vector<demo::RouteStop>(2, stop));
    route.set_note("closest-first");
    return route;
}

void Print(const char* name, double ns)
{
    std::printf("%s %.2f\n", name, ns);
}

}  // namespace

int main()
{
    {
        bench::Fixed512 m;
        Print("Fixed512.encode", Measure([&](int i) { FillFixed512(m, g_seed + (i & 1023)); g_sink = g_sink + m.Serialize().size(); }));
        const std::vector<uint8_t> bytes = m.Serialize();
        bench::Fixed512 out;
        Print("Fixed512.decode", Measure([&](int) { protaco::TryToObject(bytes.data(), bytes.size(), &out); g_sink = g_sink + static_cast<size_t>(out.get_i0()); }));
        bench::Fixed512::View view;
        Print("Fixed512.view", Measure([&](int) { protaco::TryView<bench::Fixed512>(bytes, &view); g_sink = g_sink + static_cast<size_t>(view.get_i0()); }));
    }
    {
        bench::Mixed256 m;
        Print("Mixed256.encode", Measure([&](int i) { FillMixed256(m, g_seed + (i & 1023)); g_sink = g_sink + m.Serialize().size(); }));
        const std::vector<uint8_t> bytes = m.Serialize();
        bench::Mixed256 out;
        Print("Mixed256.decode", Measure([&](int) { protaco::TryToObject(bytes.data(), bytes.size(), &out); g_sink = g_sink + static_cast<size_t>(out.get_i0()); }));
    }
    {
        Print("DeliveryRoute.new+serialize", Measure([&](int i) { g_sink = g_sink + MakeRoute(g_seed + i).Serialize().size(); }));
        const std::vector<uint8_t> bytes = MakeRoute(g_seed).Serialize();
        demo::DeliveryRoute out;
        Print("DeliveryRoute.decode", Measure([&](int) { protaco::TryToObject(bytes.data(), bytes.size(), &out); g_sink = g_sink + out.get_stops().size(); }));
        Print("DeliveryRoute.tostring", Measure([&](int) { g_sink = g_sink + out.ToString().size(); }));
        Print("Heartbeat.new", Measure([&](int) { demo::Heartbeat beat; g_sink = g_sink + beat.GetId(); }));
    }
    return 0;
}
