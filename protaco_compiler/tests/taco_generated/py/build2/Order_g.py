# Order_g.py. protacoc 0.7.1, source Order.yaml (sha1 d90a16ea2114). Do not edit.
import protaco


class Order(protaco.Message):
    """Version window test. Order at version 2: weight (fixed) and tags (variable) added.
    [version 2; schema 0x0E7D8E49; supports 1..2; source Order.yaml]
    """

    __slots__ = ("id", "weight", "qty", "note", "color", "tags")
    MsgName = "Order"
    MsgVersion = 2
    MsgMinVersion = 1
    MsgSchemaHash = 0x0E7D8E49

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Order(id=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string(8); fixed 8 bytes, set rejects longer values]
        self.id = ""
        #: Declared in the middle, laid out after the version 1 fields.
        #: [type double; default 0.5; added in 2]
        self.weight = 0.5
        #: [type int; default 1]
        self.qty = 1
        #: [type string]
        self.note = ""
        #: [type string; required, decoding fails if missing]
        self.color = ""
        #: [type list<string>; added in 2]
        self.tags = []
        if fields:
            self._Assign(fields)


Order._Bind(
    hashes=(0xD05758A9, 0x0E7D8E49),
    allow_extension=True,
    schema_keys=("id", "weight", "qty", "note", "color", "tags"),
    fixed=(
        ("id", "id", protaco.Items.Chars(8), None),
        ("qty", "qty", protaco.Items.Int, None),
        ("weight", "weight", protaco.Items.Double, None),
    ),
    var=(
        ("note", "note", protaco.Items.String, False, False),
        ("color", "color", protaco.Items.String, True, False),
        ("tags", "tags", protaco.Items.List(protaco.Items.String), False, False),
    ),
    text=("id", "weight", "qty", "note", "color", "tags"),
)


class v1:
    """Version 1 of Order, for frames from peers on version 1."""

    class Order(protaco.Message):
        """Version window test. Order at version 2: weight (fixed) and tags (variable) added.
        Version 1 layout, for frames from peers on version 1.
        [version 1; schema 0xD05758A9; supports 1..2; source Order.yaml]
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


v1.Order._Bind(
    hashes=(0xD05758A9,),
    allow_extension=True,
    schema_keys=("id", "weight", "qty", "note", "color", "tags"),
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
