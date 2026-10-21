# DeliveryRoute_g.py. protacoc 0.7.0, source DeliveryRoute.yaml (sha1 c54af80e4be8). Do not edit.
import protaco
from .RouteStop_g import RouteStop


class DeliveryRoute(protaco.Message):
    """Delivery route of one rider.
    [version 1; schema 0x7A0526C6; source DeliveryRoute.yaml]
    """

    __slots__ = ("rider", "stops", "note")
    MsgName = "DeliveryRoute"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x7A0526C6

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example DeliveryRoute(rider=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: rider id
        #: [type string; required, decoding fails if missing]
        self.rider = ""
        #: Stops, visited in order.
        #: stops
        #: [type list<RouteStop>; required, decoding fails if missing]
        self.stops = []
        #: why this route
        #: [type string; default ""]
        self.note = ""
        if fields:
            self._Assign(fields)


DeliveryRoute._Bind(
    hashes=(0x7A0526C6,),
    allow_extension=True,
    schema_keys=("rider", "stops", "note"),
    fixed=(),
    var=(
        ("rider", "rider", protaco.Items.String, True, False),
        ("stops", "stops", protaco.Items.List(protaco.Items.Message(RouteStop)), True, False),
        ("note", "note", protaco.Items.String, False, False),
    ),
    text=("rider", "stops", "note"),
)
