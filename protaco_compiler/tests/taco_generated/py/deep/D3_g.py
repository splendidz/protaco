# D3_g.py. protacoc 0.6.0, source D3.yaml (sha1 5f1911271792). Do not edit.
import protaco
from .D2_g import D2


class D3(protaco.Message):
    """[version 1; schema 0x93F6E6B3; source D3.yaml]
    """

    __slots__ = ("d",)
    MsgName = "D3"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x93F6E6B3

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example D3(d=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type D2]
        self.d = D2(_stamp=protaco.NO_STAMP)
        if fields:
            self._Assign(fields)


D3._Bind(
    hashes=(0x93F6E6B3,),
    allow_extension=True,
    schema_keys=("d",),
    fixed=(),
    var=(
        ("d", "d", protaco.Items.Message(D2), False, False),
    ),
    text=("d",),
)
