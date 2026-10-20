# D4_g.py. protacoc 0.6.0, source D4.yaml (sha1 9998c5d5943b). Do not edit.
import protaco
from .D3_g import D3


class D4(protaco.Message):
    """[version 1; schema 0x0ABD0F13; source D4.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D4"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x0ABD0F13

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D4(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D3]
        self.d = D3(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D4._Bind(
    hashes=(0x0ABD0F13,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D3), False, False),
    ),
    text=("d",),
)
