// main.cpp. How to use protacoc generated code.
// Steps: include the generated header, set values and call Serialize(), receiver calls protaco::ToObject<T>(bytes).
#include <cstdio>
#include <string>
#include <vector>

#include "DeliveryRoute.g.h"
#include "Heartbeat.g.h"
#include "PizzaOrder.g.h"

namespace
{

// 1. Basics: send and receive a message with a list of nested messages.
void SendAndReceive()
{
    demo::DeliveryRoute route;
    route.set_rider("rider-7");
    demo::RouteStop stop;
    stop.set_orderId("A-100");
    stop.set_address("1 Pizza St");
    stop.set_etaMin(10);
    route.get_mutable_stops().push_back(stop);
    route.set_note("closest-first");

    const std::vector<uint8_t>& bytes = route.Serialize();   // send bytes.data(), bytes.size()
    std::printf("DeliveryRoute: %zu bytes\n", bytes.size());

    const demo::DeliveryRoute got = protaco::ToObject<demo::DeliveryRoute>(bytes);
    std::printf("  rider=%s first=%s at %s in %d min, note=%s\n",
        got.get_rider().c_str(), got.get_stops()[0].get_orderId().c_str(), got.get_stops()[0].get_address().c_str(),
        got.get_stops()[0].get_etaMin(), got.get_note().c_str());
}

// 2. Fixed fields only: setters already serialize, Serialize() returns the buffer as is.
void FixedOnly()
{
    demo::Heartbeat beat;
    beat.set_node("oven-1");
    beat.set_uptimeSec(10);
    const std::vector<uint8_t>& bytes = beat.Serialize();
    beat.set_uptimeSec(11);   // written into the same buffer, no need to call Serialize again
    std::printf("Heartbeat: received uptime=%d, same buffer=%s\n",
        protaco::ToObject<demo::Heartbeat>(bytes).get_uptimeSec(), &bytes == &beat.Serialize() ? "yes" : "no");
}

// 3. Extra data without touching the yaml (ext). Receivers keep keys they do not know.
void ExtraData()
{
    demo::PizzaOrder order;
    order.set_orderId("A-100");
    demo::Topping basil;
    basil.set_name("basil");
    order.get_mutable_toppings().push_back(basil);
    order.GetMutableExt().SetString("secretSauce", "yes please");   // key not in the yaml
    order.GetMutableExt().SetInt("pineapple", 0);

    const demo::PizzaOrder got = protaco::ToObject<demo::PizzaOrder>(order.Serialize());
    std::string sauce;
    got.GetExt().GetString("secretSauce", &sauce);
    std::printf("PizzaOrder ext: secretSauce=%s, schema key in ext=%s\n",
        sauce.c_str(), order.GetMutableExt().SetInt("orderId", 9) ? "accepted (bug)" : "rejected");
}

// 4. Errors: TryToObject never throws.
void ErrorHandling()
{
    const std::vector<uint8_t> bytes = demo::Heartbeat().Serialize();
    demo::DeliveryRoute wrong;
    protaco::Error error;
    if (!protaco::TryToObject(bytes.data(), bytes.size(), &wrong, &error))
        std::printf("decode failed: path='%s' what='%s'\n", error.path.c_str(), error.what.c_str());
}

// 5. Logs: ToString shows every field, ext entries under ext{...}.
void Logging()
{
    demo::DeliveryRoute route;
    route.set_rider("rider-7");
    demo::RouteStop stop;
    stop.set_orderId("A-100");
    stop.set_address("1 Pizza St");
    route.get_mutable_stops().push_back(stop);
    route.GetMutableExt().SetInt("priority", 5);
    std::printf("%s\n", route.ToString().c_str());
    std::printf("%s\n", route.ToString(protaco::TextStyle::Pretty()).c_str());
}

}  // namespace

int main()
{
    SendAndReceive();
    FixedOnly();
    ExtraData();
    ErrorHandling();
    Logging();
    return 0;
}
