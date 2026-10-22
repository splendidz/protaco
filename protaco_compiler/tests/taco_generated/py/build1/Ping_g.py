# Ping_g.py. protacoc 0.7.1, source Ping.yaml (sha1 600f6283777d). Do not edit.
import protaco


class Ping(protaco.Message):
    """Version window test. Closed message at version 1.
    [version 1; schema 0xB81406B7; allow_extension false; source Ping.yaml]
    """

    __slots__ = ("seq", "who")
    MsgName = "Ping"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xB81406B7

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Ping(seq=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type int]
        self.seq = 0
        #: [type string]
        self.who = ""
        if fields:
            self._Assign(fields)


Ping._Bind(
    hashes=(0xB81406B7,),
    allow_extension=False,
    schema_keys=("seq", "who"),
    fixed=(
        ("seq", "seq", protaco.Items.Int, None),
    ),
    var=(
        ("who", "who", protaco.Items.String, False, False),
    ),
    text=("seq", "who"),
)
