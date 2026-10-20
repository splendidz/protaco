// Cpp14Check.cpp. Generated headers and the runtime must compile as C++14.
// Only this file is built as C++14 (ProtacoTest.vcxproj /std:c++14, build.sh -std=c++14). Real calls force template
// instantiation: maps, enums, recursive messages, views and adopted buffers included.
#include <algorithm>
#include <utility>

#include "DeliveryRoute.g.h"
#include "Heartbeat.g.h"
#include "PizzaOrder.g.h"
#include "arrays/Sizes.const.h"
#include "enums/Pie.g.h"
#include "maps/Menu.g.h"
#include "recursive/Node.g.h"

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG == 201402L, "must be compiled as C++14");
#else
static_assert(__cplusplus == 201402L, "must be compiled as C++14");
#endif

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
    Heartbeat::View view;
    maps::Menu menu;
    menu.mutable_prices()["a"] = 1.5;
    menu.mutable_specials()[maps::Grade::Gold].set_name("x");
    recursive::Node node;
    node.mutable_next().mutable_children().push_back(recursive::Node());
    enums::Pie pie;
    pie.set_size(enums::PizzaSize::Medium);
    std::vector<uint8_t> nodeBytes = node.Serialize();
    const recursive::Node nodeBack = protaco::ToObject<recursive::Node>(std::move(nodeBytes));
    return protaco::TryToObject(route.Serialize().data(), route.Serialize().size(), &routeBack)
        && protaco::TryToObject(order.Serialize().data(), order.Serialize().size(), &orderBack)
        && protaco::TryToObject(beat.Serialize().data(), beat.Serialize().size(), &beatBack)
        && protaco::TryView<Heartbeat>(beat.Serialize(), &view) && view.get_healthy()
        && protaco::ToObject<maps::Menu>(menu.Serialize()).get_specials().at(maps::Grade::Gold).get_name() == "x"
        && nodeBack.get_next().get_children().size() == 1
        && protaco::ToObject<enums::Pie>(pie.Serialize()).get_size() == enums::PizzaSize::Medium
        && !routeBack.ToString().empty() && !orderBack.ToString(protaco::TextStyle::Pretty()).empty()
        && !beatBack.ToString().empty()
        && std::max(arrays::Sizes::Slots, arrays::Sizes::Names) == 4;
}
