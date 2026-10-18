// Program.cs. protacoc.exe command line (protoc style).
//   protacoc -I <yaml dir> [-I ...] --cpp_out <dir> [--cs_out <dir>] [--no_runtime] [--check] [message name or yaml path...]
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
            List<MessageDef> messages = LoadAll(options, diag);
            List<MessageDef> order = diag.ErrorCount == 0 ? Linker.Link(messages, diag) : null;
            if (order == null)
            {
                diag.Print();
                Console.Error.WriteLine("protacoc: " + diag.ErrorCount + " error(s)");
                return 2;
            }
            HashSet<MessageDef> targets = SelectTargets(order, options, diag);
            if (diag.ErrorCount != 0)
            {
                diag.Print();
                return 2;
            }
            OutputSet output = new OutputSet(options.Check, diag);
            Emit(order, targets, options, output);
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
            if (options.CppOut == null && options.CsOut == null)
                return "--cpp_out or --cs_out required";
            return null;
        }

        private static void PrintUsage()
        {
            Console.WriteLine("protacoc " + Naming.Version + ". message yaml to C++ (.g.h) / C# (.g.cs)");
            Console.WriteLine("usage: protacoc -I <yaml dir> [-I ...] --cpp_out <dir> [--cs_out <dir>] [options] [message...]");
            Console.WriteLine("  -I, --proto_path <dir>   yaml folder. Several folders share one name space");
            Console.WriteLine("  --cpp_out <dir>          writes <message>.g.h and the runtime Protaco.hpp");
            Console.WriteLine("  --cs_out <dir>           writes <message>.g.cs and the runtime Protaco.cs");
            Console.WriteLine("  --no_runtime             skips the runtime files");
            Console.WriteLine("  --check                  checks without writing. Exit code 3 if anything differs");
            Console.WriteLine("  message...               generates only these (name or yaml path). Default: all");
        }

        private static List<MessageDef> LoadAll(Options options, Diag diag)
        {
            List<MessageDef> messages = new List<MessageDef>();
            foreach (string dir in options.ProtoPaths)
            {
                if (!Directory.Exists(dir))
                {
                    diag.Error(Codes.Io, "folder not found: " + dir);
                    continue;
                }
                foreach (string path in Directory.GetFiles(dir, "*.yaml").OrderBy(p => p, StringComparer.Ordinal))
                {
                    MessageDef message = Loader.Load(path, diag);
                    if (message != null)
                        messages.Add(message);
                }
            }
            return messages;
        }

        private static HashSet<MessageDef> SelectTargets(List<MessageDef> order, Options options, Diag diag)
        {
            if (options.Targets.Count == 0)
                return new HashSet<MessageDef>(order);
            HashSet<MessageDef> targets = new HashSet<MessageDef>();
            foreach (string target in options.Targets)
            {
                string name = target.EndsWith(".yaml", StringComparison.OrdinalIgnoreCase) ? Path.GetFileNameWithoutExtension(target) : target;
                MessageDef found = order.FirstOrDefault(m => m.Name == name);
                if (found == null)
                    diag.Error(Codes.Usage, "message not found: " + target);
                else
                    targets.Add(found);
            }
            return targets;
        }

        private static void Emit(List<MessageDef> order, HashSet<MessageDef> targets, Options options, OutputSet output)
        {
            foreach (MessageDef message in order.Where(targets.Contains))
            {
                if (options.CppOut != null)
                    output.Write(Path.Combine(options.CppOut, message.Name + ".g.h"), CppEmitter.Emit(message, message.Namespace.Split('.')));
                if (options.CsOut != null)
                    output.Write(Path.Combine(options.CsOut, message.Name + ".g.cs"), CsEmitter.Emit(message, message.Namespace));
            }
            if (!options.Runtime)
                return;
            if (options.CppOut != null)
                output.Write(Path.Combine(options.CppOut, "Protaco.hpp"), Resource("Protaco.hpp"));
            if (options.CsOut != null)
                output.Write(Path.Combine(options.CsOut, "Protaco.cs"), Resource("Protaco.cs"));
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
