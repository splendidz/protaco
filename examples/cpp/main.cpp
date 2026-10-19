// main.cpp. How to use protacoc generated code.
// Steps: include the generated header, set values and call Serialize(), receiver calls protaco::ToObject<T>(bytes).
#include <cstdio>
#include <string>
#include <vector>

#include "DeliveryRoute.g.h"
#include "Heartbeat.g.h"
#include "Menu.g.h"
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
    route.mutable_stops().push_back(stop);
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
    order.mutable_toppings().push_back(basil);
    order.GetMutableExt().SetValue("secretSauce", "yes please");   // key not in the yaml
    order.GetMutableExt().SetValue("pineapple", 0);

    const demo::PizzaOrder got = protaco::ToObject<demo::PizzaOrder>(order.Serialize());
    std::string sauce;
    got.GetExt().TryGetValue("secretSauce", &sauce);
    std::printf("PizzaOrder ext: secretSauce=%s, schema key in ext=%s\n",
        sauce.c_str(), order.GetMutableExt().SetValue("orderId", 9) ? "accepted (bug)" : "rejected");
}

// 4. Errors: TryToObject never throws.
void ErrorHandling()
{
    const std::vector<uint8_t> bytes = demo::Heartbeat().Serialize();
    demo::DeliveryRoute wrong;
    std::string error;
    if (!protaco::TryToObject(bytes.data(), bytes.size(), &wrong, &error))
        std::printf("decode failed: %s\n", error.c_str());
}

// 5. Logs: ToString shows every field, ext entries under ext{...}.
void Logging()
{
    demo::DeliveryRoute route;
    route.set_rider("rider-7");
    demo::RouteStop stop;
    stop.set_orderId("A-100");
    stop.set_address("1 Pizza St");
    route.mutable_stops().push_back(stop);
    route.GetMutableExt().SetValue("priority", 5);
    std::printf("%s\n", route.ToString().c_str());
    std::printf("%s\n", route.ToString(protaco::TextStyle::Pretty()).c_str());
}

// 6. Enums, maps and a message that holds itself (a tree of menu sections).
void EnumsMapsAndTrees()
{
    demo::Menu menu;
    menu.set_store("NAPOLI-1");
    menu.mutable_prices()["margherita"] = 9.5;
    menu.mutable_prices()["marinara"] = 8.0;
    demo::Category pizza;
    pizza.set_name("pizza");
    pizza.mutable_dishes() = { "margherita", "marinara" };
    demo::Category white;
    white.set_name("white");
    white.mutable_dishes().push_back("bianca");
    pizza.mutable_subsections().push_back(white);
    menu.mutable_sections().push_back(pizza);

    const demo::Menu got = protaco::ToObject<demo::Menu>(menu.Serialize());
    std::printf("Menu: default size %s, margherita %.1f\n", demo::EnumName(got.get_defaultSize()), got.get_prices().at("margherita"));
    std::printf("%s\n", got.ToString().c_str());
}

// 7. Zero copy: TryView reads a fixed-only frame in place; ToObject with std::move takes the buffer.
void ZeroCopy()
{
    demo::Heartbeat beat;
    beat.set_node("oven-1");
    beat.set_uptimeSec(42);
    const std::vector<uint8_t> received = beat.Serialize();
    demo::Heartbeat::View view;   // valid while received lives
    if (protaco::TryView<demo::Heartbeat>(received, &view))
        std::printf("Heartbeat view: node=%s uptime=%d\n", view.get_node().c_str(), view.get_uptimeSec());

    std::vector<uint8_t> bytes = demo::DeliveryRoute().Serialize();
    const uint8_t* data = bytes.data();
    const demo::DeliveryRoute route = protaco::ToObject<demo::DeliveryRoute>(std::move(bytes));
    std::printf("DeliveryRoute adopted the buffer: %s\n", route.Serialize().data() == data ? "yes" : "no");
}

// 8. Envelope: a new message gets an id and the current time; a reply names the request in corr.
void Envelope()
{
    demo::DeliveryRoute request;
    demo::DeliveryRoute reply;
    reply.SetCorr(request.GetId());
    std::string date;
    std::string time;
    if (reply.TryGetTimeFromTimestamp(&date, &time))
        std::printf("Envelope: request id=%u, reply id=%u corr=%u at %s %s\n", request.GetId(), reply.GetId(), reply.GetCorr(),
            date.c_str(), time.c_str());
}

}  // namespace

int main()
{
    SendAndReceive();
    FixedOnly();
    ExtraData();
    ErrorHandling();
    Logging();
    EnumsMapsAndTrees();
    ZeroCopy();
    Envelope();
    return 0;
}
