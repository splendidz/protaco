// Program.cs. protacoc.exe command line (protoc style).
//   protacoc -I <yaml dir> [-I ...] [--cpp_out <dir>] [--cs_out <dir>] [--py_out <dir>] [--no_runtime] [--check] [name or yaml path...]
// Inputs: <Name>.yaml (one message), <Name>.enum.yaml (one enum), <Name>.const.yaml (one const set).
// A file name containing .bak is skipped.
// The namespace comes from each yaml header (namespace:).
// Exit codes: 0 success, 1 usage, 2 schema error, 3 --check mismatch
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Protacoc
{
    internal static class Program
    {
        private sealed class Options
        {
            public List<string> ProtoPaths = new List<string>();
            public string CppOut;
            public string CsOut;
            public string PyOut;
            public bool Runtime = true;
            public bool Check;
            public List<string> Targets = new List<string>();
        }

        private static int Main(string[] args)
        {
            Options options;
            string usageError = Parse(args, out options);
            if (usageError != null)
            {
                if (usageError.Length > 0)
                    Console.Error.WriteLine("protacoc: error " + Codes.Usage + ": " + usageError);
                PrintUsage();
                return usageError.Length > 0 ? 1 : 0;
            }
            Diag diag = new Diag();
            List<SchemaDef> defs = LoadAll(options, diag);
            List<MessageDef> order = null;
            List<EnumDef> enums = null;
            if (diag.ErrorCount != 0 || !Linker.Link(defs, diag, out order, out enums))
            {
                diag.Print();
                Console.Error.WriteLine("protacoc: " + diag.ErrorCount + " error(s)");
                return 2;
            }
            HashSet<SchemaDef> targets = SelectTargets(defs, options, diag);
            if (diag.ErrorCount != 0)
            {
                diag.Print();
                return 2;
            }
            OutputSet output = new OutputSet(options.Check, diag);
            Emit(order, enums, targets, options, output);
            if (diag.ErrorCount != 0)
            {
                diag.Print();
                return 2;
            }
            foreach (string path in output.Stale)
                Console.Error.WriteLine(path + ": error " + Codes.Io + ": out of date (run protacoc again)");
            Console.WriteLine(output.Summary());
            return output.Stale.Count == 0 ? 0 : 3;
        }

        // Returns null on success, "" for help only, otherwise the error text.
        private static string Parse(string[] args, out Options options)
        {
            options = new Options();
            for (int i = 0; i < args.Length; ++i)
            {
                string arg = args[i];
                Func<string> next = () => i + 1 < args.Length ? args[++i] : null;
                switch (arg)
                {
                    case "-h":
                    case "--help":
                        return "";
                    case "--version":
                        Console.WriteLine("protacoc " + Naming.Version);
                        Environment.Exit(0);
                        break;
                    case "-I":
                    case "--proto_path":
                        string dir = next();
                        if (dir == null)
                            return "folder expected after " + arg;
                        options.ProtoPaths.Add(dir);
                        break;
                    case "--cpp_out":
                        options.CppOut = next();
                        break;
                    case "--cs_out":
                        options.CsOut = next();
                        break;
                    case "--py_out":
                        options.PyOut = next();
                        break;
                    case "--namespace":
                        return "--namespace was removed. Set the namespace in each yaml header (namespace: a.b)";
                    case "--no_runtime":
                        options.Runtime = false;
                        break;
                    case "--check":
                        options.Check = true;
                        break;
                    default:
                        if (arg.StartsWith("-"))
                            return "unknown option " + arg;
                        options.Targets.Add(arg);
                        break;
                }
            }
            if (options.ProtoPaths.Count == 0)
                return "-I <yaml dir> required";
            if (options.CppOut == null && options.CsOut == null && options.PyOut == null)
                return "--cpp_out, --cs_out or --py_out required";
            return null;
        }

        private static void PrintUsage()
        {
            Console.WriteLine("protacoc " + Naming.Version + ". message yaml to C++ (.g.h) / C# (.g.cs) / Python (_g.py)");
            Console.WriteLine("usage: protacoc -I <yaml dir> [-I ...] [--cpp_out <dir>] [--cs_out <dir>] [--py_out <dir>] [options] [message...]");
            Console.WriteLine("  -I, --proto_path <dir>   yaml folder. Several folders share one name space");
            Console.WriteLine("  --cpp_out <dir>          writes <message>.g.h, <enum>.enum.h, <const>.const.h and the runtime Protaco.hpp");
            Console.WriteLine("  --cs_out <dir>           writes <message>.g.cs, <enum>.enum.cs, <const>.const.cs and the runtime Protaco.cs");
            Console.WriteLine("  --py_out <dir>           writes <namespace folders>/<message>_g.py, <enum>_enum.py, <const>_const.py and the runtime protaco.py");
            Console.WriteLine("  --no_runtime             skips the runtime files");
            Console.WriteLine("  --check                  checks without writing. Exit code 3 if anything differs");
            Console.WriteLine("  name...                  generates only these messages, enums or consts (name or yaml path). Default: all");
        }

        private static List<SchemaDef> LoadAll(Options options, Diag diag)
        {
            List<SchemaDef> defs = new List<SchemaDef>();
            foreach (string dir in options.ProtoPaths)
            {
                if (!Directory.Exists(dir))
                {
                    diag.Error(Codes.Io, "folder not found: " + dir);
                    continue;
                }
                foreach (string path in Directory.GetFiles(dir, "*.yaml").OrderBy(p => p, StringComparer.Ordinal))
                {
                    if (Loader.IsBackupFile(path))
                    {
                        Console.WriteLine("protacoc: skipped " + path + " (.bak in the file name)");
                        continue;
                    }
                    SchemaDef def = Loader.Load(path, diag);
                    if (def != null)
                        defs.Add(def);
                }
            }
            return defs;
        }

        private static HashSet<SchemaDef> SelectTargets(List<SchemaDef> defs, Options options, Diag diag)
        {
            if (options.Targets.Count == 0)
                return new HashSet<SchemaDef>(defs);
            HashSet<SchemaDef> targets = new HashSet<SchemaDef>();
            foreach (string target in options.Targets)
            {
                string name = target.EndsWith(".yaml", StringComparison.OrdinalIgnoreCase) ? Loader.Stem(target) : target;
                SchemaDef found = defs.FirstOrDefault(m => m.Name == name);
                if (found == null)
                    diag.Error(Codes.Usage, "message, enum or const not found: " + target);
                else
                    targets.Add(found);
            }
            return targets;
        }

        private static void Emit(List<MessageDef> order, List<EnumDef> enums, HashSet<SchemaDef> targets, Options options, OutputSet output)
        {
            foreach (ConstDef constDef in targets.OfType<ConstDef>().OrderBy(c => c.Name, StringComparer.Ordinal))
            {
                if (options.CppOut != null)
                    output.Write(Path.Combine(options.CppOut, constDef.Name + ".const.h"), CppEmitter.EmitConst(constDef, constDef.Namespace.Split('.')));
                if (options.CsOut != null)
                    output.Write(Path.Combine(options.CsOut, constDef.Name + ".const.cs"), CsEmitter.EmitConst(constDef, constDef.Namespace));
                if (options.PyOut != null)
                    output.Write(Path.Combine(PyPackage(options.PyOut, constDef), constDef.Name + "_const.py"), PyEmitter.EmitConst(constDef));
            }
            foreach (EnumDef enumDef in enums.Where(targets.Contains))
            {
                if (options.CppOut != null)
                    output.Write(Path.Combine(options.CppOut, enumDef.Name + ".enum.h"), CppEmitter.EmitEnum(enumDef, enumDef.Namespace.Split('.')));
                if (options.CsOut != null)
                    output.Write(Path.Combine(options.CsOut, enumDef.Name + ".enum.cs"), CsEmitter.EmitEnum(enumDef, enumDef.Namespace));
                if (options.PyOut != null)
                    output.Write(Path.Combine(PyPackage(options.PyOut, enumDef), enumDef.Name + "_enum.py"), PyEmitter.EmitEnum(enumDef));
            }
            foreach (MessageDef message in order.Where(targets.Contains))
            {
                if (options.CppOut != null)
                    output.Write(Path.Combine(options.CppOut, message.Name + ".g.h"), CppEmitter.Emit(message, message.Namespace.Split('.')));
                if (options.CsOut != null)
                    output.Write(Path.Combine(options.CsOut, message.Name + ".g.cs"), CsEmitter.Emit(message, message.Namespace));
                if (options.PyOut != null)
                    output.Write(Path.Combine(PyPackage(options.PyOut, message), message.Name + "_g.py"), PyEmitter.Emit(message));
            }
            if (!options.Runtime)
                return;
            if (options.CppOut != null)
                output.Write(Path.Combine(options.CppOut, "Protaco.hpp"), Resource("Protaco.hpp"));
            if (options.CsOut != null)
                output.Write(Path.Combine(options.CsOut, "Protaco.cs"), Resource("Protaco.cs"));
            if (options.PyOut != null)
                output.Write(Path.Combine(options.PyOut, "protaco.py"), Resource("protaco.py"));
        }

        // Python package folder of a namespace: shop.orders -> <py_out>/shop/orders.
        private static string PyPackage(string pyOut, SchemaDef def)
        {
            return Path.Combine(new[] { pyOut }.Concat(def.Namespace.Split('.')).ToArray());
        }

        // Runtime files are embedded resources, written next to generated code so versions always match.
        private static string Resource(string name)
        {
            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name))
            using (StreamReader reader = new StreamReader(stream, System.Text.Encoding.UTF8, true))
                return reader.ReadToEnd();
        }
    }
}
