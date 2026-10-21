# RouteStop_g.py. protacoc 0.7.0, source RouteStop.yaml (sha1 7cc7901a2cd9). Do not edit.
import protaco


class RouteStop(protaco.Message):
    """One stop of a delivery route. Element of DeliveryRoute.stops.
    Every field is fixed-length, so sending it alone costs nothing to serialize.
    [version 1; schema 0x9F1C87C1; source RouteStop.yaml]
    """

    __slots__ = ("orderId", "address", "etaMin")
    MsgName = "RouteStop"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x9F1C87C1

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example RouteStop(orderId=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: Order delivered at this stop. Example: A-100
        #: order number
        #: [type string(16); fixed 16 bytes, set rejects longer values]
        self.orderId = ""
        #: street address
        #: [type string(48); fixed 48 bytes, set rejects longer values]
        self.address = ""
        #: minutes until arrival
        #: [type int; default 15]
        self.etaMin = 15
        if fields:
            self._Assign(fields)


RouteStop._Bind(
    hashes=(0x9F1C87C1,),
    allow_extension=True,
    schema_keys=("orderId", "address", "etaMin"),
    fixed=(
        ("orderId", "orderId", protaco.Items.Chars(16), None),
        ("address", "address", protaco.Items.Chars(48), None),
        ("etaMin", "etaMin", protaco.Items.Int, None),
    ),
    var=(),
    text=("orderId", "address", "etaMin"),
)
