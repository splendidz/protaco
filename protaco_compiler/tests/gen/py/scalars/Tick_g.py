# Tick_g.py. protacoc 0.6.0, source Tick.yaml (sha1 d48b7805e10f). Do not edit.
import protaco


class Tick(protaco.Message):
    """The sender fills the envelope: new objects keep id 0 and ts 0.
    [version 1; schema 0x6D9855B0; source Tick.yaml]
    """

    __slots__ = ("n",)
    MsgName = "Tick"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x6D9855B0

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Tick(n=...).
        The envelope stays 0 (auto_envelope false): set it with SetId, SetCorr and SetTimestamp."""
        protaco.Message.__init__(self, protaco.NO_STAMP)
        #: [type int]
        self.n = 0
        if fields:
            self._Assign(fields)


Tick._Bind(
    hashes=(0x6D9855B0,),
    allow_extension=True,
    schema_keys=("n",),
    fixed=(
        ("n", "n", protaco.Items.Int, None),
    ),
    var=(),
    text=("n",),
)
