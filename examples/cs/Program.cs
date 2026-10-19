// Program.cs. How to use protacoc generated code from C#. Same steps as ../cpp/main.cpp.
using System;
using demo;

namespace ProtacoExample
{
    internal static class Program
    {
        // 1. Basics: send and receive a message with a list of nested messages.
        private static void SendAndReceive()
        {
            DeliveryRoute route = new DeliveryRoute { Rider = "rider-7", Note = "closest-first" };
            route.Stops.Add(new RouteStop { OrderId = "A-100", Address = "1 Pizza St", EtaMin = 10 });

            byte[] bytes = route.Serialize();   // same serialization format as the C++ example
            Console.WriteLine("DeliveryRoute: " + bytes.Length + " bytes");

            DeliveryRoute got = Protaco.Codec.ToObject<DeliveryRoute>(bytes);
            Console.WriteLine("  rider=" + got.Rider + " first=" + got.Stops[0].OrderId + " at " + got.Stops[0].Address
                + " in " + got.Stops[0].EtaMin + " min, note=" + got.Note);
        }

        // 2. Extra data without touching the yaml (ext). Receivers keep keys they do not know.
        private static void ExtraData()
        {
            PizzaOrder order = new PizzaOrder { OrderId = "A-100" };
            order.Toppings.Add(new Topping { Name = "basil" });
            order.Ext.SetValue("secretSauce", "yes please");

            PizzaOrder got = Protaco.Codec.ToObject<PizzaOrder>(order.Serialize());
            string sauce;
            got.Ext.TryGetValue("secretSauce", out sauce);
            Console.WriteLine("PizzaOrder ext: secretSauce=" + sauce + ", schema key in ext=" + (order.Ext.SetValue("orderId", 9) ? "accepted (bug)" : "rejected"));
        }

        // 3. Errors: TryToObject never throws.
        private static void ErrorHandling()
        {
            byte[] bytes = new Heartbeat().Serialize();
            DeliveryRoute wrong;
            string error;
            if (!Protaco.Codec.TryToObject(bytes, out wrong, out error))
                Console.WriteLine("decode failed: " + error);
        }

        // 4. Logs: ToString shows every field, ext entries under ext{...}.
        private static void Logging()
        {
            DeliveryRoute route = new DeliveryRoute { Rider = "rider-7" };
            route.Stops.Add(new RouteStop { OrderId = "A-100", Address = "1 Pizza St" });
            route.Ext.SetValue("priority", 5);
            Console.WriteLine(route.ToString());
            Console.WriteLine(route.ToString(Protaco.TextStyle.Pretty()));
        }

        // 5. Enums, maps and a message that holds itself (a tree of menu sections).
        private static void EnumsMapsAndTrees()
        {
            Menu menu = new Menu { Store = "NAPOLI-1" };
            menu.Prices["margherita"] = 9.5;
            menu.Prices["marinara"] = 8.0;
            Category pizza = new Category { Name = "pizza" };
            pizza.Dishes.AddRange(new[] { "margherita", "marinara" });
            Category white = new Category { Name = "white" };
            white.Dishes.Add("bianca");
            pizza.Subsections.Add(white);
            menu.Sections.Add(pizza);

            Menu got = Protaco.Codec.ToObject<Menu>(menu.Serialize());
            Console.WriteLine("Menu: default size " + PizzaSizeNames.Name(got.DefaultSize) + ", margherita " + got.Prices["margherita"]);
            Console.WriteLine(got.ToString());
        }

        // 6. Zero copy: TryView reads a fixed-only frame in place.
        private static void ZeroCopy()
        {
            byte[] received = new Heartbeat { Node = "oven-1", UptimeSec = 42 }.Serialize();
            Heartbeat.View view;   // valid while received is unchanged
            if (Heartbeat.TryView(received, out view))
                Console.WriteLine("Heartbeat view: node=" + view.Node + " uptime=" + view.UptimeSec);
        }

        // 7. Envelope: a new message gets an id and the current time; a reply names the request in corr.
        private static void Envelope()
        {
            DeliveryRoute request = new DeliveryRoute();
            DeliveryRoute reply = new DeliveryRoute();
            reply.SetCorr(request.GetId());
            string date;
            string time;
            if (reply.TryGetTimeFromTimestamp(out date, out time))
                Console.WriteLine("Envelope: request id=" + request.GetId() + ", reply id=" + reply.GetId() + " corr=" + reply.GetCorr() + " at " + date + " " + time);
        }

        private static int Main()
        {
            SendAndReceive();
            ExtraData();
            ErrorHandling();
            Logging();
            EnumsMapsAndTrees();
            ZeroCopy();
            Envelope();
            return 0;
        }
    }
}
