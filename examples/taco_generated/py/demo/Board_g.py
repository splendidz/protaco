# Board_g.py. protacoc 0.7.0, source Board.yaml (sha1 9f54d74d5bc3). Do not edit.
import protaco
from .PizzaSize_enum import PizzaSize, PizzaSizeNames
from .Spot_g import Spot


class Board(protaco.Message):
    """Dispatch board kept in shared memory. Every field is fixed, so programs read and write it in place.
    [version 1; schema 0xB785B631; allow_extension false; source Board.yaml]
    """

    __slots__ = ("depot", "spots", "ovens", "queue", "busy")
    MsgName = "Board"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xB785B631

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Board(depot=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: where riders start
        #: [type Spot; fixed message, inline 32 bytes]
        self.depot = Spot(_stamp=protaco.NO_STAMP)
        #: delivery spots
        #: [type Spot[Limits.MaxSpots]; 4 elements, fixed 128 bytes]
        self.spots = [Spot(_stamp=protaco.NO_STAMP) for _ in range(4)]
        #: oven temperatures
        #: [type double[3]; 3 elements, fixed 24 bytes; default 220.5]
        self.ovens = [220.5] * 3
        #: The length comes from an enum value: PizzaSize.Large is 3.
        #: sizes of the next pizzas
        #: [type PizzaSize[PizzaSize.Large]; 3 elements, fixed 12 bytes]
        self.queue = [PizzaSize.Small] * 3
        #: riders out on a delivery
        #: [type bool[Limits.MaxRiders]; 8 elements, fixed 8 bytes]
        self.busy = [False] * 8
        if fields:
            self._Assign(fields)


Board._Bind(
    hashes=(0xB785B631,),
    allow_extension=False,
    schema_keys=("depot", "spots", "ovens", "queue", "busy"),
    fixed=(
        ("depot", "depot", protaco.Items.Fixed(Spot), None),
        ("spots", "spots", protaco.Items.Array(protaco.Items.Fixed(Spot), 4), None),
        ("ovens", "ovens", protaco.Items.Array(protaco.Items.Double, 3), None),
        ("queue", "queue", protaco.Items.Array(protaco.Items.Enum(PizzaSize, PizzaSizeNames.Name), 3), None),
        ("busy", "busy", protaco.Items.Array(protaco.Items.Bool, 8), None),
    ),
    var=(),
    text=("depot", "spots", "ovens", "queue", "busy"),
)
