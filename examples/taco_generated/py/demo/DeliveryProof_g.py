# DeliveryProof_g.py. protacoc 0.7.0, source DeliveryProof.yaml (sha1 c97411e287f7). Do not edit.
import protaco


class DeliveryProof(protaco.Message):
    """Proof of delivery. Shows the bytes type: binary data carried as is, zeros and 0xFF included.
    [version 1; schema 0xF769024A; source DeliveryProof.yaml]
    """

    __slots__ = ("orderId", "photo", "signatures")
    MsgName = "DeliveryProof"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0xF769024A

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example DeliveryProof(orderId=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: order number
        #: [type string(32); fixed 32 bytes, set rejects longer values]
        self.orderId = ""
        #: JPEG bytes, untouched.
        #: doorstep photo
        #: [type bytes; required, decoding fails if missing]
        self.photo = b""
        #: signature images
        #: [type list<bytes>]
        self.signatures = []
        if fields:
            self._Assign(fields)


DeliveryProof._Bind(
    hashes=(0xF769024A,),
    allow_extension=True,
    schema_keys=("orderId", "photo", "signatures"),
    fixed=(
        ("orderId", "orderId", protaco.Items.Chars(32), None),
    ),
    var=(
        ("photo", "photo", protaco.Items.Bytes, True, False),
        ("signatures", "signatures", protaco.Items.List(protaco.Items.Bytes), False, False),
    ),
    text=("orderId", "photo", "signatures"),
)
