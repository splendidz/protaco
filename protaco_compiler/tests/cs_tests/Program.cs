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
            Run("generator: nesting deeper than 100 PC406", GeneratorTooDeep);
            Run("generator: versions on a nested message PC407", GeneratorNestedVersion);
            Run("generator: --namespace option removed", GeneratorNamespaceOption);
            Run("generator: committed demo output is up to date (--check)", GeneratorCheckUpToDate);
            Run("generator: hand edits detected (--check)", GeneratorCheckDetectsEdit);
            Run("generator: .bak files are skipped", GeneratorSkipsBackupFiles);
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
            Run("C#: auto_envelope false and DisableAutoEnvelope", AutoEnvelopeCanBeTurnedOff);
            Run("C#: SetEnvelope from values", EnvelopeSetFromValues);
            Run("C#: const values", ConstValues);
            Run("generator: a const set is not a type PC304", GeneratorConstIsNotAType);
            Run("C#: enum fields match C++", EnumFields);
            Run("C#: fixed-length arrays match C++", FixedArrays);
            Run("C#: fixed messages inline match C++", FixedMessages);
            Run("C#: map fields match C++", MapFields);
            Run("C#: recursive messages match C++", RecursiveMessages);
            Run("C#: recursion stops at 100 levels", RecursionDepthLimit);
            Run("C#: TryView reads fixed fields in place", ViewReadsFixedFieldsInPlace);
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

        // T0..T101, each Tn holding a T(n-1): Tn nests n levels.
        private static void GeneratorTooDeep()
        {
            string dir = TempDir();
            for (int n = 0; n <= 101; ++n)
            {
                string field = n == 0 ? "  - {key: v, type: int}" : "  - {key: inner, type: T" + (n - 1) + "}";
                File.WriteAllText(Path.Combine(dir, "T" + n + ".yaml"), "message: T" + n + "\nversion: 1\nnamespace: toodeep\nfields:\n" + field + "\n");
            }
            GenResult result = Protacoc("-I", dir, "--cpp_out", Path.Combine(dir, "out"));
            Equal(2, result.Exit, "exit code");
            Check(result.Output.Contains("error PC406:") && result.Output.Contains("'T101'"), "no PC406 on T101: " + result.Output);
            Check(!result.Output.Contains("'T100'"), "T100 (100 levels) rejected: " + result.Output);
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
                "--cpp_out", Path.Combine(tests_, "taco_generated", "cpp", "demo"), "--cs_out", Path.Combine(tests_, "taco_generated", "cs", "demo"), "--check");
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

        // Copies named RouteStop.bak.yaml and RouteStop.BAK2.yaml would be PC301 if read.
        private static void GeneratorSkipsBackupFiles()
        {
            string dir = TempDir();
            foreach (string file in Directory.GetFiles(Demo(), "*.yaml"))
                File.Copy(file, Path.Combine(dir, Path.GetFileName(file)));
            File.Copy(Path.Combine(dir, "RouteStop.yaml"), Path.Combine(dir, "RouteStop.bak.yaml"));
            File.Copy(Path.Combine(dir, "RouteStop.yaml"), Path.Combine(dir, "RouteStop.BAK2.yaml"));
            GenResult result = Protacoc("-I", dir, "--cpp_out", Path.Combine(dir, "out"));
            Equal(0, result.Exit, "exit code (" + result.Output.Trim() + ")");
            Check(result.Output.Contains("RouteStop.bak.yaml (.bak in the file name)") && result.Output.Contains("RouteStop.BAK2.yaml (.bak in the file name)"), "skip not reported: " + result.Output);
            Check(File.Exists(Path.Combine(dir, "out", "RouteStop.g.h")), "RouteStop.g.h missing");
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

        private static void ConstValues()
        {
            const int slots = arrays.Sizes.Slots;   // compile-time constant
            Equal(4, slots, "Slots");
            Equal(2, arrays.Sizes.Names, "Names");
            Equal(-7, arrays.Sizes.Offset, "Offset");
            Equal(1, arrays.Sizes.pass, "keyword key");
        }

        private static void GeneratorConstIsNotAType()
        {
            string dir = TempDir();
            File.WriteAllText(Path.Combine(dir, "Sizes.const.yaml"), "const: Sizes\nnamespace: ct\nvalues:\n  - {key: Slots, value: 4}\n");
            File.WriteAllText(Path.Combine(dir, "Row.yaml"), "message: Row\nversion: 1\nnamespace: ct\nfields:\n  - {key: s, type: Sizes}\n");
            GenResult result = Protacoc("-I", dir, "--cpp_out", Path.Combine(dir, "out"));
            Equal(2, result.Exit, "exit code");
            Check(result.Output.Contains("error PC304:") && result.Output.Contains("'Sizes' is a const set, not a type"), "no PC304: " + result.Output);
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

        // Same values as the C++ test MakeMenu.
        // Golden values. Same as the C++ test MakePanel.
        private static arrays.Panel MakePanel()
        {
            arrays.Panel panel = new arrays.Panel { Total = 42 };
            for (int i = 0; i < panel.Temps.Length; ++i)
                panel.Temps[i] = 20.5 + i;
            new[] { 1, -2, 3, -4 }.CopyTo(panel.Counts, 0);
            new[] { 1ul, 18446744073709551615ul, 7ul }.CopyTo(panel.Ids, 0);
            panel.Masks[1] = 4294967295u;
            panel.Ticks[0] = -9000000000L;
            panel.Flags[1] = true;
            panel.Names[0] = "alpha";
            panel.Levels[2] = arrays.Level.High;
            panel.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return panel;
        }

        private static void FixedArrays()
        {
            arrays.Panel defaults = new arrays.Panel();
            Check(defaults.Counts.SequenceEqual(new[] { -1, -1, -1, -1 }), "counts default");
            Equal("none", defaults.Names[1], "names default");
            Equal(arrays.Level.Low, defaults.Levels[2], "levels default");
            Equal(arrays.Sizes.Slots, defaults.Temps.Length, "length from a const");
            byte[] bytes = MakePanel().Serialize();
            Check(bytes.SequenceEqual(Golden("Panel.bin")), "same bytes as C++");
            arrays.Panel got = Protaco.Codec.ToObject<arrays.Panel>(Golden("Panel.bin"));
            Equal(-9000000000L, got.Ticks[0], "ticks");
            Equal("Panel{temps=[20.5, 21.5, 22.5, 23.5], counts=[1, -2, 3, -4], ids=[1, 18446744073709551615, 7], masks=[0, 4294967295], ticks=[-9000000000, 0], flags=[false, true], names=[\"alpha\", \"none\"], levels=[Low(1), Low(1), High(2)], total=42}", got.ToString(), "text");
            arrays.Panel.View view;
            Check(arrays.Panel.TryView(bytes, out view), "view");
            Equal(-2, view.Counts(1), "view element");
            Equal("alpha", view.Names(0), "view string");
            Check(view.Temps().SequenceEqual(got.Temps), "view copy");
            try
            {
                view.Counts(4);
                Check(false, "index 4 accepted");
            }
            catch (ArgumentOutOfRangeException)
            {
            }
            arrays.Panel bad = MakePanel();
            bad.Names[1] = "123456789";
            try
            {
                bad.Serialize();
                Check(false, "long element accepted");
            }
            catch (Protaco.ProtacoException e)
            {
                Equal("names[1]: 9 bytes, longer than string(8)", e.Message, "error");
            }
        }

        // Golden values. Same as the C++ test MakeRack.
        private static arrays.Rack MakeRack()
        {
            arrays.Rack rack = new arrays.Rack { Count = 4 };
            rack.Origin.X = 3;
            for (int i = 0; i < rack.Slots.Length; ++i)
            {
                rack.Slots[i].Id = (uint)(100 + i);
                rack.Slots[i].Pos.Y = i;
            }
            rack.Slots[1].Name = "one";
            rack.Slots[2].Path[1].X = 7;
            rack.Slots[3].Level = arrays.Level.High;
            rack.Slots[3].Temps[1] = 1.5;
            rack.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return rack;
        }

        // Golden values. Same as the C++ test MakeTrail.
        private static arrays.Trail MakeTrail()
        {
            arrays.Trail trail = new arrays.Trail { Note = "n" };
            trail.Start.Y = 5;
            trail.Points.Add(new arrays.Point { X = 3, Y = 0 });
            trail.SetEnvelope(default(Protaco.Envelope));
            return trail;
        }

        private static void FixedMessages()
        {
            arrays.Rack defaults = new arrays.Rack();
            Equal(-1, defaults.Origin.X, "inline default");
            Equal("empty", defaults.Slots[3].Name, "element default");
            Equal(-1, defaults.Slots[0].Path[1].X, "nested default");
            Equal(arrays.Sizes.Slots, defaults.Slots.Length, "length from a const");
            byte[] bytes = MakeRack().Serialize();
            Check(bytes.SequenceEqual(Golden("Rack.bin")), "same bytes as C++");
            arrays.Rack got = Protaco.Codec.ToObject<arrays.Rack>(Golden("Rack.bin"));
            Equal(7, got.Slots[2].Path[1].X, "nested element");
            Equal("Rack{origin=Point{x=3, y=0}, slots=[Slot{id=100, name=\"empty\", pos=Point{x=-1, y=0}, path=[Point{x=-1, y=0}, Point{x=-1, y=0}], level=Low(1), temps=[0, 0]}, "
                + "Slot{id=101, name=\"one\", pos=Point{x=-1, y=1}, path=[Point{x=-1, y=0}, Point{x=-1, y=0}], level=Low(1), temps=[0, 0]}, "
                + "Slot{id=102, name=\"empty\", pos=Point{x=-1, y=2}, path=[Point{x=-1, y=0}, Point{x=7, y=0}], level=Low(1), temps=[0, 0]}, "
                + "Slot{id=103, name=\"empty\", pos=Point{x=-1, y=3}, path=[Point{x=-1, y=0}, Point{x=-1, y=0}], level=High(2), temps=[0, 1.5]}], count=4}", got.ToString(), "text");
            arrays.Rack.View view;
            Check(arrays.Rack.TryView(bytes, out view), "view");
            Equal(1.5, view.Slots(3).Temps(1), "view nested array");
            Equal(3, view.Origin.X, "view inline");
            Equal("one", view.Slots(1).Copy().Name, "ConstRef copy");
            Equal("Point{x=3, y=0}", view.Origin.ToString(), "ConstRef text");
            Check(MakeTrail().Serialize().SequenceEqual(Golden("Trail.bin")), "trail bytes");
            Equal("Trail{start=Point{x=-1, y=5}, points=[Point{x=3, y=0}], note=\"n\"}", Protaco.Codec.ToObject<arrays.Trail>(Golden("Trail.bin")).ToString(), "trail text");
            arrays.Rack bad = MakeRack();
            bad.Slots[1] = null;
            try
            {
                bad.Serialize();
                Check(false, "null element accepted");
            }
            catch (Protaco.ProtacoException e)
            {
                Equal("slots[1]: null fixed message", e.Message, "error");
            }
        }

        private static maps.Menu MakeMenu()
        {
            maps.Menu menu = new maps.Menu { Title = "lunch" };
            menu.Prices["dupA"] = 9.5;
            menu.Stock[-1] = 4294967295u;
            menu.Specials[maps.Grade.Silver] = new maps.Dish { Name = "ragu", Price = 12.25 };
            menu.Notes[18446744073709551615ul] = "\uB9E4\uC6B4\uB9DB";
            menu.Vegan["ragu"] = false;
            menu.Photos["ragu"] = new byte[] { 0xFF, 0x00 };
            menu.Grades["ragu"] = (maps.Grade)9;
            menu.Tags["ragu"] = new List<string> { "hot", "" };
            menu.Daily[20261020] = new Dictionary<string, int> { { "ragu", 3 } };
            menu.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return menu;
        }

        private static void MapFields()
        {
            Check(MakeMenu().Serialize().SequenceEqual(Golden("Menu.bin")), "same bytes as C++");
            maps.Menu golden = Protaco.Codec.ToObject<maps.Menu>(Golden("Menu.bin"));
            Equal("Menu{prices={\"dupA\": 9.5}, stock={-1: 4294967295}, specials={Silver(2): Dish{name=\"ragu\", price=12.25}}, notes={18446744073709551615: \"\uB9E4\uC6B4\uB9DB\"}, vegan={\"ragu\": false}, photos={\"ragu\": <2 bytes: FF 00>}, grades={\"ragu\": ?(9)}, tags={\"ragu\": [\"hot\", \"\"]}, daily={20261020: {\"ragu\": 3}}, title=\"lunch\"}", golden.ToString(), "text");
            Equal(9, (int)golden.Grades["ragu"], "unknown enum number kept");

            maps.Menu sent = MakeMenu();
            sent.Prices["dupB"] = 7.0;
            sent.Prices["coffee"] = 3.5;
            sent.Stock[2] = 0u;
            maps.Menu got = Protaco.Codec.ToObject<maps.Menu>(sent.Serialize());
            Equal(3, got.Prices.Count, "prices");
            Equal(7.0, got.Prices["dupB"], "dupB");
            Equal(2, got.Tags["ragu"].Count, "list value");
            Equal(3, got.Daily[20261020]["ragu"], "map value");
            string text = got.ToString();
            Equal("Menu{prices={\"coffee\": 3.5, \"dupA\": 9.5, \"dupB\": 7}, stock={-1: 4294967295, 2: 0}, specials={Silver(2): Dish{name=\"ragu\", price=12.25}}, ", text.Substring(0, text.IndexOf("notes=")), "sorted text");

            // Same key twice: "dupB" patched to "dupA". The entry index depends on the dictionary order.
            byte[] bytes = sent.Serialize();
            byte[] dupB = { 4, 0, 0, 0, (byte)'d', (byte)'u', (byte)'p', (byte)'B' };
            int at = Enumerable.Range(0, bytes.Length - dupB.Length).First(i => bytes.Skip(i).Take(dupB.Length).SequenceEqual(dupB));
            bytes[at + 7] = (byte)'A';
            Protaco.ProtacoException e = Fails<maps.Menu>(bytes);
            Check(e.Path.StartsWith("prices[") && e.What == "duplicate map key", "duplicate key: " + e.Message);
        }

        // Same values as the C++ test MakeTree.
        private static recursive.Node MakeTree()
        {
            recursive.Node root = new recursive.Node { Name = "root", Value = 1 };
            root.Children.Add(new recursive.Node { Name = "a", Value = 2, Next = new recursive.Node { Name = "b", Value = 3 } });
            root.Children.Add(new recursive.Node { Name = "c", Value = 4 });
            root.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return root;
        }

        private static void RecursiveMessages()
        {
            Check(MakeTree().Serialize().SequenceEqual(Golden("Node.bin")), "same bytes as C++");
            recursive.Node got = Protaco.Codec.ToObject<recursive.Node>(Golden("Node.bin"));
            Equal("Node{name=\"root\", value=1, children=[Node{name=\"a\", value=2, children=[], next=Node{name=\"b\", value=3, children=[], next=null}}, Node{name=\"c\", value=4, children=[], next=null}], next=null}", got.ToString(), "text");
            Equal(null, got.Next, "absent");
            Equal("b", got.Children[0].Next.Name, "next");
        }

        private static void RecursionDepthLimit()
        {
            foreach (int levels in new[] { 100, 101 })
            {
                recursive.Node root = new recursive.Node();
                recursive.Node node = root;
                for (int i = 0; i < levels; ++i)
                {
                    node.Next = new recursive.Node();
                    node = node.Next;
                }
                recursive.Node got;
                string error;
                bool ok = Protaco.Codec.TryToObject(root.Serialize(), out got, out error);
                if (levels == 100)
                    Check(ok, "100 levels: " + error);
                else
                    Check(!ok && error.StartsWith("next.next.next.") && error.EndsWith(": nesting too deep"), "101 levels: " + error);
            }
        }

        private static void ViewReadsFixedFieldsInPlace()
        {
            Heartbeat sent = MakeHeartbeat();
            sent.SetCorr(77);
            byte[] bytes = sent.Serialize();
            Heartbeat.View view;
            Check(Heartbeat.TryView(bytes, out view), "view");
            Equal("oven-1", view.Node, "node");
            Equal(123456789L, view.Seq, "seq");
            Equal(true, view.Healthy, "healthy");
            Equal(0.25, view.CpuLoad, "cpuLoad");
            Equal(3600, view.UptimeSec, "uptimeSec");
            Equal(77u, view.GetEnvelope().Corr, "corr");
            for (int length = 0; length < bytes.Length; ++length)
            {
                byte[] cut = bytes.Take(length).ToArray();
                string viewError;
                string objectError;
                Heartbeat object_;
                Check(!Heartbeat.TryView(cut, out view, out viewError), "view accepted at length " + length);
                Check(!Protaco.Codec.TryToObject(cut, out object_, out objectError), "object accepted at length " + length);
                Equal(objectError, viewError, "error at length " + length);
            }
            string error;
            Check(!Heartbeat.TryView(WithExtraTlv(bytes, "abc", 7), out view, out error), "unknown key accepted");
            Equal("abc: unknown key (allow_extension false)", error, "unknown key");
            Check(!Heartbeat.TryView(MakeDeliveryRoute().Serialize(), out view, out error), "other message accepted");
            Equal("message name mismatch: expected Heartbeat, got DeliveryRoute", error, "name");
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

        // SetEnvelope(id, corr, ts): corr 0 and ts NowUs() by default. Same as the C++ test.
        private static void EnvelopeSetFromValues()
        {
            Protaco.Envelopes.SetClock(() => 1760000000123456L);
            DeliveryRoute route = new DeliveryRoute();
            route.SetEnvelope(41);
            Heartbeat beat = new Heartbeat();
            beat.SetEnvelope(42, 7);
            Protaco.Envelopes.SetClock(null);
            Equal(41u, route.GetId(), "id");
            Equal(0u, route.GetCorr(), "default corr");
            Equal(1760000000123456L, route.GetTimestamp(), "default ts");
            Equal(42u, beat.GetId(), "id with corr");
            Equal(7u, beat.GetCorr(), "corr");
            route.SetEnvelope(43, 8, 9);
            DeliveryRoute decoded = Protaco.Codec.ToObject<DeliveryRoute>(route.Serialize());
            Equal(43u, decoded.GetId(), "decoded id");
            Equal(8u, decoded.GetCorr(), "decoded corr");
            Equal(9L, decoded.GetTimestamp(), "decoded ts");
        }

        // auto_envelope false (Tick) and DisableAutoEnvelope() leave Id and Ts at 0 and take no id from the counter.
        private static void AutoEnvelopeCanBeTurnedOff()
        {
            DeliveryRoute before = new DeliveryRoute();
            scalars.Tick tick = new scalars.Tick();
            Equal(0u, tick.GetId(), "Tick id");
            Equal(0L, tick.GetTimestamp(), "Tick ts");
            Protaco.Envelopes.DisableAutoEnvelope();
            DeliveryRoute off = new DeliveryRoute();
            Protaco.Envelopes.EnableAutoEnvelope();
            Equal(0u, off.GetId(), "id while disabled");
            Equal(0L, off.GetTimestamp(), "ts while disabled");
            DeliveryRoute after = new DeliveryRoute();
            Equal(before.GetId() + 1, after.GetId(), "counter untouched");
            scalars.Tick sent = new scalars.Tick();
            sent.SetId(Protaco.Envelopes.NextId());
            sent.SetTimestampAsNow();
            scalars.Tick got = Protaco.Codec.ToObject<scalars.Tick>(sent.Serialize());
            Equal(sent.GetId(), got.GetId(), "decoded id");
            Equal(sent.GetTimestamp(), got.GetTimestamp(), "decoded ts");
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
        private static scalars.Numbers MakeNumbers()
        {
            scalars.Numbers numbers = new scalars.Numbers { U32 = 0, Name = "\uD55C\uAE00 \u2713", Text = "caf\u00E9 \u5BFF\u53F8" };
            numbers.Us.AddRange(new uint[] { 0, 1, 4294967295 });
            numbers.U64s.Add(18446744073709551615);
            numbers.SetEnvelope(default(Protaco.Envelope));   // golden bytes: no id, no time
            return numbers;
        }

        private static void UnsignedTypesAndUtf8()
        {
            scalars.Numbers defaults = new scalars.Numbers();
            Equal(4000000000u, defaults.U32, "u32 default");
            Equal(18446744073709551615ul, defaults.U64, "u64 default");
            Equal(-9000000000L, defaults.I64, "i64 default");
            byte[] bytes = MakeNumbers().Serialize();
            Check(bytes.SequenceEqual(Golden("Numbers.bin")), "same bytes as C++");
            scalars.Numbers got = Protaco.Codec.ToObject<scalars.Numbers>(Golden("Numbers.bin"));
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
