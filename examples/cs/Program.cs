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

            byte[] bytes = route.Serialize();   // same bytes as the C++ example
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

        private static int Main()
        {
            SendAndReceive();
            ExtraData();
            ErrorHandling();
            Logging();
            return 0;
        }
    }
}
