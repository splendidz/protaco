# Level_enum.py. protacoc 0.6.0, source Level.enum.yaml (sha1 50e0c504e666). Do not edit.
import enum

import protaco


class Level(enum.IntEnum):
    """Alarm level of one slot.
    [enum; version 1; source Level.enum.yaml]
    """

    Low = 1
    High = 2
    #: number of levels and an array length
    Count = 3


class LevelNames:
    """Names of Level values."""

    _NAMES = {1: "Low", 2: "High", 3: "Count"}   # removed values included
    _VALUES = {"Low": Level.Low, "High": Level.High, "Count": Level.Count}

    @staticmethod
    def Name(value):
        """Name of a value, removed values included. None for a number never declared."""
        return LevelNames._NAMES.get(int(value))

    @staticmethod
    def TryParse(name):
        """Value from its yaml name (values of version 1). Result value: the Level."""
        value = LevelNames._VALUES.get(name)
        if value is None:
            return protaco.Result(False, None, "unknown Level name: " + str(name))
        return protaco.Result(True, value)
