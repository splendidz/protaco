# Delivery_g.py. protacoc 0.6.0, source Delivery.yaml (sha1 7aef43bfce7c). Do not edit.
import protaco


class Delivery(protaco.Message):
    """Delivery details. Used by PizzaOrder.delivery.
    [version 1; schema 0xA031C44E; source Delivery.yaml]
    """

    __slots__ = ("address", "floor", "doorCode", "contacts")
    MsgName = "Delivery"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xA031C44E

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Delivery(address=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: street address
        #: [type string(64); fixed 64 bytes, set rejects longer values]
        self.address = ""
        #: floor number
        #: [type int; default 1]
        self.floor = 1
        #: Building door code. One digit 0..9 per element.
        #: building door code
        #: [type list<int>]
        self.doorCode = []
        #: phone numbers
        #: [type list<string>]
        self.contacts = []
        if fields:
            self._Assign(fields)


Delivery._Bind(
    hashes=(0xA031C44E,),
    allow_extension=True,
    schema_keys=("address", "floor", "doorCode", "contacts"),
    fixed=(
        ("address", "address", protaco.Items.Chars(64), None),
        ("floor", "floor", protaco.Items.Int, None),
    ),
    var=(
        ("doorCode", "doorCode", protaco.Items.List(protaco.Items.Int), False, False),
        ("contacts", "contacts", protaco.Items.List(protaco.Items.String), False, False),
    ),
    text=("address", "floor", "doorCode", "contacts"),
)
