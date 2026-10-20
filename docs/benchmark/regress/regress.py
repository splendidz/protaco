# regress.py <runtime folder> <generated Python root>. Regression check of the Python runtime: the cases of
# regress.cpp, run once with an older commit and once with the working tree (regress.cmd). Prints "name ns" per case.
import sys
import timeit

sys.path.insert(0, sys.argv[1])
sys.path.insert(0, sys.argv[2])

import protaco  # noqa: E402
from bench.Fixed512_g import Fixed512  # noqa: E402
from bench.Mixed256_g import Mixed256  # noqa: E402
from demo.DeliveryRoute_g import DeliveryRoute  # noqa: E402
from demo.Heartbeat_g import Heartbeat  # noqa: E402
from demo.RouteStop_g import RouteStop  # noqa: E402


def make_route(v):
    route = DeliveryRoute(rider="rider-7", note="closest-first")
    route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St", etaMin=v & 63))
    route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St", etaMin=v & 63))
    return route


# Median of the per-repeat ns per call.
def measure(call, number=2000, repeat=15):
    times = sorted(timeit.repeat(call, number=number, repeat=repeat))
    return times[len(times) // 2] / number * 1e9


def main():
    fixed = Fixed512(name="sensor-node-0001", tag="site=north;zone=a;rack=12;unit=03;rev=07")
    fixed_bytes = fixed.Serialize()
    mixed = Mixed256(name="sensor-node-0001-line-07", note="calibrated today", code="QC-PASS-2026")
    mixed_bytes = mixed.Serialize()
    route_bytes = make_route(1).Serialize()
    route = protaco.ToObject(DeliveryRoute, route_bytes)
    cases = (
        ("Fixed512.encode", fixed.Serialize),
        ("Fixed512.decode", lambda: protaco.ToObject(Fixed512, fixed_bytes)),
        ("Mixed256.encode", mixed.Serialize),
        ("Mixed256.decode", lambda: protaco.ToObject(Mixed256, mixed_bytes)),
        ("DeliveryRoute.new+serialize", lambda: make_route(5).Serialize()),
        ("DeliveryRoute.decode", lambda: protaco.ToObject(DeliveryRoute, route_bytes)),
        ("DeliveryRoute.tostring", route.ToString),
        ("Heartbeat.new", Heartbeat),
    )
    for name, call in cases:
        print("%s %.2f" % (name, measure(call)))


if __name__ == "__main__":
    main()
