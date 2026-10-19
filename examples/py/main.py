# main.py. How to use protacoc generated Python code. Same steps as cpp/main.cpp.
# Steps: import the generated module, set values and call Serialize(), receiver calls protaco.ToObject(type, data).
# Run: python examples/py/main.py (generated packages and protaco.py in examples/gen/py).
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "gen", "py"))

import protaco  # noqa: E402
from demo.DeliveryRoute_g import DeliveryRoute  # noqa: E402
from demo.Heartbeat_g import Heartbeat  # noqa: E402
from demo.PizzaOrder_g import PizzaOrder  # noqa: E402
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


if __name__ == "__main__":
    send_and_receive()
    fixed_only()
    extra_data()
    error_handling()
    logging()
