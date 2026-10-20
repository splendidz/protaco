# main.py. How to use protacoc generated Python code. Same steps as cpp_examples/main.cpp.
# Steps: import the generated module, set values and call Serialize(), receiver calls protaco.ToObject(type, data).
# Run: python examples/python_examples/main.py (generated packages and protaco.py in examples/taco_generated/py).
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "taco_generated", "py"))

import protaco  # noqa: E402
from demo.Category_g import Category  # noqa: E402
from demo.DeliveryRoute_g import DeliveryRoute  # noqa: E402
from demo.Heartbeat_g import Heartbeat  # noqa: E402
from demo.Menu_g import Menu  # noqa: E402
from demo.PizzaOrder_g import PizzaOrder  # noqa: E402
from demo.PizzaSize_enum import PizzaSizeNames  # noqa: E402
from demo.RouteStop_g import RouteStop  # noqa: E402
from demo.Topping_g import Topping  # noqa: E402


# 1. Basics: send and receive a message with a list of nested messages.
def send_and_receive():
    route = DeliveryRoute(rider="rider-7", note="closest-first")
    route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St", etaMin=10))

    data = route.Serialize()   # send these bytes
    print("DeliveryRoute: %d bytes" % len(data))

    got = protaco.ToObject(DeliveryRoute, data)
    stop = got.stops[0]
    print("  rider=%s first=%s at %s in %d min, note=%s" % (got.rider, stop.orderId, stop.address, stop.etaMin, got.note))


# 2. Fixed fields only: one struct pack on send, one unpack on receive.
def fixed_only():
    beat = Heartbeat(node="oven-1", uptimeSec=10)
    print("Heartbeat: received uptime=%d" % protaco.ToObject(Heartbeat, beat.Serialize()).uptimeSec)


# 3. Extra data without touching the yaml (ext). Receivers keep keys they do not know.
def extra_data():
    order = PizzaOrder(orderId="A-100")
    order.toppings.append(Topping(name="basil"))
    order.ext.SetValue("secretSauce", "yes please")   # key not in the yaml
    order.ext.SetValue("pineapple", 0, protaco.INT)   # an int needs its kind

    got = protaco.ToObject(PizzaOrder, order.Serialize())
    sauce = got.ext.TryGetValue("secretSauce", protaco.STRING)
    print("PizzaOrder ext: secretSauce=%s, schema key in ext=%s"
          % (sauce.value, "accepted (bug)" if order.ext.SetValue("orderId", 9, protaco.INT) else "rejected"))


# 4. Errors: TryToObject never raises.
def error_handling():
    ok, _, error = protaco.TryToObject(DeliveryRoute, Heartbeat().Serialize())
    if not ok:
        print("decode failed: " + error)


# 5. Logs: ToString shows every field, ext entries under ext{...}.
def logging():
    route = DeliveryRoute(rider="rider-7")
    route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St"))
    route.ext.SetValue("priority", 5, protaco.INT)
    print(route)
    print(route.ToString(protaco.TextStyle.Pretty()))


# 6. Enums, maps and a message that holds itself (a tree of menu sections).
def enums_maps_and_trees():
    menu = Menu(store="NAPOLI-1", prices={"margherita": 9.5, "marinara": 8.0})
    pizza = Category(name="pizza", dishes=["margherita", "marinara"])
    pizza.subsections.append(Category(name="white", dishes=["bianca"]))
    menu.sections.append(pizza)

    got = protaco.ToObject(Menu, menu.Serialize())
    print("Menu: default size %s, margherita %.1f" % (PizzaSizeNames.Name(got.defaultSize), got.prices["margherita"]))
    print(got)


# 7. Envelope: a new message gets an id and the current time; a reply names the request in corr.
def envelope():
    request = DeliveryRoute()
    reply = DeliveryRoute()
    reply.SetCorr(request.GetId())
    when = reply.TryGetTimeFromTimestamp()
    if when:
        date, time_text = when.value
        print("Envelope: request id=%d, reply id=%d corr=%d at %s %s" % (request.GetId(), reply.GetId(), reply.GetCorr(), date, time_text))


if __name__ == "__main__":
    send_and_receive()
    fixed_only()
    extra_data()
    error_handling()
    logging()
    enums_maps_and_trees()
    envelope()
