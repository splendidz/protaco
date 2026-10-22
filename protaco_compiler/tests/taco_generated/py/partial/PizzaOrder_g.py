# PizzaOrder_g.py. protacoc 0.7.1, source PizzaOrder.yaml (sha1 300feed1cc93). Do not edit.
import protaco
from .Delivery_g import Delivery
from .Topping_g import Topping


class PizzaOrder(protaco.Message):
    """Error test. Same fixed fields as examples/schemas/PizzaOrder.yaml; the nested doorCode has another type
    (error path "delivery.doorCode").
    [version 1; schema 0x1643B1CB; source PizzaOrder.yaml]
    """

    __slots__ = ("orderId", "store", "size", "slices", "woodFired", "deepDishCm", "toppings", "delivery")
    MsgName = "PizzaOrder"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x1643B1CB

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example PizzaOrder(orderId=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string(32); fixed 32 bytes, set rejects longer values]
        self.orderId = ""
        #: [type string(32); fixed 32 bytes, set rejects longer values]
        self.store = ""
        #: [type int]
        self.size = 0
        #: [type int]
        self.slices = 0
        #: [type bool]
        self.woodFired = False
        #: [type int]
        self.deepDishCm = 0
        #: [type list<Topping>]
        self.toppings = []
        #: [type Delivery]
        self.delivery = Delivery(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


PizzaOrder._Bind(
    hashes=(0x1643B1CB,),
    allow_extension=True,
    schema_keys=("orderId", "store", "size", "slices", "woodFired", "deepDishCm", "toppings", "delivery"),
    fixed=(
        ("orderId", "orderId", protaco.Items.Chars(32), None),
        ("store", "store", protaco.Items.Chars(32), None),
        ("size", "size", protaco.Items.Int, None),
        ("slices", "slices", protaco.Items.Int, None),
        ("woodFired", "woodFired", protaco.Items.Bool, None),
        ("deepDishCm", "deepDishCm", protaco.Items.Int, None),
    ),
    var=(
        ("toppings", "toppings", protaco.Items.List(protaco.Items.Message(Topping)), False, False),
        ("delivery", "delivery", protaco.Items.Message(Delivery), False, False),
    ),
    text=("orderId", "store", "size", "slices", "woodFired", "deepDishCm", "toppings", "delivery"),
)
