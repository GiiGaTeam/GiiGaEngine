from typing import Callable
from .BlackBoard import BlackBoard

pred_type = Callable[[BlackBoard], bool]


def IsHighHP(bb: BlackBoard) -> bool:
    return bb.getKey["HP"] > 50


def EnemySet(bb: BlackBoard) -> bool:
    return bb.getKey["Enemy"] is not None


def target_acquired(bb: BlackBoard) -> bool:
    return False


def target_lost(bb: BlackBoard) -> bool:
    return True


def target_in_warhead_range(bb: BlackBoard) -> bool:
    return False


def target_dead(bb: BlackBoard) -> bool:
    return False
