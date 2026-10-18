// Cpp14Check.cpp. Generated headers and the runtime must compile as C++14.
// Only this file is built with /std:c++14 (ProtacoTest.vcxproj). Real calls force template instantiation.
#include "Heartbeat.g.h"
#include "PizzaOrder.g.h"
#include "TransferRequest.g.h"

static_assert(_MSVC_LANG == 201402L, "must be compiled as C++14");

using namespace demo;

bool Cpp14RoundTrip()
{
    TransferRequest request;
    request.set_tm("TM1");
    request.get_mutable_moves().push_back(MoveInfo());
    request.get_mutable_ext().SetInt("priority", 5);
    PizzaOrder order;
    order.get_mutable_toppings().push_back(Topping());
    order.get_mutable_delivery().get_mutable_doorCode().push_back(1);
    Heartbeat beat;
    TransferRequest requestBack;
    PizzaOrder orderBack;
    Heartbeat beatBack;
    return protaco::TryToObject(request.Serialize().data(), request.Serialize().size(), &requestBack)
        && protaco::TryToObject(order.Serialize().data(), order.Serialize().size(), &orderBack)
        && protaco::TryToObject(beat.Serialize().data(), beat.Serialize().size(), &beatBack)
        && !requestBack.ToString().empty() && !orderBack.ToString(protaco::TextStyle::Pretty()).empty()
        && !beatBack.ToString().empty();
}
