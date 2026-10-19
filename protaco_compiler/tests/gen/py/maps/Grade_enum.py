# Grade_enum.py. protacoc 0.6.0, source Grade.enum.yaml (sha1 4a4ddba23d94). Do not edit.
import enum

import protaco


class Grade(enum.IntEnum):
    """Dish grade.
    [enum; version 1; source Grade.enum.yaml]
    """

    Gold = 1
    Silver = 2
    Bronze = 3


class GradeNames:
    """Names of Grade values."""

    _NAMES = {1: "Gold", 2: "Silver", 3: "Bronze"}   # removed values included
    _VALUES = {"Gold": Grade.Gold, "Silver": Grade.Silver, "Bronze": Grade.Bronze}

    @staticmethod
    def Name(value):
        """Name of a value, removed values included. None for a number never declared."""
        return GradeNames._NAMES.get(int(value))

    @staticmethod
    def TryParse(name):
        """Value from its yaml name (values of version 1). Result value: the Grade."""
        value = GradeNames._VALUES.get(name)
        if value is None:
            return protaco.Result(False, None, "unknown Grade name: " + str(name))
        return protaco.Result(True, value)
