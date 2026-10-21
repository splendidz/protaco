# D8_g.py. protacoc 0.7.0, source D8.yaml (sha1 547614165651). Do not edit.
import protaco
from .D7_g import D7


class D8(protaco.Message):
    """[version 1; schema 0x8DDE6810; source D8.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D8"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x8DDE6810

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D8(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D7]
        self.d = D7(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D8._Bind(
    hashes=(0x8DDE6810,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D7), False, False),
    ),
    text=("d",),
)
