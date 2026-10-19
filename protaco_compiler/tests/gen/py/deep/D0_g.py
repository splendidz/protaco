# D0_g.py. protacoc 0.6.0, source D0.yaml (sha1 c36d92fc3a71). Do not edit.
import protaco


class D0(protaco.Message):
    """Depth test. D<n> nests n levels; D8 is the deepest allowed (runtime kMaxDepth).
    [version 1; schema 0x329E542F; source D0.yaml]
    """

    __slots__ = ("v",)
    MsgName = "D0"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x329E542F

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D0(v=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type int]
        self.v = 0
        if fields:
            self._Assign(fields)


D0._Bind(
    hashes=(0x329E542F,),
    allow_extension=True,
    schema_keys=("v",),
    fixed=(
        ("v", "v", protaco.Items.Int, None),
    ),
    var=(),
    text=("v",),
)
