# Ping_g.py. protacoc 0.6.0, source Ping.yaml (sha1 dd3741203727). Do not edit.
import protaco


class Ping(protaco.Message):
    """Version window test. Closed message at version 2: text added.
    [version 2; schema 0xBC1DCB81; supports 1..2; allow_extension false; source Ping.yaml]
    """

    __slots__ = ("seq", "who", "text")
    MsgName = "Ping"
    MsgVersion = 2
    MsgMinVersion = 1
    MsgSchemaHash = 0xBC1DCB81

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Ping(seq=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type int]
        self.seq = 0
        #: [type string]
        self.who = ""
        #: [type string; added in 2]
        self.text = ""
        if fields:
            self._Assign(fields)


Ping._Bind(
    hashes=(0xB81406B7, 0xBC1DCB81),
    allow_extension=False,
    schema_keys=("seq", "who", "text"),
    fixed=(
        ("seq", "seq", protaco.Items.Int, None),
    ),
    var=(
        ("who", "who", protaco.Items.String, False, False),
        ("text", "text", protaco.Items.String, False, False),
    ),
    text=("seq", "who", "text"),
)


class v1:
    """Version 1 of Ping, for frames from peers on version 1."""

    class Ping(protaco.Message):
        """Version window test. Closed message at version 2: text added.
        Version 1 layout, for frames from peers on version 1.
        [version 1; schema 0xB81406B7; supports 1..2; allow_extension false; source Ping.yaml]
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


v1.Ping._Bind(
    hashes=(0xB81406B7,),
    allow_extension=False,
    schema_keys=("seq", "who", "text"),
    fixed=(
        ("seq", "seq", protaco.Items.Int, None),
    ),
    var=(
        ("who", "who", protaco.Items.String, False, False),
    ),
    text=("seq", "who"),
)
