# PizzaOrder_g.py. protacoc 0.7.0, source PizzaOrder.yaml (sha1 a1af26cb1afe). Do not edit.
import protaco
from .Delivery_g import Delivery
from .Topping_g import Topping


class PizzaOrder(protaco.Message):
    """Pizza order. One message for every store; fields used by one store only say so in their comment.
    pizza order
    [version 1; schema 0x9D0CF242; source PizzaOrder.yaml]
    """

    __slots__ = ("orderId", "store", "customer", "size", "slices", "woodFired", "deepDishCm", "toppings", "notes", "delivery")
    MsgName = "PizzaOrder"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x9D0CF242

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example PizzaOrder(orderId=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: Order number. Fixed length to fit the receipt column.
        #: order number
        #: [type string(32); fixed 32 bytes, set rejects longer values]
        self.orderId = ""
        #: store name
        #: [type string(32); fixed 32 bytes, set rejects longer values]
        self.store = ""
        #: customer name
        #: [type string; default "anonymous"]
        self.customer = "anonymous"
        #: diameter in inches
        #: [type int; default 12]
        self.size = 12
        #: number of slices
        #: [type int; default 8]
        self.slices = 8
        #: Wood-fired oven. NAPOLI store only.
        #: baked in a wood-fired oven
        #: [type bool; default true]
        self.woodFired = True
        #: Deep dish depth in cm. CHICAGO store only. 0 means regular crust.
        #: deep dish depth
        #: [type int; default 0]
        self.deepDishCm = 0
        #: toppings
        #: [type list<Topping>; required, decoding fails if missing]
        self.toppings = []
        #: special requests (e.g. no onions)
        #: [type list<string>]
        self.notes = []
        #: delivery details
        #: [type Delivery]
        self.delivery = Delivery(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


PizzaOrder._Bind(
    hashes=(0x9D0CF242,),
    allow_extension=True,
    schema_keys=("orderId", "store", "customer", "size", "slices", "woodFired", "deepDishCm", "toppings", "notes", "delivery"),
    fixed=(
        ("orderId", "orderId", protaco.Items.Chars(32), None),
        ("store", "store", protaco.Items.Chars(32), None),
        ("size", "size", protaco.Items.Int, None),
        ("slices", "slices", protaco.Items.Int, None),
        ("woodFired", "woodFired", protaco.Items.Bool, None),
        ("deepDishCm", "deepDishCm", protaco.Items.Int, None),
    ),
    var=(
        ("customer", "customer", protaco.Items.String, False, False),
        ("toppings", "toppings", protaco.Items.List(protaco.Items.Message(Topping)), True, False),
        ("notes", "notes", protaco.Items.List(protaco.Items.String), False, False),
        ("delivery", "delivery", protaco.Items.Message(Delivery), False, False),
    ),
    text=("orderId", "store", "customer", "size", "slices", "woodFired", "deepDishCm", "toppings", "notes", "delivery"),
)
