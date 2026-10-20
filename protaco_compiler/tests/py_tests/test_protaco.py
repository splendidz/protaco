# test_protaco.py. Tests for the Python runtime (include/python/protaco.py) and protacoc generated Python code.
# Run: python test_protaco.py <tests folder> (protaco_compiler/tests). Generated packages: <tests folder>/taco_generated/py.
# Same values and texts as the C++ (ProtacoTest.cpp) and C# (Program.cs) tests; golden files come from C++.
import datetime
import os
import struct
import sys
import unittest

TESTS = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(TESTS, "..", "..", "include", "python"))
sys.path.insert(0, os.path.join(TESTS, "taco_generated", "py"))

import protaco  # noqa: E402
from alt.Recipe_g import Recipe as AltRecipe  # noqa: E402
from arrays.Level_enum import Level  # noqa: E402
from arrays.Panel_g import Panel  # noqa: E402
from arrays.Point_g import Point  # noqa: E402
from arrays.Rack_g import Rack  # noqa: E402
from arrays.Sizes_const import Sizes  # noqa: E402
from arrays.Trail_g import Trail  # noqa: E402
from build1.Order_g import Order as Order1  # noqa: E402
from build1.Ping_g import Ping as Ping1  # noqa: E402
from build2.Order_g import Order as Order2  # noqa: E402
from build2.Ping_g import Ping as Ping2  # noqa: E402
from build3.Order_g import Order as Order3, v1 as Order3Old  # noqa: E402
from build4.Order_g import Order as Order4  # noqa: E402
from deep.D8_g import D8  # noqa: E402
from demo.DeliveryProof_g import DeliveryProof  # noqa: E402
from demo.DeliveryRoute_g import DeliveryRoute  # noqa: E402
from demo.Heartbeat_g import Heartbeat  # noqa: E402
from demo.PizzaOrder_g import PizzaOrder  # noqa: E402
from demo.RouteStop_g import RouteStop  # noqa: E402
from demo.Topping_g import Topping  # noqa: E402
from enums.Pie_g import Pie  # noqa: E402
from enums.PizzaSize_enum import PizzaSize, PizzaSizeNames  # noqa: E402
from maps.Dish_g import Dish  # noqa: E402
from maps.Grade_enum import Grade  # noqa: E402
from maps.Menu_g import Menu  # noqa: E402
from partial.DeliveryRoute_g import DeliveryRoute as PartialRoute  # noqa: E402
from partial.PizzaOrder_g import PizzaOrder as PartialOrder  # noqa: E402
from partial.Topping_g import Topping as PartialTopping  # noqa: E402
from recursive.Node_g import Node  # noqa: E402
from scalars.Move_g import Move  # noqa: E402
from scalars.Numbers_g import Numbers  # noqa: E402
from scalars.Tick_g import Tick  # noqa: E402
from v1.Recipe_g import Recipe as Recipe1  # noqa: E402
from v2.Recipe_g import Recipe as Recipe2  # noqa: E402


def golden(name):
    with open(os.path.join(TESTS, "golden", name), "rb") as f:
        return f.read()


# Rewrites the fingerprint of the frame's own version so a frame from another yaml reaches the body decoder.
def with_schema_hash(frame, schema_hash):
    frame = bytearray(frame)
    version, min_version = struct.unpack_from("<HH", frame, 4 + frame[3])
    struct.pack_into("<I", frame, 28 + frame[3] + 4 * (version - min_version), schema_hash)
    return bytes(frame)


# Appends one int TLV to the body and fixes BodyLen.
def with_extra_tlv(frame, key, value):
    frame = bytearray(frame)
    frame += bytes((len(key),)) + key.encode() + bytes((protaco.INT,)) + struct.pack("<Ii", 4, value)
    at = 24 + frame[3]
    struct.pack_into("<I", frame, at, struct.unpack_from("<I", frame, at)[0] + 1 + len(key) + 1 + 4 + 4)
    return bytes(frame)


def make_delivery_route():
    route = DeliveryRoute(rider="rider-7", note="closest-first")
    route.stops.append(RouteStop(orderId="A-100", address="1 Pizza St", etaMin=10))
    route.stops.append(RouteStop(orderId="A-101", address="2 Pasta Ave", etaMin=25))
    route.ext.SetValue("priority", 5, protaco.INT)
    route.SetEnvelope(protaco.Envelope(7, 0, 1000))
    return route


def make_pizza_order():
    order = PizzaOrder(orderId="A-100", store="NAPOLI-1", customer="mario", size=14, woodFired=True)
    order.toppings.append(Topping(name="basil", grams=5))
    order.toppings.append(Topping(name="mozzarella"))
    order.notes.append("no onion")
    order.delivery.address = "1 Pizza St"
    order.delivery.floor = 3
    order.delivery.doorCode.extend([4, 2, 4, 2])
    order.delivery.contacts.append("010-0000-0000")
    order.SetEnvelope(protaco.Envelope())   # golden bytes: no id, no time
    return order


def make_heartbeat():
    beat = Heartbeat(node="oven-1", seq=123456789, cpuLoad=0.25, uptimeSec=3600)
    beat.SetEnvelope(protaco.Envelope())
    return beat


def make_delivery_proof():
    proof = DeliveryProof(orderId="A-1001", photo=b"\xFF\xD8\x00\x10JFIF\x00\xFF\xD9")
    proof.signatures.extend([b"\x00", b"", b"\x01\x02"])
    proof.SetEnvelope(protaco.Envelope())
    return proof


def make_numbers():
    numbers = Numbers(u32=0, name="한글 ✓", text="café 寿司")
    numbers.us.extend([0, 1, 4294967295])
    numbers.u64s.append(18446744073709551615)
    numbers.SetEnvelope(protaco.Envelope())
    return numbers


def make_pie():
    pie = Pie(size=PizzaSize.Family, crust=9)
    pie.sizes.extend([PizzaSize.Small, PizzaSize.Family, 5, 9])
    pie.SetEnvelope(protaco.Envelope())
    return pie


def make_panel():
    panel = Panel(total=42)
    panel.temps = [20.5 + i for i in range(4)]
    panel.counts = [1, -2, 3, -4]
    panel.ids = [1, 18446744073709551615, 7]
    panel.masks[1] = 4294967295
    panel.ticks[0] = -9000000000
    panel.flags[1] = True
    panel.names[0] = "alpha"
    panel.levels[2] = Level.High
    panel.SetEnvelope(protaco.Envelope())
    return panel


def make_rack():
    rack = Rack(count=4)
    rack.origin.x = 3
    for i, slot in enumerate(rack.slots):
        slot.id = 100 + i
        slot.pos.y = i
    rack.slots[1].name = "one"
    rack.slots[2].path[1].x = 7
    rack.slots[3].level = Level.High
    rack.slots[3].temps[1] = 1.5
    rack.SetEnvelope(protaco.Envelope())
    return rack


def make_trail():
    trail = Trail(note="n")
    trail.start.y = 5
    trail.points.append(Point(x=3, y=0))
    trail.SetEnvelope(protaco.Envelope())
    return trail


def make_menu():
    menu = Menu(title="lunch")
    menu.prices["dupA"] = 9.5
    menu.stock[-1] = 4294967295
    menu.specials[Grade.Silver] = Dish(name="ragu", price=12.25)
    menu.notes[18446744073709551615] = "매운맛"
    menu.vegan["ragu"] = False
    menu.photos["ragu"] = b"\xFF\x00"
    menu.grades["ragu"] = 9
    menu.tags["ragu"] = ["hot", ""]
    menu.daily[20261020] = {"ragu": 3}
    menu.SetEnvelope(protaco.Envelope())
    return menu


def make_tree():
    root = Node(name="root", value=1)
    root.children.append(Node(name="a", value=2, next=Node(name="b", value=3)))
    root.children.append(Node(name="c", value=4))
    root.SetEnvelope(protaco.Envelope())
    return root


def make_order_v3():
    order = Order3(id="A1", weight=2.5, color="red", label="L", rush=True)
    order.tags.append("x")
    return order


MENU_ONE = ('Menu{prices={"dupA": 9.5}, stock={-1: 4294967295}, specials={Silver(2): Dish{name="ragu", price=12.25}}, '
            'notes={18446744073709551615: "매운맛"}, vegan={"ragu": false}, photos={"ragu": <2 bytes: FF 00>}, '
            'grades={"ragu": ?(9)}, tags={"ragu": ["hot", ""]}, daily={20261020: {"ragu": 3}}, title="lunch"}')
NODE_TEXT = ('Node{name="root", value=1, children=[Node{name="a", value=2, children=[], next=Node{name="b", value=3, children=[], next=null}}, '
             'Node{name="c", value=4, children=[], next=null}], next=null}')


class ProtacoTest(unittest.TestCase):
    # Decoding must fail. TryToObject reports "path: reason"; ToObject raises the same failure split into path and what.
    def fails(self, cls, data):
        result = protaco.TryToObject(cls, data)
        self.assertFalse(result, cls.__name__ + " accepted")
        with self.assertRaises(protaco.ProtacoError) as caught:
            protaco.ToObject(cls, data)
        self.assertEqual(result.error, str(caught.exception))
        return caught.exception

    def test_round_trip(self):
        data = make_delivery_route().Serialize()
        got = protaco.ToObject(DeliveryRoute, data)
        self.assertEqual("rider-7", got.rider)
        self.assertEqual(2, len(got.stops))
        self.assertEqual("1 Pizza St", got.stops[0].address)
        ok, priority, _ = got.ext.TryGetValue("priority", protaco.INT)
        self.assertTrue(ok)
        self.assertEqual(5, priority)
        self.assertEqual(data, got.Serialize())

    def test_same_bytes_as_cpp_golden(self):
        self.assertEqual(golden("DeliveryRoute.bin"), make_delivery_route().Serialize())
        self.assertEqual(golden("PizzaOrder.bin"), make_pizza_order().Serialize())
        self.assertEqual(golden("Heartbeat.bin"), make_heartbeat().Serialize())
        self.assertEqual(golden("DeliveryProof.bin"), make_delivery_proof().Serialize())
        self.assertEqual(golden("Numbers.bin"), make_numbers().Serialize())
        self.assertEqual(golden("Pie.bin"), make_pie().Serialize())
        self.assertEqual(golden("Menu.bin"), make_menu().Serialize())
        self.assertEqual(golden("Node.bin"), make_tree().Serialize())

    def test_reads_cpp_golden(self):
        route = protaco.ToObject(DeliveryRoute, golden("DeliveryRoute.bin"))
        self.assertEqual(7, route.GetId())
        self.assertEqual(1000, route.GetTimestamp())
        self.assertEqual("2 Pasta Ave", route.stops[1].address)
        order = protaco.ToObject(PizzaOrder, golden("PizzaOrder.bin"))
        self.assertEqual(5, order.toppings[0].grams)
        self.assertEqual(2, order.delivery.doorCode[3])
        self.assertTrue(order.woodFired)
        proof = protaco.ToObject(DeliveryProof, golden("DeliveryProof.bin"))
        self.assertEqual(make_delivery_proof().photo, proof.photo)
        self.assertEqual([b"\x00", b"", b"\x01\x02"], proof.signatures)
        self.assertTrue(proof.ext.SetValue("thumbnail", b"\x00\xFF"))
        again = protaco.ToObject(DeliveryProof, proof.Serialize())
        self.assertEqual(b"\x00\xFF", again.ext.TryGetValue("thumbnail", protaco.BYTES).value)

    def test_version_mismatch(self):
        self.assertEqual("message version too new: Recipe reads version 1, sender supports 2..2",
                         self.fails(Recipe1, Recipe2(path="/a").Serialize()).what)
        self.assertEqual("message version too old: Recipe reads version 2 or newer, got 1",
                         self.fails(Recipe2, Recipe1(path="/b").Serialize()).what)

    def test_schema_mismatch(self):
        self.assertNotEqual(AltRecipe.MsgSchemaHash, Recipe1.MsgSchemaHash)
        data = AltRecipe(step=5, name="R1", path="/a").Serialize()
        self.assertEqual("schema mismatch: Recipe version 1 expected 0x%08X, got 0x%08X" % (Recipe1.MsgSchemaHash, AltRecipe.MsgSchemaHash),
                         self.fails(Recipe1, data).what)

    def test_new_version_keeps_old_fingerprints(self):
        self.assertEqual(Order1.MsgSchemaHash, Order3Old.Order.MsgSchemaHash)
        self.assertEqual(Order2.MsgSchemaHash, Order3._SCHEMA.hashes[1])

    def test_newer_frame_read_as_own_version(self):
        data = make_order_v3().Serialize()
        reader1 = protaco.ToObject(Order1, data)
        self.assertEqual("A1", reader1.id)
        self.assertEqual(1, reader1.qty)
        self.assertEqual("", reader1.note)
        self.assertEqual("red", reader1.color)
        self.assertEqual(["label", "tags"], sorted(reader1.ext.Keys()))
        reader2 = protaco.ToObject(Order2, data)
        self.assertEqual(2.5, reader2.weight)
        self.assertEqual(["x"], reader2.tags)
        self.assertEqual(data, protaco.ToObject(Order3, data).Serialize())

    def test_older_frame_needs_old_type(self):
        data = Order1(id="B2", qty=4, note="n", color="blue").Serialize()
        self.assertEqual("message version too old: Order reads version 3 or newer, got 1. Use the v1 type", self.fails(Order3, data).what)
        got = protaco.ToObject(Order3Old.Order, data)
        self.assertEqual(4, got.qty)
        self.assertEqual('Order{id="B2", qty=4, note="n", color="blue"}', got.ToString())

    def test_sender_min_version_above_reader(self):
        data = Order4(id="C3").Serialize()
        self.assertEqual("message version too new: Order reads version 1, sender supports 2..4", self.fails(Order1, data).what)
        error = self.fails(Order2, data)
        self.assertEqual("color", error.path)
        self.assertEqual("missing required (frame version 4 read as 2)", error.what)

    def test_closed_message_drops_newer_fields(self):
        got = protaco.ToObject(Ping1, Ping2(seq=7, who="w", text="hi").Serialize())
        self.assertEqual(7, got.seq)
        self.assertEqual("unknown key (allow_extension false)", self.fails(Ping1, with_extra_tlv(got.Serialize(), "text", 1)).what)

    def test_closed_message_rejects_unknown_key(self):
        error = self.fails(Heartbeat, with_extra_tlv(make_heartbeat().Serialize(), "abc", 7))
        self.assertEqual("abc", error.path)
        self.assertEqual("unknown key (allow_extension false)", error.what)
        opened = protaco.ToObject(DeliveryRoute, with_extra_tlv(make_delivery_route().Serialize(), "abc", 7))
        self.assertEqual(7, opened.ext.TryGetValue("abc", protaco.INT).value)
        self.assertFalse(make_heartbeat().ext.SetValue("abc", 1, protaco.INT))

    def test_errors_have_paths(self):
        error = self.fails(DeliveryRoute, with_schema_hash(PartialRoute(rider="rider-7").Serialize(), DeliveryRoute.MsgSchemaHash))
        self.assertEqual("stops", error.path)
        self.assertEqual("missing required", error.what)
        sender = PartialOrder()
        sender.toppings.append(PartialTopping())
        sender.delivery.doorCode.append("x")
        error = self.fails(PizzaOrder, with_schema_hash(sender.Serialize(), PizzaOrder.MsgSchemaHash))
        self.assertEqual("delivery.doorCode", error.path)
        self.assertEqual("list element type mismatch", error.what)
        data = bytearray(Recipe1(path="/p").Serialize())
        data[4 + data[3]] = 2   # version 1 -> 2
        data[6 + data[3]] = 2   # min version 2, still one fingerprint
        data = with_schema_hash(bytes(data), Recipe2.MsgSchemaHash)
        self.assertEqual("fixed block size mismatch: expected 28, got 20", self.fails(Recipe2, data).what)

    def test_eight_nested_levels(self):
        sent = D8()
        sent.d.d.d.d.d.d.d.d.v = 42
        got = protaco.ToObject(D8, sent.Serialize())
        self.assertEqual(42, got.d.d.d.d.d.d.d.d.v)
        self.assertEqual("D8{d=D7{d=D6{d=D5{d=D4{d=D3{d=D2{d=D1{d=D0{v=42}}}}}}}}}", got.ToString())

    def test_every_truncation_fails(self):
        data = make_pizza_order().Serialize()
        for length in range(len(data)):
            result = protaco.TryToObject(PizzaOrder, data[:length])
            self.assertFalse(result, "accepted at length %d" % length)
            self.assertTrue(result.error)

    def test_values_that_do_not_fit(self):
        stop = RouteStop(orderId="x" * 17)
        with self.assertRaises(protaco.ProtacoError) as caught:
            stop.Serialize()
        self.assertEqual("orderId: 17 bytes, longer than string(16)", str(caught.exception))
        numbers = Numbers(u32=-1)
        with self.assertRaises(protaco.ProtacoError) as caught:
            numbers.Serialize()
        self.assertEqual("u32: -1 does not fit uint", str(caught.exception))
        numbers = Numbers()
        numbers.us.append(1 << 32)
        with self.assertRaises(protaco.ProtacoError) as caught:
            numbers.Serialize()
        self.assertEqual("us[0]: 4294967296 does not fit uint", str(caught.exception))
        with self.assertRaises(TypeError):
            Numbers(nothing=1)

    def test_ext_rules(self):
        route = DeliveryRoute()
        self.assertEqual("ext key is a field of the message: rider", route.ext.SetValue("rider", 1, protaco.INT).error)
        self.assertEqual("ext key starts with '_' (reserved): _x", route.ext.SetValue("_x", 1, protaco.INT).error)
        self.assertEqual("ext key is empty", route.ext.SetValue("", 1, protaco.INT).error)
        self.assertEqual("ext value 1 needs a kind: INT, INT64, UINT or UINT64", route.ext.SetValue("n", 1).error)
        self.assertTrue(route.ext.SetValue("channel", "web"))
        ok, value, error = route.ext.TryGetValue("channel", protaco.INT)
        self.assertFalse(ok)
        self.assertIsNone(value)
        self.assertEqual("ext key channel holds string, not int", error)
        self.assertEqual("ext key not found: missing", route.ext.TryGetValue("missing", protaco.STRING).error)
        self.assertEqual("web", route.ext.TryGetValue("channel", protaco.STRING).value)

    def test_to_string_matches_cpp(self):
        self.assertEqual('DeliveryRoute{rider="rider-7", stops=[RouteStop{orderId="A-100", address="1 Pizza St", etaMin=10}, '
                         'RouteStop{orderId="A-101", address="2 Pasta Ave", etaMin=25}], note="closest-first", ext{priority=5}}',
                         make_delivery_route().ToString())
        self.assertEqual('Heartbeat{node="oven-1", seq=123456789, healthy=true, cpuLoad=0.25, uptimeSec=3600}', str(make_heartbeat()))
        self.assertEqual('DeliveryProof{orderId="A-1001", photo=<11 bytes: FF D8 00 10 4A 46 49 46 ...>, '
                         'signatures=[<1 bytes: 00>, <0 bytes>, <2 bytes: 01 02>]}', make_delivery_proof().ToString())
        self.assertEqual('Recipe{name="", temp=25.5, step=3, path="", note="none"}', Recipe2().ToString())
        self.assertEqual('DeliveryRoute{\n'
                         '  envelope{id=7, corr=0, ts=1000},\n'
                         '  rider="rider-7",\n'
                         '  stops=[\n'
                         '    RouteStop{\n'
                         '      orderId="A-100",\n'
                         '      address="1 Pizza St",\n'
                         '      etaMin=10\n'
                         '    },\n'
                         '    RouteStop{\n'
                         '      orderId="A-101",\n'
                         '      address="2 Pasta Ave",\n'
                         '      etaMin=25\n'
                         '    }\n'
                         '  ],\n'
                         '  note="closest-first",\n'
                         '  ext{\n'
                         '    priority=5\n'
                         '  }\n'
                         '}', make_delivery_route().ToString(protaco.TextStyle.Pretty()))
        style = protaco.TextStyle(separator="; ", open="(", close=")", maxText=4)
        self.assertEqual(r'RouteStop(orderId="a\"b\\"...(+2); address="Main"; etaMin=3)',
                         RouteStop(orderId='a"b\\c\n', address="Main", etaMin=3).ToString(style))
        route = DeliveryRoute(rider="rider-7")
        route.ext.SetValue("priority", 5, protaco.INT)
        route.ext.SetValue("channel", "web")
        route.ext.SetValue("hot", True)
        route.ext.SetValue("ratio", 0.5)
        route.ext.SetValue("big", 1234567890123, protaco.INT64)
        route.ext.SetValue("blob", b"\x00\xFF")
        self.assertEqual('DeliveryRoute{rider="rider-7", stops=[], note="", '
                         'ext{priority=5, channel="web", hot=true, ratio=0.5, big=1234567890123, blob=<2 bytes: 00 FF>}}', route.ToString())

    def test_unsigned_types_and_utf8(self):
        defaults = Numbers()
        self.assertEqual(4000000000, defaults.u32)
        self.assertEqual(18446744073709551615, defaults.u64)
        self.assertEqual(-9000000000, defaults.i64)
        got = protaco.ToObject(Numbers, golden("Numbers.bin"))
        self.assertEqual("한글 ✓", got.name)
        self.assertEqual(4294967295, got.us[2])
        self.assertEqual('Numbers{u32=0, u64=18446744073709551615, i64=-9000000000, us=[0, 1, 4294967295], u64s=[18446744073709551615], '
                         'name="한글 ✓", text="café 寿司"}', got.ToString())

    def test_envelope_is_stamped_on_creation(self):
        protaco.SetClock(lambda: 1760000000123456)
        try:
            first = DeliveryRoute()
            second = DeliveryRoute()
            self.assertEqual(first.GetId() + 1, second.GetId())
            self.assertEqual(1760000000123456, second.GetTimestamp())
            order = PizzaOrder()   # one id; its nested delivery field takes none
            decoded = protaco.ToObject(DeliveryRoute, first.Serialize())
            third = DeliveryRoute()
            self.assertEqual(second.GetId() + 2, third.GetId())
            self.assertEqual(first.GetId(), decoded.GetId())
            self.assertEqual(second.GetId() + 1, order.GetId())
        finally:
            protaco.SetClock(None)
        reply = DeliveryRoute()
        reply.SetCorr(first.GetId())
        reply.SetTimestampAsNow()
        self.assertGreater(reply.GetTimestamp(), protaco.TIMESTAMP_MIN)
        self.assertEqual(first.GetId(), protaco.ToObject(DeliveryRoute, reply.Serialize()).GetCorr())

    def test_auto_envelope_can_be_turned_off(self):
        # auto_envelope false (Tick) and DisableAutoEnvelope() leave id and ts at 0 and take no id from the counter.
        before = DeliveryRoute()
        tick = Tick()
        self.assertEqual((0, 0), (tick.GetId(), tick.GetTimestamp()))
        protaco.DisableAutoEnvelope()
        try:
            off = DeliveryRoute()
        finally:
            protaco.EnableAutoEnvelope()
        self.assertEqual((0, 0), (off.GetId(), off.GetTimestamp()))
        self.assertEqual(before.GetId() + 1, DeliveryRoute().GetId())
        sent = Tick(n=1)
        sent.SetId(protaco.NextId())
        sent.SetTimestampAsNow()
        got = protaco.ToObject(Tick, sent.Serialize())
        self.assertEqual((sent.GetId(), sent.GetTimestamp()), (got.GetId(), got.GetTimestamp()))

    def test_timestamp_converts_to_local_time(self):
        local = datetime.datetime(2026, 10, 20, 14, 3, 22)
        timestamp = int(local.timestamp()) * 1000000 + 123456
        route = DeliveryRoute()
        route.SetId(9)
        route.SetTimestamp(timestamp)
        ok, (date, time_text), _ = route.TryGetTimeFromTimestamp()
        self.assertTrue(ok)
        self.assertEqual("2026-10-20", date)
        self.assertEqual("14:03:22.123456", time_text)
        parts = route.TryGetTimeFromTimestampParts()
        self.assertEqual((2026, 10, 20, 14, 3, 22, 123, 456), parts.value)
        self.assertEqual('DeliveryRoute{\n  envelope{id=9, corr=0, ts=2026-10-20 14:03:22.123456},\n  rider="",\n  stops=[],\n  note=""\n}',
                         route.ToString(protaco.TextStyle.Pretty()))
        route.SetTimestamp(1000)
        result = route.TryGetTimeFromTimestamp()
        self.assertFalse(result)
        self.assertEqual("timestamp 1000 is not a time (2000-01-01 to 9999-12-31 in UTC microseconds)", result.error)

    def test_const_values(self):
        self.assertEqual((4, 2, -7, 1), (Sizes.Slots, Sizes.Names, Sizes.Offset, Sizes.pass_))

    def test_enum_fields(self):
        defaults = Pie()
        self.assertEqual(PizzaSize.Large, defaults.size)
        self.assertEqual(PizzaSize.Small, defaults.crust)
        got = protaco.ToObject(Pie, golden("Pie.bin"))
        self.assertEqual(9, got.crust)
        self.assertIs(PizzaSize.Family, got.size)
        self.assertEqual('Pie{size=Family(4), crust=?(9), sizes=[Small(1), Family(4), Kids(5), ?(9)], name=""}', got.ToString())
        self.assertIs(PizzaSize.Medium, PizzaSizeNames.TryParse("Medium").value)
        self.assertEqual("unknown PizzaSize name: Kids", PizzaSizeNames.TryParse("Kids").error)
        self.assertEqual("Kids", PizzaSizeNames.Name(5))
        self.assertIsNone(PizzaSizeNames.Name(9))

    def test_fixed_arrays(self):
        defaults = Panel()
        self.assertEqual([-1] * 4, defaults.counts)
        self.assertEqual("none", defaults.names[1])
        self.assertEqual(Level.Low, defaults.levels[2])
        self.assertEqual(Sizes.Slots, len(defaults.temps))
        self.assertEqual(golden("Panel.bin"), make_panel().Serialize())
        got = protaco.ToObject(Panel, golden("Panel.bin"))
        self.assertEqual(-9000000000, got.ticks[0])
        self.assertEqual('Panel{temps=[20.5, 21.5, 22.5, 23.5], counts=[1, -2, 3, -4], ids=[1, 18446744073709551615, 7], masks=[0, 4294967295], ticks=[-9000000000, 0], flags=[false, true], names=["alpha", "none"], levels=[Low(1), Low(1), High(2)], total=42}', got.ToString())
        bad = make_panel()
        bad.names[1] = "123456789"
        with self.assertRaises(protaco.ProtacoError) as caught:
            bad.Serialize()
        self.assertEqual("names[1]: 9 bytes, longer than string(8)", str(caught.exception))
        short = make_panel()
        short.counts = [1, 2]
        with self.assertRaises(protaco.ProtacoError) as caught:
            short.Serialize()
        self.assertEqual("counts: 2 elements, the array has 4", str(caught.exception))

    def test_fixed_messages(self):
        defaults = Rack()
        self.assertEqual(-1, defaults.origin.x)
        self.assertEqual("empty", defaults.slots[3].name)
        self.assertEqual(-1, defaults.slots[0].path[1].x)
        self.assertEqual(golden("Rack.bin"), make_rack().Serialize())
        got = protaco.ToObject(Rack, golden("Rack.bin"))
        self.assertEqual(7, got.slots[2].path[1].x)
        self.assertEqual('Rack{origin=Point{x=3, y=0}, slots=[Slot{id=100, name="empty", pos=Point{x=-1, y=0}, path=[Point{x=-1, y=0}, Point{x=-1, y=0}], level=Low(1), temps=[0, 0]}, Slot{id=101, name="one", pos=Point{x=-1, y=1}, path=[Point{x=-1, y=0}, Point{x=-1, y=0}], level=Low(1), temps=[0, 0]}, Slot{id=102, name="empty", pos=Point{x=-1, y=2}, path=[Point{x=-1, y=0}, Point{x=7, y=0}], level=Low(1), temps=[0, 0]}, Slot{id=103, name="empty", pos=Point{x=-1, y=3}, path=[Point{x=-1, y=0}, Point{x=-1, y=0}], level=High(2), temps=[0, 1.5]}], count=4}', got.ToString())
        self.assertEqual(golden("Trail.bin"), make_trail().Serialize())
        self.assertEqual('Trail{start=Point{x=-1, y=5}, points=[Point{x=3, y=0}], note="n"}', protaco.ToObject(Trail, golden("Trail.bin")).ToString())
        bad = make_rack()
        bad.slots[1].name = "123456789"
        with self.assertRaises(protaco.ProtacoError) as caught:
            bad.Serialize()
        self.assertEqual("slots[1].name: 9 bytes, longer than string(8)", str(caught.exception))

    def test_map_fields(self):
        got = protaco.ToObject(Menu, golden("Menu.bin"))
        self.assertEqual(MENU_ONE, got.ToString())
        self.assertEqual(9, got.grades["ragu"])
        sent = make_menu()
        sent.prices["dupB"] = 7.0
        sent.prices["coffee"] = 3.5
        sent.stock[2] = 0
        got = protaco.ToObject(Menu, sent.Serialize())
        self.assertEqual(sent.prices, got.prices)
        self.assertEqual(sent.daily, got.daily)
        self.assertEqual(["hot", ""], got.tags["ragu"])
        text = got.ToString()
        self.assertEqual('Menu{prices={"coffee": 3.5, "dupA": 9.5, "dupB": 7}, stock={-1: 4294967295, 2: 0}, '
                         'specials={Silver(2): Dish{name="ragu", price=12.25}}, ', text[:text.index("notes=")])
        # Same key twice: "dupB" patched to "dupA".
        data = bytearray(sent.Serialize())
        at = data.index(b"\x04\x00\x00\x00dupB")
        data[at + 7] = ord("A")
        error = self.fails(Menu, bytes(data))
        self.assertTrue(error.path.startswith("prices["), error.path)
        self.assertEqual("duplicate map key", error.what)

    def test_recursive_messages(self):
        got = protaco.ToObject(Node, golden("Node.bin"))
        self.assertEqual(NODE_TEXT, got.ToString())
        self.assertIsNone(got.next)
        self.assertEqual("b", got.children[0].next.name)

    def test_recursion_stops_at_100_levels(self):
        for levels in (100, 101):
            root = Node()
            node = root
            for _ in range(levels):
                node.next = Node()
                node = node.next
            result = protaco.TryToObject(Node, root.Serialize())
            if levels == 100:
                self.assertTrue(result, result.error)
            else:
                self.assertFalse(result)
                self.assertTrue(result.error.startswith("next.next.next."))
                self.assertTrue(result.error.endswith(": nesting too deep"))

    def test_python_keyword_keys(self):
        move = Move(from_="a1", to="b2")
        move.pass_ = True
        got = protaco.ToObject(Move, move.Serialize())
        self.assertEqual("a1", got.from_)
        self.assertTrue(got.pass_)
        self.assertEqual('Move{from="a1", to="b2", pass=true}', got.ToString())
        self.assertEqual("a1", Move(**{"from": "a1"}).from_)

    def test_peek_header(self):
        ok, header, _ = protaco.PeekHeader(golden("DeliveryRoute.bin"))
        self.assertTrue(ok)
        self.assertEqual("DeliveryRoute", header.name)
        self.assertEqual(protaco.Envelope(7, 0, 1000), header.env)
        self.assertEqual("frame too short", protaco.PeekHeader(b"PC").error)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]], verbosity=1)
