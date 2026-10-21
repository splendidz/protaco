# Order_g.py. protacoc 0.7.0, source Order.yaml (sha1 da7bfc519b96). Do not edit.
import protaco


class Order(protaco.Message):
    """Version window test. Order at version 3: qty (fixed) and note (variable) removed, label and rush added.
    [version 3; schema 0x7073B2E7; supports 1..3; source Order.yaml]
    """

    __slots__ = ("id", "weight", "color", "tags", "label", "rush")
    MsgName = "Order"
    MsgVersion = 3
    MsgMinVersion = 1
    MsgSchemaHash = 0x7073B2E7

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Order(id=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string(8); fixed 8 bytes, set rejects longer values]
        self.id = ""
        #: [type double; default 0.5; added in 2]
        self.weight = 0.5
        #: [type string; required, decoding fails if missing]
        self.color = ""
        #: [type list<string>; added in 2]
        self.tags = []
        #: [type string; added in 3]
        self.label = ""
        #: [type bool; added in 3]
        self.rush = False
        if fields:
            self._Assign(fields)


Order._Bind(
    hashes=(0xD05758A9, 0x0E7D8E49, 0x7073B2E7),
    allow_extension=True,
    schema_keys=("id", "weight", "qty", "note", "color", "tags", "label", "rush"),
    fixed=(
        ("id", "id", protaco.Items.Chars(8), None),
        ("qty", None, protaco.Items.Int, 1),
        ("weight", "weight", protaco.Items.Double, None),
        ("rush", "rush", protaco.Items.Bool, None),
    ),
    var=(
        ("color", "color", protaco.Items.String, True, False),
        ("tags", "tags", protaco.Items.List(protaco.Items.String), False, False),
        ("label", "label", protaco.Items.String, False, False),
    ),
    text=("id", "weight", "color", "tags", "label", "rush"),
)


class v1:
    """Version 1 of Order, for frames from peers on version 1."""

    class Order(protaco.Message):
        """Version window test. Order at version 3: qty (fixed) and note (variable) removed, label and rush added.
        Version 1 layout, for frames from peers on version 1.
        [version 1; schema 0xD05758A9; supports 1..3; source Order.yaml]
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
            #: [type int; default 1; removed in 3]
            self.qty = 1
            #: [type string; removed in 3]
            self.note = ""
            #: [type string; required, decoding fails if missing]
            self.color = ""
            if fields:
                self._Assign(fields)


v1.Order._Bind(
    hashes=(0xD05758A9,),
    allow_extension=True,
    schema_keys=("id", "weight", "qty", "note", "color", "tags", "label", "rush"),
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


class v2:
    """Version 2 of Order, for frames from peers on version 2."""

    class Order(protaco.Message):
        """Version window test. Order at version 3: qty (fixed) and note (variable) removed, label and rush added.
        Version 2 layout, for frames from peers on version 2.
        [version 2; schema 0x0E7D8E49; supports 1..3; source Order.yaml]
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
            #: [type double; default 0.5; added in 2]
            self.weight = 0.5
            #: [type int; default 1; removed in 3]
            self.qty = 1
            #: [type string; removed in 3]
            self.note = ""
            #: [type string; required, decoding fails if missing]
            self.color = ""
            #: [type list<string>; added in 2]
            self.tags = []
            if fields:
                self._Assign(fields)


v2.Order._Bind(
    hashes=(0xD05758A9, 0x0E7D8E49),
    allow_extension=True,
    schema_keys=("id", "weight", "qty", "note", "color", "tags", "label", "rush"),
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
