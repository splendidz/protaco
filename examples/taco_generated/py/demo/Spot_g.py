# Spot_g.py. protacoc 0.7.0, source Spot.yaml (sha1 11b8a28b10ac). Do not edit.
import protaco


class Spot(protaco.Message):
    """A place on the dispatch board. fixed: true, so it sits inline in the board's fixed block.
    [version 1; schema 0x2538569A; fixed: inline in other messages; source Spot.yaml]
    """

    __slots__ = ("lat", "lon", "label")
    MsgName = "Spot"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x2538569A

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Spot(lat=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type double]
        self.lat = 0.0
        #: [type double]
        self.lon = 0.0
        #: [type string(16); fixed 16 bytes, set rejects longer values]
        self.label = ""
        if fields:
            self._Assign(fields)


Spot._Bind(
    hashes=(0x2538569A,),
    allow_extension=False,
    schema_keys=("lat", "lon", "label"),
    fixed=(
        ("lat", "lat", protaco.Items.Double, None),
        ("lon", "lon", protaco.Items.Double, None),
        ("label", "label", protaco.Items.Chars(16), None),
    ),
    var=(),
    text=("lat", "lon", "label"),
)
