// Program.cs. Tests for protacoc and the C# runtime. Console runner, no test framework.
// Run: protacoc.Tests.exe <tests folder> (protaco_compiler/tests). Exit code 1 if any test fails.
// protacoc: the PROTACOC environment variable, otherwise protacoc.exe (or protacoc) next to this runner.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using demo;

namespace Protacoc.Tests
{
    internal static class Program
    {
        private static string tests_;
        private static int passed_;
        private static readonly List<string> failures_ = new List<string>();

        private static int Main(string[] args)
        {
            tests_ = Path.GetFullPath(args.Length > 0 ? args[0] : ".");
            Run("generator: expected error code per bad yaml", GeneratorErrors);
            Run("generator: reference cycle PC403", GeneratorCycle);
            Run("generator: two namespaces in one run PC405", GeneratorNamespaceMix);
            Run("generator: nesting deeper than 8 PC406", GeneratorTooDeep);
            Run("generator: versions on a nested message PC407", GeneratorNestedVersion);
            Run("generator: --namespace option removed", GeneratorNamespaceOption);
            Run("generator: committed demo output is up to date (--check)", GeneratorCheckUpToDate);
            Run("generator: hand edits detected (--check)", GeneratorCheckDetectsEdit);
            Run("generator: input folder order does not matter", GeneratorOrderIndependent);
            Run("generator: fingerprint is FNV-1a of the documented canonical text", FingerprintCanonicalText);
            Run("C#: round trip", RoundTrip);
            Run("C#: same bytes as the C++ golden files", GoldenEqualsCpp);
            Run("C#: reads the C++ golden files", ReadsCppGolden);
            Run("C#: other version is rejected", VersionMismatch);
            Run("C#: same version, other schema is rejected", SchemaMismatch);
            Run("C#: new version keeps old fingerprints", NewVersionKeepsOldFingerprints);
            Run("C#: newer frame read as own version", NewerFrameReadAsOwnVersion);
            Run("C#: older frame needs the old type", OlderFrameNeedsOldType);
            Run("C#: sender min_version above the reader", SenderMinVersionAboveReader);
            Run("C#: closed message drops newer fields", ClosedMessageDropsNewerFields);
            Run("C#: closed message rejects an unknown key", ClosedMessage);
            Run("C#: missing required field and nested error path", Errors);
            Run("C#: 8 nested levels round trip", DeepNesting);
            Run("C#: every truncation fails", Truncation);
            Run("C#: fixed-length string overflow is rejected", FixedStringOverflow);
            Run("C#: ext key rules", ExtRules);
            Run("C#: ToString matches C++", ToStringMatchesCpp);
            Run("C#: unsigned types and UTF-8 match C++", UnsignedTypesAndUtf8);
            Run("C#: envelope stamped on creation, not on decode", EnvelopeIsStampedOnCreation);
            Run("C#: enum fields match C++", EnumFields);
            Run("generator: enum default must be a value PC308", GeneratorEnumDefault);
            Run("C#: timestamp to local time, same text as C++", TimestampConvertsToLocalTime);
            Console.WriteLine();
            Console.WriteLine("passed " + passed_ + " / failed " + failures_.Count);
            foreach (string failure in failures_)
                Console.WriteLine("  FAIL " + failure);
            return failures_.Count == 0 ? 0 : 1;
        }

        private static void Run(string name, Action test)
        {
            try
            {
                test();
                ++passed_;
                Console.WriteLine("[ OK ] " + name);
            }
            catch (Exception e)
            {
                failures_.Add(name + ": " + e.Message);
                Console.WriteLine("[FAIL] " + name + ": " + e.Message);
            }
        }

        private static void Check(bool condition, string message)
        {
            if (!condition)
                throw new Exception(message);
        }

        private static void Equal<T>(T expected, T actual, string what)
        {
            if (!EqualityComparer<T>.Default.Equals(expected, actual))
                throw new Exception(what + ": expected " + expected + ", got " + actual);
        }

        // ---- generator ----

        private sealed class GenResult
        {
            public int Exit;
            public string Output;
        }

        private static GenResult Protacoc(params string[] args)
        {
            ProcessStartInfo info = new ProcessStartInfo(ProtacocPath())
            {
                Arguments = string.Join(" ", args.Select(a => "\"" + a + "\"")),
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.Default,
                StandardErrorEncoding = Encoding.Default,
            };
            using (Process process = Process.Start(info))
            {
                string output = process.StandardOutput.ReadToEnd() + process.StandardError.ReadToEnd();
                process.WaitForExit();
                return new GenResult { Exit = process.ExitCode, Output = output };
            }
        }

        private static string ProtacocPath()
        {
            string fromEnv = Environment.GetEnvironmentVariable("PROTACOC");
            if (!string.IsNullOrEmpty(fromEnv))
                return fromEnv;
            string dir = AppDomain.CurrentDomain.BaseDirectory;
            string exe = Path.Combine(dir, "protacoc.exe");
            return File.Exists(exe) ? exe : Path.Combine(dir, "protacoc");
        }

        private static string TempDir()
        {
            string dir = Path.Combine(Path.GetTempPath(), "protacoc_test_" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(dir);
            return dir;
        }

        private static string Demo()
        {
            return TestSchemas("demo");
        }

        private static string TestSchemas(string name)
        {
            return Path.Combine(tests_, "schemas", name);
        }

        private static void GeneratorErrors()
        {
            string[] files = Directory.GetFiles(TestSchemas("errors"), "*.yaml");
            Check(files.Length >= 20, "too few error cases: " + files.Length);
            foreach (string file in files)
            {
                string expected = File.ReadLines(file).First().Replace("# expect:", "").Trim();
                string dir = TempDir();
                File.Copy(file, Path.Combine(dir, Path.GetFileName(file)));
                GenResult result = Protacoc("-I", dir, "--cpp_out", Path.Combine(dir, "out"));
                Equal(2, result.Exit, Path.GetFileName(file) + " exit code");
                Check(result.Output.Contains("error " + expected + ":"), Path.GetFileName(file) + ": no " + expected + ". Output: " + result.Output);
            }
        }

        private static void GeneratorCycle()
        {
            GenResult result = Protacoc("-I", TestSchemas("cycle"), "--cpp_out", Path.Combine(TempDir(), "out"));
            Equal(2, result.Exit, "exit code");
            Check(result.Output.Contains("error PC403:") && result.Output.Contains("NodeA -> NodeB -> NodeA"), "no cycle path: " + result.Output);
        }

        private static void GeneratorNamespaceMix()
        {
            GenResult result = Protacoc("-I", TestSchemas("nsmix"), "--cpp_out", Path.Combine(TempDir(), "out"));
            Equal(2, result.Exit, "exit code");
            Check(result.Output.Contains("error PC405:") && result.Output.Contains("B.yaml"), "no PC405 on B.yaml: " + result.Output);
        }

        private static void GeneratorTooDeep()
        {
            GenResult result = Protacoc("-I", TestSchemas("toodeep"), "--cpp_out", Path.Combine(TempDir(), "out"));
            Equal(2, result.Exit, "exit code");
            Check(result.Output.Contains("error PC406:") && result.Output.Contains("'T9'"), "no PC406 on T9: " + result.Output);
            Check(!result.Output.Contains("'T8'"), "T8 (8 levels) rejected: " + result.Output);
        }

        private static void GeneratorNestedVersion()
        {
            GenResult result = Protacoc("-I", TestSchemas("nestedwin"), "--cpp_out", Path.Combine(TempDir(), "out"));
            Equal(2, result.Exit, "exit code");
            Check(result.Output.Contains("error PC407:") && result.Output.Contains("'Inner'") && result.Output.Contains("Outer.inner"), "no PC407 on Inner: " + result.Output);
        }

        private static void GeneratorNamespaceOption()
        {
            GenResult result = Protacoc("-I", Demo(), "--cpp_out", Path.Combine(TempDir(), "out"), "--namespace", "x");
            Equal(1, result.Exit, "exit code (usage)");
            Check(result.Output.Contains("--namespace was removed"), "no message: " + result.Output);
        }

        private static void GeneratorCheckUpToDate()
        {
            GenResult result = Protacoc("-I", Demo(), "--no_runtime",
                "--cpp_out", Path.Combine(tests_, "gen", "cpp", "demo"), "--cs_out", Path.Combine(tests_, "gen", "cs", "demo"), "--check");
            Equal(0, result.Exit, "exit code (" + result.Output.Trim() + ")");
        }

        private static void GeneratorCheckDetectsEdit()
        {
            string dir = TempDir();
            Equal(0, Protacoc("-I", Demo(), "--cpp_out", dir).Exit, "generate");
            File.AppendAllText(Path.Combine(dir, "RouteStop.g.h"), "// hand edit\r\n");
            GenResult result = Protacoc("-I", Demo(), "--cpp_out", dir, "--check");
            Equal(3, result.Exit, "exit code");
            Check(result.Output.Contains("RouteStop.g.h"), "edited file not reported: " + result.Output);
        }

        // Splitting the same yaml into two folders and swapping their order gives identical output.
        private static void GeneratorOrderIndependent()
        {
            string a = TempDir();
            string b = TempDir();
            foreach (string file in Directory.GetFiles(Demo(), "*.yaml"))
            {
                bool first = Path.GetFileName(file).StartsWith("P") || Path.GetFileName(file).StartsWith("Route");
                File.Copy(file, Path.Combine(first ? a : b, Path.GetFileName(file)));
            }
            string out1 = TempDir();
            string out2 = TempDir();
            Equal(0, Protacoc("-I", a, "-I", b, "--cpp_out", out1).Exit, "order a,b");
            Equal(0, Protacoc("-I", b, "-I", a, "--cpp_out", out2).Exit, "order b,a");
            foreach (string file in Directory.GetFiles(out1))
            {
                string other = Path.Combine(out2, Path.GetFileName(file));
                Check(File.ReadAllBytes(file).SequenceEqual(File.ReadAllBytes(other)), Path.GetFileName(file) + " differs");
            }
        }

        // README.md "Schema fingerprint". Another implementation must reach the same number from the same text.
        private static void FingerprintCanonicalText()
        {
            Equal(Fnv1a("protaco-schema-1|Topping|1|1|name:string(24);grams:int=30;"), Topping.MsgSchemaHash, "Topping");
            Equal(Fnv1a("protaco-schema-1|Heartbeat|1|0|node:string(32);seq:int64;healthy:bool=true;cpuLoad:double;uptimeSec:int;"), Heartbeat.MsgSchemaHash, "Heartbeat");
            string toppingRef = "Topping@" + Topping.MsgSchemaHash.ToString("x8");
            string deliveryRef = "Delivery@" + Delivery.MsgSchemaHash.ToString("x8");
            Equal(Fnv1a("protaco-schema-1|PizzaOrder|1|1|orderId:string(32);store:string(32);customer:string=anonymous;size:int=12;slices:int=8;"
                + "woodFired:bool=true;deepDishCm:int=0;toppings:list<" + toppingRef + ">!;notes:list<string>;delivery:" + deliveryRef + ";"), PizzaOrder.MsgSchemaHash, "PizzaOrder");
            // Serialization order (added, then declaration). Removed fixed field ~key, removed variable field left out.
            Equal(Fnv1a("protaco-schema-1|Order|3|1|id:string(8);~qty:int;color:string!;weight:double=0.5;tags:list<string>;label:string;rush:bool;"), build3.Order.MsgSchemaHash, "Order v3");
            Equal(Fnv1a("protaco-schema-1|Order|1|1|id:string(8);qty:int=1;note:string;color:string!;"), build3.v1.Order.MsgSchemaHash, "Order v1");
        }

        private static uint Fnv1a(string text)
        {
            uint hash = 2166136261u;
            foreach (byte b in Encoding.UTF8.GetBytes(text))
                hash = unchecked((hash ^ b) * 16777619u);
            return hash;
        }

        // ---- C# runtime ----

        // Same values as the C++ test (ProtacoTest.cpp MakeDeliveryRoute).
        private static DeliveryRoute MakeDeliveryRoute()
        {
            DeliveryRoute route = new DeliveryRoute { Rider = "rider-7", Note = "closest-first" };
            route.Stops.Add(new RouteStop { OrderId = "A-100", Address = "1 Pizza St", EtaMin = 10 });
            route.Stops.Add(new RouteStop { OrderId = "A-101", Address = "2 Pasta Ave", EtaMin = 25 });
            route.Ext.SetValue("priority", 5);
            route.SetEnvelope(new Protaco.Envelope { Id = 7, Corr = 0, Ts = 1000 });
            return route;
        }



        // Same values as the C++ test MakePizzaOrder.
        private static PizzaOrder MakePizzaOrder()
        {
            PizzaOrder order = new PizzaOrder
            {
                OrderId = "A-100",
                Store = "NAPOLI-1",
                Customer = "mario",
                Size = 14,
                WoodFired = true,
            };
            order.Toppings.Add(new Topping { Name = "basil", Grams = 5 });
            order.Toppings.Add(new Topping { Name = "mozzarella" });
            order.Notes.Add("no onion");
            order.Delivery.Address = "1 Pizza St";
            order.Delivery.Floor = 3;
            order.Delivery.DoorCode.AddRange(new[] { 4, 2, 4, 2 });
            order.Delivery.Contacts.Add("010-0000-0000");
            order.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return order;
        }

        private static Heartbeat MakeHeartbeat()
        {
            Heartbeat beat = new Heartbeat { Node = "oven-1", Seq = 123456789, CpuLoad = 0.25, UptimeSec = 3600 };
            beat.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return beat;
        }

        // Same values as the C++ test MakePie.
        private static enums.Pie MakePie()
        {
            enums.Pie pie = new enums.Pie { Size = enums.PizzaSize.Family, Crust = (enums.PizzaSize)9 };
            pie.Sizes.AddRange(new[] { enums.PizzaSize.Small, enums.PizzaSize.Family, (enums.PizzaSize)5, (enums.PizzaSize)9 });
            pie.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return pie;
        }

        private static void EnumFields()
        {
            enums.Pie defaults = new enums.Pie();
            Equal(enums.PizzaSize.Large, defaults.Size, "default");
            Equal(enums.PizzaSize.Small, defaults.Crust, "first value");
            Check(MakePie().Serialize().SequenceEqual(Golden("Pie.bin")), "same bytes as C++");
            enums.Pie got = Protaco.Codec.ToObject<enums.Pie>(Golden("Pie.bin"));
            Equal(9, (int)got.Crust, "unknown number kept");
            Equal("Pie{size=Family(4), crust=?(9), sizes=[Small(1), Family(4), Kids(5), ?(9)], name=\"\"}", got.ToString(), "text");
            enums.PizzaSize parsed;
            Check(enums.PizzaSizeNames.TryParse("Medium", out parsed) && parsed == enums.PizzaSize.Medium, "parse");
            Check(!enums.PizzaSizeNames.TryParse("Kids", out parsed), "removed name parsed");
            Equal("Kids", enums.PizzaSizeNames.Name((enums.PizzaSize)5), "removed name");
            Equal(null, enums.PizzaSizeNames.Name((enums.PizzaSize)9), "unknown name");
        }

        private static void GeneratorEnumDefault()
        {
            GenResult result = Protacoc("-I", TestSchemas("enumdefault"), "--cpp_out", Path.Combine(TempDir(), "out"));
            Equal(2, result.Exit, "exit code");
            Check(result.Output.Contains("error PC308:") && result.Output.Contains("'Huge' is not a value of enum Size"), "no PC308: " + result.Output);
        }

        private static void EnvelopeIsStampedOnCreation()
        {
            Protaco.Envelopes.SetClock(() => 1760000000123456L);
            DeliveryRoute first = new DeliveryRoute();
            DeliveryRoute second = new DeliveryRoute();
            Equal(first.GetId() + 1, second.GetId(), "next id");
            Equal(1760000000123456L, second.GetTimestamp(), "clock");
            PizzaOrder order = new PizzaOrder();   // one id; its nested Delivery field takes none
            DeliveryRoute decoded = Protaco.Codec.ToObject<DeliveryRoute>(first.Serialize());
            DeliveryRoute third = new DeliveryRoute();
            Equal(second.GetId() + 2, third.GetId(), "decoder and nested fields take no id");
            Equal(first.GetId(), decoded.GetId(), "decoded id");
            Protaco.Envelopes.SetClock(null);
            DeliveryRoute reply = new DeliveryRoute();
            reply.SetCorr(first.GetId());
            reply.SetTimestampAsNow();
            Check(reply.GetTimestamp() > Protaco.Envelopes.TimestampMin, "system clock");
            Equal(first.GetId(), Protaco.Codec.ToObject<DeliveryRoute>(reply.Serialize()).GetCorr(), "corr");
            Check(order.GetId() == second.GetId() + 1, "order id");
        }

        private static void TimestampConvertsToLocalTime()
        {
            DateTime local = new DateTime(2026, 10, 20, 14, 3, 22, DateTimeKind.Local);
            long timestamp = (local.ToUniversalTime().Ticks - 621355968000000000L) / 10 + 123456;
            DeliveryRoute route = new DeliveryRoute();
            route.SetId(9);
            route.SetTimestamp(timestamp);
            string date;
            string time;
            Check(route.TryGetTimeFromTimestamp(out date, out time), "time");
            Equal("2026-10-20", date, "date");
            Equal("14:03:22.123456", time, "time text");
            int year;
            int month;
            int day;
            int hour;
            int minute;
            int second;
            int millisecond;
            int microsecond;
            Check(route.TryGetTimeFromTimestamp(out year, out month, out day, out hour, out minute, out second, out millisecond, out microsecond), "parts");
            Equal(123, millisecond, "ms");
            Equal(456, microsecond, "us");
            Equal("DeliveryRoute{\n  envelope{id=9, corr=0, ts=2026-10-20 14:03:22.123456},\n  rider=\"\",\n  stops=[],\n  note=\"\"\n}",
                route.ToString(Protaco.TextStyle.Pretty()), "pretty");
            route.SetTimestamp(1000);
            string error;
            Check(!route.TryGetTimeFromTimestamp(out date, out time, out error), "1000 is not a time");
            Equal("timestamp 1000 is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)", error, "error");
        }

        // Same values as the C++ test MakeNumbers.
        private static types.Numbers MakeNumbers()
        {
            types.Numbers numbers = new types.Numbers { U32 = 0, Name = "\uD55C\uAE00 \u2713", Text = "caf\u00E9 \u5BFF\u53F8" };
            numbers.Us.AddRange(new uint[] { 0, 1, 4294967295 });
            numbers.U64s.Add(18446744073709551615);
            numbers.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return numbers;
        }

        private static void UnsignedTypesAndUtf8()
        {
            types.Numbers defaults = new types.Numbers();
            Equal(4000000000u, defaults.U32, "u32 default");
            Equal(18446744073709551615ul, defaults.U64, "u64 default");
            Equal(-9000000000L, defaults.I64, "i64 default");
            byte[] bytes = MakeNumbers().Serialize();
            Check(bytes.SequenceEqual(Golden("Numbers.bin")), "same bytes as C++");
            types.Numbers got = Protaco.Codec.ToObject<types.Numbers>(Golden("Numbers.bin"));
            Equal("\uD55C\uAE00 \u2713", got.Name, "name");
            Equal(4294967295u, got.Us[2], "us[2]");
            Equal("Numbers{u32=0, u64=18446744073709551615, i64=-9000000000, us=[0, 1, 4294967295], u64s=[18446744073709551615], "
                + "name=\"\uD55C\uAE00 \u2713\", text=\"caf\u00E9 \u5BFF\u53F8\"}", got.ToString(), "text");
            try
            {
                got.Name = new string('x', 17);
            }
            catch (ArgumentException e)
            {
                Equal("name: 17 bytes, longer than string(16)", e.Message, "fixed string error");
                return;
            }
            throw new Exception("17 bytes accepted");
        }

        // Same values as the C++ test MakeDeliveryProof.
        private static DeliveryProof MakeDeliveryProof()
        {
            DeliveryProof proof = new DeliveryProof { OrderId = "A-1001", Photo = new byte[] { 0xFF, 0xD8, 0x00, 0x10, (byte)'J', (byte)'F', (byte)'I', (byte)'F', 0x00, 0xFF, 0xD9 } };
            proof.Signatures.Add(new byte[] { 0x00 });
            proof.Signatures.Add(new byte[0]);
            proof.Signatures.Add(new byte[] { 0x01, 0x02 });
            proof.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return proof;
        }

        private static byte[] Golden(string name)
        {
            return File.ReadAllBytes(Path.Combine(tests_, "golden", name));
        }

        // Rewrites the fingerprint of the frame's own version so a frame from another yaml reaches the body decoder.
        private static byte[] WithSchemaHash(byte[] frame, uint hash)
        {
            int version = BitConverter.ToUInt16(frame, 4 + frame[3]);
            int minVersion = BitConverter.ToUInt16(frame, 6 + frame[3]);
            Buffer.BlockCopy(BitConverter.GetBytes(hash), 0, frame, 28 + frame[3] + 4 * (version - minVersion), 4);
            return frame;
        }

        // Appends one int TLV to the body and fixes BodyLen.
        private static byte[] WithExtraTlv(byte[] frame, string key, int value)
        {
            List<byte> bytes = new List<byte>(frame);
            bytes.Add((byte)key.Length);
            bytes.AddRange(Encoding.ASCII.GetBytes(key));
            bytes.Add((byte)Protaco.TacoType.Int);
            bytes.AddRange(BitConverter.GetBytes(4u));
            bytes.AddRange(BitConverter.GetBytes(value));
            byte[] result = bytes.ToArray();
            int at = 24 + result[3];
            uint bodySize = BitConverter.ToUInt32(result, at) + (uint)(1 + key.Length + 1 + 4 + 4);
            Buffer.BlockCopy(BitConverter.GetBytes(bodySize), 0, result, at, 4);
            return result;
        }

        // Decoding must fail. TryToObject reports "path: reason"; ToObject throws the same failure split into Path and What.
        private static Protaco.ProtacoException Fails<T>(byte[] bytes) where T : Protaco.IMessage, new()
        {
            T got;
            string text;
            Check(!Protaco.Codec.TryToObject(bytes, out got, out text), typeof(T).FullName + " accepted");
            try
            {
                Protaco.Codec.ToObject<T>(bytes);
            }
            catch (Protaco.ProtacoException e)
            {
                Equal(text, e.Message, "TryToObject error text");
                return e;
            }
            throw new Exception(typeof(T).FullName + " accepted by ToObject");
        }

        private static void RoundTrip()
        {
            byte[] bytes = MakeDeliveryRoute().Serialize();
            DeliveryRoute got = Protaco.Codec.ToObject<DeliveryRoute>(bytes);
            Equal("rider-7", got.Rider, "rider");
            Equal(2, got.Stops.Count, "stops");
            Equal("1 Pizza St", got.Stops[0].Address, "stops[0].address");
            Equal("closest-first", got.Note, "note");
            int priority;
            Check(got.Ext.TryGetValue("priority", out priority) && priority == 5, "ext priority");
            Check(bytes.SequenceEqual(got.Serialize()), "re-serialized bytes differ");
        }

        private static void GoldenEqualsCpp()
        {
            Check(MakeDeliveryRoute().Serialize().SequenceEqual(Golden("DeliveryRoute.bin")), "DeliveryRoute");
            Check(MakePizzaOrder().Serialize().SequenceEqual(Golden("PizzaOrder.bin")), "PizzaOrder");
            Check(MakeHeartbeat().Serialize().SequenceEqual(Golden("Heartbeat.bin")), "Heartbeat");
            Check(MakeDeliveryProof().Serialize().SequenceEqual(Golden("DeliveryProof.bin")), "DeliveryProof");
        }

        private static void ReadsCppGolden()
        {
            DeliveryRoute route = Protaco.Codec.ToObject<DeliveryRoute>(Golden("DeliveryRoute.bin"));
            Equal(7u, route.GetId(), "envelope id");
            Equal(1000L, route.GetTimestamp(), "envelope ts");
            Equal("2 Pasta Ave", route.Stops[1].Address, "stops[1].address");
            PizzaOrder order = Protaco.Codec.ToObject<PizzaOrder>(Golden("PizzaOrder.bin"));
            Equal(5, order.Toppings[0].Grams, "toppings[0].grams");
            Equal(2, order.Delivery.DoorCode[3], "delivery.doorCode[3]");
            Check(order.WoodFired, "woodFired");
            DeliveryProof proof = Protaco.Codec.ToObject<DeliveryProof>(Golden("DeliveryProof.bin"));
            Check(proof.Photo.SequenceEqual(MakeDeliveryProof().Photo), "photo bytes");
            Equal(3, proof.Signatures.Count, "signatures");
            Equal(0, proof.Signatures[1].Length, "signatures[1] empty");
            proof.Ext.SetValue("thumbnail", new byte[] { 0x00, 0xFF });
            byte[] thumbnail;
            Check(Protaco.Codec.ToObject<DeliveryProof>(proof.Serialize()).Ext.TryGetValue("thumbnail", out thumbnail) && thumbnail.SequenceEqual(new byte[] { 0x00, 0xFF }), "ext bytes");
        }

        private static void VersionMismatch()
        {
            Equal("message version too new: Recipe reads version 1, sender supports 2..2", Fails<v1.Recipe>(new v2.Recipe { Path = "/a" }.Serialize()).What, "v2 -> v1");
            Equal("message version too old: Recipe reads version 2 or newer, got 1", Fails<v2.Recipe>(new v1.Recipe { Path = "/b" }.Serialize()).What, "v1 -> v2");
        }

        private static void SchemaMismatch()
        {
            Check(alt.Recipe.MsgSchemaHash != v1.Recipe.MsgSchemaHash, "same fingerprint");
            byte[] bytes = new alt.Recipe { Step = 5, Name = "R1", Path = "/a" }.Serialize();
            Equal("schema mismatch: Recipe version 1 expected 0x" + v1.Recipe.MsgSchemaHash.ToString("X8") + ", got 0x" + alt.Recipe.MsgSchemaHash.ToString("X8"),
                Fails<v1.Recipe>(bytes).What, "alt -> v1");
            Equal(partial.Topping.MsgSchemaHash, Topping.MsgSchemaHash, "namespace changes the fingerprint");
        }

        // Same checks as ProtacoTest.cpp (supported version tests).
        private static void NewVersionKeepsOldFingerprints()
        {
            Equal(build1.Order.MsgSchemaHash, build3.v1.Order.MsgSchemaHash, "v1");
            Equal(build2.Order.MsgSchemaHash, build3.v2.Order.MsgSchemaHash, "v2");
            Equal(build2.Order.MsgSchemaHash, build4.v2.Order.MsgSchemaHash, "v2 in build4");
            Equal(build3.Order.MsgSchemaHash, build4.v3.Order.MsgSchemaHash, "v3 in build4");
        }

        private static build3.Order MakeOrderV3()
        {
            build3.Order order = new build3.Order { Id = "A1", Weight = 2.5, Color = "red", Label = "L", Rush = true };
            order.Tags.Add("x");
            return order;
        }

        private static void NewerFrameReadAsOwnVersion()
        {
            byte[] bytes = MakeOrderV3().Serialize();
            build1.Order reader1 = Protaco.Codec.ToObject<build1.Order>(bytes);
            Equal("A1", reader1.Id, "id");
            Equal(1, reader1.Qty, "qty default kept by the sender");
            Equal("", reader1.Note, "note absent");
            Equal("red", reader1.Color, "color");
            Equal("label,tags", string.Join(",", reader1.Ext.Keys.OrderBy(k => k, StringComparer.Ordinal)), "ext keys");
            build2.Order reader2 = Protaco.Codec.ToObject<build2.Order>(bytes);
            Equal(2.5, reader2.Weight, "weight");
            Equal("x", reader2.Tags[0], "tags");
            Check(bytes.SequenceEqual(Protaco.Codec.ToObject<build3.Order>(bytes).Serialize()), "round trip");
        }

        private static void OlderFrameNeedsOldType()
        {
            byte[] bytes = new build1.Order { Id = "B2", Qty = 4, Note = "n", Color = "blue" }.Serialize();
            Equal("message version too old: Order reads version 3 or newer, got 1. Use the v1 type", Fails<build3.Order>(bytes).What, "latest");
            build3.v1.Order got = Protaco.Codec.ToObject<build3.v1.Order>(bytes);
            Equal(4, got.Qty, "qty");
            Equal("Order{id=\"B2\", qty=4, note=\"n\", color=\"blue\"}", got.ToString(), "text");
        }

        private static void SenderMinVersionAboveReader()
        {
            byte[] bytes = new build4.Order { Id = "C3" }.Serialize();
            Equal("message version too new: Order reads version 1, sender supports 2..4", Fails<build1.Order>(bytes).What, "v1");
            Protaco.ProtacoException error = Fails<build2.Order>(bytes);
            Equal("color", error.Path, "path");
            Equal("missing required (frame version 4 read as 2)", error.What, "what");
        }

        private static void ClosedMessageDropsNewerFields()
        {
            build1.Ping got = Protaco.Codec.ToObject<build1.Ping>(new build2.Ping { Seq = 7, Who = "w", Text = "hi" }.Serialize());
            Equal(7, got.Seq, "seq");
            Equal("unknown key (allow_extension false)", Fails<build1.Ping>(WithExtraTlv(got.Serialize(), "text", 1)).What, "same version");
        }

        private static void ClosedMessage()
        {
            Protaco.ProtacoException error = Fails<Heartbeat>(WithExtraTlv(MakeHeartbeat().Serialize(), "abc", 7));
            Equal("abc", error.Path, "path");
            Equal("unknown key (allow_extension false)", error.What, "what");
            DeliveryRoute open = Protaco.Codec.ToObject<DeliveryRoute>(WithExtraTlv(MakeDeliveryRoute().Serialize(), "abc", 7));
            int value;
            Check(open.Ext.TryGetValue("abc", out value) && value == 7, "open message lost the key");
        }

        private static void Errors()
        {
            Protaco.ProtacoException error = Fails<DeliveryRoute>(WithSchemaHash(new partial.DeliveryRoute { Rider = "rider-7" }.Serialize(), DeliveryRoute.MsgSchemaHash));
            Equal("stops", error.Path, "path");
            Equal("missing required", error.What, "what");
            partial.PizzaOrder sender = new partial.PizzaOrder();
            sender.Toppings.Add(new partial.Topping());
            sender.Delivery.DoorCode.Add("x");
            error = Fails<PizzaOrder>(WithSchemaHash(sender.Serialize(), PizzaOrder.MsgSchemaHash));
            Equal("delivery.doorCode", error.Path, "nested path");
            Equal("list element type mismatch", error.What, "nested what");
            byte[] bytes = new v1.Recipe { Path = "/p" }.Serialize();
            bytes[4 + bytes[3]] = 2;   // version 1 -> 2
            bytes[6 + bytes[3]] = 2;   // min version 2, still one fingerprint
            bytes = WithSchemaHash(bytes, v2.Recipe.MsgSchemaHash);
            Equal("fixed block size mismatch: expected 28, got 20", Fails<v2.Recipe>(bytes).What, "fixed size");
        }

        private static void DeepNesting()
        {
            deep.D8 sent = new deep.D8();
            sent.D.D.D.D.D.D.D.D.V = 42;
            deep.D8 got = Protaco.Codec.ToObject<deep.D8>(sent.Serialize());
            Equal(42, got.D.D.D.D.D.D.D.D.V, "v");
            Equal("D8{d=D7{d=D6{d=D5{d=D4{d=D3{d=D2{d=D1{d=D0{v=42}}}}}}}}}", got.ToString(), "text");
        }

        private static void Truncation()
        {
            byte[] bytes = MakePizzaOrder().Serialize();
            for (int length = 0; length < bytes.Length; ++length)
            {
                PizzaOrder got;
                string error;
                Check(!Protaco.Codec.TryToObject(bytes.Take(length).ToArray(), out got, out error) && !string.IsNullOrEmpty(error), "accepted at length " + length);
            }
        }

        private static void FixedStringOverflow()
        {
            RouteStop stop = new RouteStop { OrderId = new string('y', 16) };
            try
            {
                stop.OrderId = new string('x', 17);
            }
            catch (ArgumentException)
            {
                Equal(new string('y', 16), stop.OrderId, "value changed");
                return;
            }
            throw new Exception("17 bytes accepted");
        }

        private static void ExtRules()
        {
            DeliveryRoute route = new DeliveryRoute();
            string error;
            Check(!route.Ext.SetValue("rider", 1, out error), "schema key accepted");
            Equal("ext key is a field of the message: rider", error, "schema key error");
            Check(!route.Ext.SetValue("_x", 1, out error), "reserved key accepted");
            Equal("ext key starts with '_' (reserved): _x", error, "reserved key error");
            Check(route.Ext.SetValue("channel", "web"), "plain key rejected");
            int wrongType = 7;
            Check(!route.Ext.TryGetValue("channel", out wrongType, out error), "string read as int");
            Equal("ext key channel holds string, not int", error, "type error");
            Equal(0, wrongType, "value on failure");
            string missing;
            Check(!route.Ext.TryGetValue("missing", out missing, out error), "missing key found");
            Equal("ext key not found: missing", error, "missing key error");
        }

        // Same strings as ProtacoTest.cpp ToString* tests.
        private static void ToStringMatchesCpp()
        {
            Equal("DeliveryRoute{rider=\"rider-7\", stops=[RouteStop{orderId=\"A-100\", address=\"1 Pizza St\", etaMin=10}, "
                + "RouteStop{orderId=\"A-101\", address=\"2 Pasta Ave\", etaMin=25}], note=\"closest-first\", ext{priority=5}}", MakeDeliveryRoute().ToString(), "DeliveryRoute");
            Equal("Heartbeat{node=\"oven-1\", seq=123456789, healthy=true, cpuLoad=0.25, uptimeSec=3600}", MakeHeartbeat().ToString(), "Heartbeat");
            Equal("DeliveryProof{orderId=\"A-1001\", photo=<11 bytes: FF D8 00 10 4A 46 49 46 ...>, "
                + "signatures=[<1 bytes: 00>, <0 bytes>, <2 bytes: 01 02>]}", MakeDeliveryProof().ToString(), "DeliveryProof");
            Equal("Recipe{name=\"\", temp=25.5, step=3, path=\"\", note=\"none\"}", new v2.Recipe().ToString(), "defaults");
            Equal("DeliveryRoute{\n"
                + "  envelope{id=7, corr=0, ts=1000},\n"
                + "  rider=\"rider-7\",\n"
                + "  stops=[\n"
                + "    RouteStop{\n"
                + "      orderId=\"A-100\",\n"
                + "      address=\"1 Pizza St\",\n"
                + "      etaMin=10\n"
                + "    },\n"
                + "    RouteStop{\n"
                + "      orderId=\"A-101\",\n"
                + "      address=\"2 Pasta Ave\",\n"
                + "      etaMin=25\n"
                + "    }\n"
                + "  ],\n"
                + "  note=\"closest-first\",\n"
                + "  ext{\n"
                + "    priority=5\n"
                + "  }\n"
                + "}", MakeDeliveryRoute().ToString(Protaco.TextStyle.Pretty()), "pretty");
            Protaco.TextStyle style = new Protaco.TextStyle { Separator = "; ", Open = "(", Close = ")", MaxText = 4 };
            Equal("RouteStop(orderId=\"a\\\"b\\\\\"...(+2); address=\"Main\"; etaMin=3)", new RouteStop { OrderId = "a\"b\\c\n", Address = "Main", EtaMin = 3 }.ToString(style), "style");
            DeliveryRoute route = new DeliveryRoute { Rider = "rider-7" };
            route.Ext.SetValue("priority", 5);
            route.Ext.SetValue("channel", "web");
            route.Ext.SetValue("hot", true);
            route.Ext.SetValue("ratio", 0.5);
            route.Ext.SetValue("big", 1234567890123L);
            route.Ext.SetValue("blob", new byte[] { 0x00, 0xFF });
            Equal("DeliveryRoute{rider=\"rider-7\", stops=[], note=\"\", "
                + "ext{priority=5, channel=\"web\", hot=true, ratio=0.5, big=1234567890123, blob=<2 bytes: 00 FF>}}", route.ToString(), "ext kinds");
        }
    }
}
