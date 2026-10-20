# DeliveryRoute_g.py. protacoc 0.6.0, source DeliveryRoute.yaml (sha1 974fbf1766a4). Do not edit.
import protaco


class DeliveryRoute(protaco.Message):
    """Error test. A sender that leaves out the required stops.
    [version 1; schema 0xD6D1FAE0; source DeliveryRoute.yaml]
    """

    __slots__ = ("rider",)
    MsgName = "DeliveryRoute"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xD6D1FAE0

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example DeliveryRoute(rider=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string]
        self.rider = ""
        if fields:
            self._Assign(fields)


DeliveryRoute._Bind(
    hashes=(0xD6D1FAE0,),
    allow_extension=True,
    schema_keys=("rider",),
    fixed=(),
    var=(
        ("rider", "rider", protaco.Items.String, False, False),
    ),
    text=("rider",),
)
