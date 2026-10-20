# Panel_g.py. protacoc 0.6.0, source Panel.yaml (sha1 236d0ca5085f). Do not edit.
import protaco
from .Level_enum import Level, LevelNames


class Panel(protaco.Message):
    """Fixed-length arrays of every element kind. Every field is fixed, so TryView reads it in place.
    [version 1; schema 0x71A7E9E9; allow_extension false; source Panel.yaml]
    """

    __slots__ = ("temps", "counts", "ids", "masks", "ticks", "flags", "names", "levels", "total")
    MsgName = "Panel"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x71A7E9E9

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Panel(temps=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type double[Sizes.Slots]; 4 elements, fixed 32 bytes; default 20.5]
        self.temps = [20.5] * 4
        #: [type int[Sizes.Slots]; 4 elements, fixed 16 bytes; default -1]
        self.counts = [-1] * 4
        #: [type uint64[3]; 3 elements, fixed 24 bytes]
        self.ids = [0] * 3
        #: [type uint[2]; 2 elements, fixed 8 bytes]
        self.masks = [0] * 2
        #: [type int64[2]; 2 elements, fixed 16 bytes]
        self.ticks = [0] * 2
        #: [type bool[2]; 2 elements, fixed 2 bytes]
        self.flags = [False] * 2
        #: [type string(8)[Sizes.Names]; 2 elements, fixed 16 bytes, set rejects values longer than 8 bytes; default none]
        self.names = ["none"] * 2
        #: [type Level[Level.Count]; 3 elements, fixed 12 bytes]
        self.levels = [Level.Low] * 3
        #: [type int]
        self.total = 0
        if fields:
            self._Assign(fields)


Panel._Bind(
    hashes=(0x71A7E9E9,),
    allow_extension=False,
    schema_keys=("temps", "counts", "ids", "masks", "ticks", "flags", "names", "levels", "total"),
    fixed=(
        ("temps", "temps", protaco.Items.Array(protaco.Items.Double, 4), None),
        ("counts", "counts", protaco.Items.Array(protaco.Items.Int, 4), None),
        ("ids", "ids", protaco.Items.Array(protaco.Items.UInt64, 3), None),
        ("masks", "masks", protaco.Items.Array(protaco.Items.UInt, 2), None),
        ("ticks", "ticks", protaco.Items.Array(protaco.Items.Int64, 2), None),
        ("flags", "flags", protaco.Items.Array(protaco.Items.Bool, 2), None),
        ("names", "names", protaco.Items.Array(protaco.Items.Chars(8), 2), None),
        ("levels", "levels", protaco.Items.Array(protaco.Items.Enum(Level, LevelNames.Name), 3), None),
        ("total", "total", protaco.Items.Int, None),
    ),
    var=(),
    text=("temps", "counts", "ids", "masks", "ticks", "flags", "names", "levels", "total"),
)
