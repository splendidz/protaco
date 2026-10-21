# D7_g.py. protacoc 0.7.0, source D7.yaml (sha1 e4cf6dfc2fdd). Do not edit.
import protaco
from .D6_g import D6


class D7(protaco.Message):
    """[version 1; schema 0x2D65EA08; source D7.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D7"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x2D65EA08

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D7(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D6]
        self.d = D6(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D7._Bind(
    hashes=(0x2D65EA08,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D6), False, False),
    ),
    text=("d",),
)
