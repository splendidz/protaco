// Regress.cs. Regression check of the C# runtime: the cases of regress.cpp, built once against an older commit and
// once against the working tree (regress.cmd). Each case prints "name ns", the median of the per-round ns per call.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;

internal static class Regress
{
    private const int Warmup = 10000;
    private const int Rounds = 20;
    private const int Calls = 20000;
    private static long sink_;

    private static double Measure(Action<int> call)
    {
        for (int i = 0; i < Warmup; ++i)
            call(i);
        List<double> rounds = new List<double>();
        for (int round = 0; round < Rounds; ++round)
        {
            Stopwatch watch = Stopwatch.StartNew();
            for (int i = 0; i < Calls; ++i)
                call(i);
            rounds.Add(watch.ElapsedTicks * (1e9 / Stopwatch.Frequency) / Calls);
        }
        rounds.Sort();
        return rounds[Rounds / 2];
    }

    private static void Print(string name, double ns)
    {
        Console.WriteLine(name + " " + ns.ToString("F2", CultureInfo.InvariantCulture));
    }

    private static demo.DeliveryRoute MakeRoute(int v)
    {
        demo.DeliveryRoute route = new demo.DeliveryRoute { Rider = "rider-7", Note = "closest-first" };
        route.Stops.Add(new demo.RouteStop { OrderId = "A-100", Address = "1 Pizza St", EtaMin = v & 63 });
        route.Stops.Add(new demo.RouteStop { OrderId = "A-100", Address = "1 Pizza St", EtaMin = v & 63 });
        return route;
    }

    private static int Main()
    {
        bench.Fixed512 fixedMessage = new bench.Fixed512 { Name = "sensor-node-0001", Tag = "site=north;zone=a;rack=12;unit=03;rev=07" };
        Print("Fixed512.encode", Measure(i => { fixedMessage.I0 = i; sink_ += fixedMessage.Serialize().Length; }));
        byte[] fixedBytes = fixedMessage.Serialize();
        Print("Fixed512.decode", Measure(i => { sink_ += Protaco.Codec.ToObject<bench.Fixed512>(fixedBytes).I0; }));
        Print("Fixed512.view", Measure(i => { bench.Fixed512.View view; bench.Fixed512.TryView(fixedBytes, out view); sink_ += view.I0; }));

        bench.Mixed256 mixed = new bench.Mixed256 { Name = "sensor-node-0001-line-07", Note = "calibrated today", Code = "QC-PASS-2026" };
        Print("Mixed256.encode", Measure(i => { mixed.I0 = i; sink_ += mixed.Serialize().Length; }));
        byte[] mixedBytes = mixed.Serialize();
        Print("Mixed256.decode", Measure(i => { sink_ += Protaco.Codec.ToObject<bench.Mixed256>(mixedBytes).I0; }));

        Print("DeliveryRoute.new+serialize", Measure(i => { sink_ += MakeRoute(i).Serialize().Length; }));
        byte[] routeBytes = MakeRoute(1).Serialize();
        Print("DeliveryRoute.decode", Measure(i => { sink_ += Protaco.Codec.ToObject<demo.DeliveryRoute>(routeBytes).Stops.Count; }));
        demo.DeliveryRoute route = Protaco.Codec.ToObject<demo.DeliveryRoute>(routeBytes);
        Print("DeliveryRoute.tostring", Measure(i => { sink_ += route.ToString().Length; }));
        Print("Heartbeat.new", Measure(i => { sink_ += new demo.Heartbeat().GetId(); }));
        return sink_ == 42 ? 1 : 0;
    }
}
