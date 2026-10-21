# Node_g.py. protacoc 0.7.0, source Node.yaml (sha1 215949a4f507). Do not edit.
import protaco


class Node(protaco.Message):
    """A tree that refers to itself: children in a list, and one optional next node.
    [version 1; schema 0x94A77C41; source Node.yaml]
    """

    __slots__ = ("name", "value", "children", "next")
    MsgName = "Node"
    MsgVersion = 1
    MsgMinVersion = 1
    MsgSchemaHash = 0x94A77C41

    def __init__(self, *, _stamp=None, **fields):
        """Defaults; fields by key, for example Node(name=...).
        The envelope gets a new id (protaco.NextId) and the current time (protaco.NowUs)."""
        protaco.Message.__init__(self, _stamp)
        #: [type string]
        self.name = ""
        #: [type int]
        self.value = 0
        #: [type list<Node>]
        self.children = []
        #: Absent until set. Nothing in the serialized data while absent.
        #: [type Node; optional, absent until set]
        self.next = None
        if fields:
            self._Assign(fields)


Node._Bind(
    hashes=(0x94A77C41,),
    allow_extension=True,
    schema_keys=("name", "value", "children", "next"),
    fixed=(
        ("value", "value", protaco.Items.Int, None),
    ),
    var=(
        ("name", "name", protaco.Items.String, False, False),
        ("children", "children", protaco.Items.List(protaco.Items.Message(Node)), False, False),
        ("next", "next", protaco.Items.Message(Node), False, True),
    ),
    text=("name", "value", "children", "next"),
)
