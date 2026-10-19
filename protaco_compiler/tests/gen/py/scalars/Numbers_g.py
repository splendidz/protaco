# Numbers_g.py. protacoc 0.6.0, source Numbers.yaml (sha1 c3af6c4b45e4). Do not edit.
import protaco


class Numbers(protaco.Message):
    """Unsigned integers at their limits and UTF-8 strings.
    [version 1; schema 0x8230A950; source Numbers.yaml]
    """

    __slots__ = ("u32", "u64", "i64", "us", "u64s", "name", "text")
    MsgName = "Numbers"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x8230A950

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Numbers(u32=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type uint; default 4000000000]
        self.u32 = 4000000000
        #: [type uint64; default 18446744073709551615]
        self.u64 = 18446744073709551615
        #: [type int64; default -9000000000]
        self.i64 = -9000000000
        #: [type list<uint>]
        self.us = []
        #: [type list<uint64>]
        self.u64s = []
        #: [type string(16); fixed 16 bytes, set rejects longer values; default ""]
        self.name = ""
        #: [type string]
        self.text = ""
        if fields:
            self._Assign(fields)


Numbers._Bind(
    hashes=(0x8230A950,),
    allow_extension=True,
    schema_keys=("u32", "u64", "i64", "us", "u64s", "name", "text"),
    fixed=(
        ("u32", "u32", protaco.Items.UInt, None),
        ("u64", "u64", protaco.Items.UInt64, None),
        ("i64", "i64", protaco.Items.Int64, None),
        ("name", "name", protaco.Items.Chars(16), None),
    ),
    var=(
        ("us", "us", protaco.Items.List(protaco.Items.UInt), False, False),
        ("u64s", "u64s", protaco.Items.List(protaco.Items.UInt64), False, False),
        ("text", "text", protaco.Items.String, False, False),
    ),
    text=("u32", "u64", "i64", "us", "u64s", "name", "text"),
)
