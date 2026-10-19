// Cpp14Check.cpp. Generated headers and the runtime must compile as C++14.
// Only this file is built with /std:c++14 (ProtacoTest.vcxproj). Real calls force template instantiation.
#include "DeliveryRoute.g.h"
#include "Heartbeat.g.h"
#include "PizzaOrder.g.h"

static_assert(_MSVC_LANG == 201402L, "must be compiled as C++14");

using namespace demo;

bool Cpp14RoundTrip()
{
    DeliveryRoute route;
    route.set_rider("rider-7");
    route.mutable_stops().push_back(RouteStop());
    route.GetMutableExt().SetValue("priority", 5);
    PizzaOrder order;
    order.mutable_toppings().push_back(Topping());
    order.mutable_delivery().mutable_doorCode().push_back(1);
    Heartbeat beat;
    DeliveryRoute routeBack;
    PizzaOrder orderBack;
    Heartbeat beatBack;
    return protaco::TryToObject(route.Serialize().data(), route.Serialize().size(), &routeBack)
        && protaco::TryToObject(order.Serialize().data(), order.Serialize().size(), &orderBack)
        && protaco::TryToObject(beat.Serialize().data(), beat.Serialize().size(), &beatBack)
        && !routeBack.ToString().empty() && !orderBack.ToString(protaco::TextStyle::Pretty()).empty()
        && !beatBack.ToString().empty();
}
