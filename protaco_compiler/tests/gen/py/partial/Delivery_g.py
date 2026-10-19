# Delivery_g.py. protacoc 0.6.0, source Delivery.yaml (sha1 41a286d128f8). Do not edit.
import protaco


class Delivery(protaco.Message):
    """[version 1; schema 0x591D673B; source Delivery.yaml]
    """

    __slots__ = ("address", "floor", "doorCode")
    MsgName = "Delivery"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x591D673B

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Delivery(address=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string(64); fixed 64 bytes, set rejects longer values]
        self.address = ""
        #: [type int]
        self.floor = 0
        #: [type list<string>]
        self.doorCode = []
        if fields:
            self._Assign(fields)


Delivery._Bind(
    hashes=(0x591D673B,),
    allow_extension=True,
    schema_keys=("address", "floor", "doorCode"),
    fixed=(
        ("address", "address", protaco.Items.Chars(64), None),
        ("floor", "floor", protaco.Items.Int, None),
    ),
    var=(
        ("doorCode", "doorCode", protaco.Items.List(protaco.Items.String), False, False),
    ),
    text=("address", "floor", "doorCode"),
)
