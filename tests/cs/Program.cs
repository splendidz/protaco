// Program.cs. Tests for protacoc and the C# runtime. Console runner, no test framework.
// Run: protacoc.Tests.exe <repo root>. Exit code 1 if any test fails.
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
        private static string root_;
        private static int passed_;
        private static readonly List<string> failures_ = new List<string>();

        private static int Main(string[] args)
        {
            root_ = Path.GetFullPath(args.Length > 0 ? args[0] : ".");
            Run("generator: expected error code per bad yaml", GeneratorErrors);
            Run("generator: reference cycle PC403", GeneratorCycle);
            Run("generator: two namespaces in one run PC405", GeneratorNamespaceMix);
            Run("generator: nesting deeper than 8 PC406", GeneratorTooDeep);
            Run("generator: versions on a nested message PC407", GeneratorNestedVersion);
            Run("generator: --namespace option removed", GeneratorNamespaceOption);
            Run("generator: committed example output is up to date (--check)", GeneratorCheckUpToDate);
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
            ProcessStartInfo info = new ProcessStartInfo(Path.Combine(root_, "build", "bin", "protacoc.exe"))
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

        private static string TempDir()
        {
            string dir = Path.Combine(Path.GetTempPath(), "protacoc_test_" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(dir);
            return dir;
        }

        private static string Examples()
        {
            return Path.Combine(root_, "examples", "schemas");
        }

        private static string TestSchemas(string name)
        {
            return Path.Combine(root_, "tests", "schemas", name);
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
            GenResult result = Protacoc("-I", Examples(), "--cpp_out", Path.Combine(TempDir(), "out"), "--namespace", "x");
            Equal(1, result.Exit, "exit code (usage)");
            Check(result.Output.Contains("--namespace was removed"), "no message: " + result.Output);
        }

        private static void GeneratorCheckUpToDate()
        {
            GenResult result = Protacoc("-I", Examples(),
                "--cpp_out", Path.Combine(root_, "examples", "gen", "cpp"), "--cs_out", Path.Combine(root_, "examples", "gen", "cs"), "--check");
            Equal(0, result.Exit, "exit code (" + result.Output.Trim() + ")");
        }

        private static void GeneratorCheckDetectsEdit()
        {
            string dir = TempDir();
            Equal(0, Protacoc("-I", Examples(), "--cpp_out", dir).Exit, "generate");
            File.AppendAllText(Path.Combine(dir, "RouteStop.g.h"), "// hand edit\r\n");
            GenResult result = Protacoc("-I", Examples(), "--cpp_out", dir, "--check");
            Equal(3, result.Exit, "exit code");
            Check(result.Output.Contains("RouteStop.g.h"), "edited file not reported: " + result.Output);
        }

        // Splitting the same yaml into two folders and swapping their order gives identical output.
        private static void GeneratorOrderIndependent()
        {
            string a = TempDir();
            string b = TempDir();
            foreach (string file in Directory.GetFiles(Examples(), "*.yaml"))
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
            route.Ext.SetInt("priority", 5);
            return route;
        }

        private static Protaco.Envelope RouteEnvelope()
        {
            return new Protaco.Envelope { Id = 7, Corr = 0, Ts = 1000 };
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
            return order;
        }

        private static Heartbeat MakeHeartbeat()
        {
            return new Heartbeat { Node = "oven-1", Seq = 123456789, CpuLoad = 0.25, UptimeSec = 3600 };
        }

        // Same values as the C++ test MakeDeliveryProof.
        private static DeliveryProof MakeDeliveryProof()
        {
            DeliveryProof proof = new DeliveryProof { OrderId = "A-1001", Photo = new byte[] { 0xFF, 0xD8, 0x00, 0x10, (byte)'J', (byte)'F', (byte)'I', (byte)'F', 0x00, 0xFF, 0xD9 } };
            proof.Signatures.Add(new byte[] { 0x00 });
            proof.Signatures.Add(new byte[0]);
            proof.Signatures.Add(new byte[] { 0x01, 0x02 });
            return proof;
        }

        private static byte[] Golden(string name)
        {
            return File.ReadAllBytes(Path.Combine(root_, "tests", "golden", name));
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

        private static Protaco.ProtacoException Fails<T>(byte[] bytes) where T : Protaco.IMessage, new()
        {
            T got;
            Protaco.ProtacoException error;
            Check(!Protaco.Codec.TryToObject(bytes, out got, out error), typeof(T).FullName + " accepted");
            return error;
        }

        private static void RoundTrip()
        {
            byte[] bytes = MakeDeliveryRoute().Serialize(RouteEnvelope());
            DeliveryRoute got = Protaco.Codec.ToObject<DeliveryRoute>(bytes);
            Equal("rider-7", got.Rider, "rider");
            Equal(2, got.Stops.Count, "stops");
            Equal("1 Pizza St", got.Stops[0].Address, "stops[0].address");
            Equal("closest-first", got.Note, "note");
            int priority;
            Check(got.Ext.TryGetInt("priority", out priority) && priority == 5, "ext priority");
            Check(bytes.SequenceEqual(got.Serialize(RouteEnvelope())), "re-serialized bytes differ");
        }

        private static void GoldenEqualsCpp()
        {
            Check(MakeDeliveryRoute().Serialize(RouteEnvelope()).SequenceEqual(Golden("DeliveryRoute.bin")), "DeliveryRoute");
            Check(MakePizzaOrder().Serialize().SequenceEqual(Golden("PizzaOrder.bin")), "PizzaOrder");
            Check(MakeHeartbeat().Serialize().SequenceEqual(Golden("Heartbeat.bin")), "Heartbeat");
            Check(MakeDeliveryProof().Serialize().SequenceEqual(Golden("DeliveryProof.bin")), "DeliveryProof");
        }

        private static void ReadsCppGolden()
        {
            Protaco.Envelope env;
            DeliveryRoute route = Protaco.Codec.ToObject<DeliveryRoute>(Golden("DeliveryRoute.bin"), out env);
            Equal(7u, env.Id, "env.id");
            Equal("2 Pasta Ave", route.Stops[1].Address, "stops[1].address");
            PizzaOrder order = Protaco.Codec.ToObject<PizzaOrder>(Golden("PizzaOrder.bin"));
            Equal(5, order.Toppings[0].Grams, "toppings[0].grams");
            Equal(2, order.Delivery.DoorCode[3], "delivery.doorCode[3]");
            Check(order.WoodFired, "woodFired");
            DeliveryProof proof = Protaco.Codec.ToObject<DeliveryProof>(Golden("DeliveryProof.bin"));
            Check(proof.Photo.SequenceEqual(MakeDeliveryProof().Photo), "photo bytes");
            Equal(3, proof.Signatures.Count, "signatures");
            Equal(0, proof.Signatures[1].Length, "signatures[1] empty");
            proof.Ext.SetBytes("thumbnail", new byte[] { 0x00, 0xFF });
            byte[] thumbnail;
            Check(Protaco.Codec.ToObject<DeliveryProof>(proof.Serialize()).Ext.TryGetBytes("thumbnail", out thumbnail) && thumbnail.SequenceEqual(new byte[] { 0x00, 0xFF }), "ext bytes");
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
            Check(open.Ext.TryGetInt("abc", out value) && value == 7, "open message lost the key");
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
                Protaco.ProtacoException error;
                Check(!Protaco.Codec.TryToObject(bytes.Take(length).ToArray(), out got, out error), "accepted at length " + length);
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
            Check(!route.Ext.SetInt("rider", 1), "schema key accepted");
            Check(!route.Ext.SetInt("_x", 1), "reserved key accepted");
            Check(route.Ext.SetString("channel", "web"), "plain key rejected");
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
            route.Ext.SetInt("priority", 5);
            route.Ext.SetString("channel", "web");
            route.Ext.SetBool("hot", true);
            route.Ext.SetDouble("ratio", 0.5);
            route.Ext.SetInt64("big", 1234567890123);
            route.Ext.SetBytes("blob", new byte[] { 0x00, 0xFF });
            Equal("DeliveryRoute{rider=\"rider-7\", stops=[], note=\"\", "
                + "ext{priority=5, channel=\"web\", hot=true, ratio=0.5, big=1234567890123, blob=<2 bytes: 00 FF>}}", route.ToString(), "ext kinds");
        }
    }
}
