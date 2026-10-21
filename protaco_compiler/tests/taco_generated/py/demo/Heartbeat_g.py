# Heartbeat_g.py. protacoc 0.7.0, source Heartbeat.yaml (sha1 32f326820062). Do not edit.
import protaco


class Heartbeat(protaco.Message):
    """Device liveness signal. Every field is fixed-length and ext is off, so Serialize() returns the buffer as is (zero cost).
    Recommended shape for messages sent very often.
    [version 1; schema 0x2BB85C55; allow_extension false; source Heartbeat.yaml]
    """

    __slots__ = ("node", "seq", "healthy", "cpuLoad", "uptimeSec")
    MsgName = "Heartbeat"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x2BB85C55

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Heartbeat(node=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: device name
        #: [type string(32); fixed 32 bytes, set rejects longer values]
        self.node = ""
        #: sequence number
        #: [type int64]
        self.seq = 0
        #: device is healthy
        #: [type bool; default true]
        self.healthy = True
        #: CPU load (0.0..1.0)
        #: [type double]
        self.cpuLoad = 0.0
        #: uptime in seconds
        #: [type int]
        self.uptimeSec = 0
        if fields:
            self._Assign(fields)


Heartbeat._Bind(
    hashes=(0x2BB85C55,),
    allow_extension=False,
    schema_keys=("node", "seq", "healthy", "cpuLoad", "uptimeSec"),
    fixed=(
        ("node", "node", protaco.Items.Chars(32), None),
        ("seq", "seq", protaco.Items.Int64, None),
        ("healthy", "healthy", protaco.Items.Bool, None),
        ("cpuLoad", "cpuLoad", protaco.Items.Double, None),
        ("uptimeSec", "uptimeSec", protaco.Items.Int, None),
    ),
    var=(),
    text=("node", "seq", "healthy", "cpuLoad", "uptimeSec"),
)
