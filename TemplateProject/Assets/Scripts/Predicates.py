from typing import Callable
from .BlackBoard import BlackBoard

pred_type = Callable[[BlackBoard], bool]


def IsHighHP(bb: BlackBoard) -> bool:
    return bb.get("HP") > 50


def EnemySet(bb: BlackBoard) -> bool:
    return bb.get("Enemy") is not None


def target_dead(bb: BlackBoard) -> bool:
    return bb.get("target_dead", False)


def HasAmmo(bb: BlackBoard) -> bool:
    return bb.get("ammo", 0) > 0


def IsAimed(bb: BlackBoard) -> bool:
    return bb.get("is_aimed", False)


def CanShoot(bb: BlackBoard) -> bool:
    return bb.get("cooldown_remaining", 0) <= 0
