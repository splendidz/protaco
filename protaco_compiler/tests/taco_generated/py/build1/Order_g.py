# Order_g.py. protacoc 0.7.1, source Order.yaml (sha1 8e16b9d89079). Do not edit.
import protaco


class Order(protaco.Message):
    """Version window test. Order as shipped at version 1. build2..build4 are later edits of the same file.
    [version 1; schema 0xD05758A9; source Order.yaml]
    """

    __slots__ = ("id", "qty", "note", "color")
    MsgName = "Order"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xD05758A9

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Order(id=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string(8); fixed 8 bytes, set rejects longer values]
        self.id = ""
        #: [type int; default 1]
        self.qty = 1
        #: [type string]
        self.note = ""
        #: [type string; required, decoding fails if missing]
        self.color = ""
        if fields:
            self._Assign(fields)


Order._Bind(
    hashes=(0xD05758A9,),
    allow_extension=True,
    schema_keys=("id", "qty", "note", "color"),
    fixed=(
        ("id", "id", protaco.Items.Chars(8), None),
        ("qty", "qty", protaco.Items.Int, None),
    ),
    var=(
        ("note", "note", protaco.Items.String, False, False),
        ("color", "color", protaco.Items.String, True, False),
    ),
    text=("id", "qty", "note", "color"),
)
