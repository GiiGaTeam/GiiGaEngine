from typing import List
import GiiGaPy as gp
import GiiGaPy.GOAP as gpGoap
from .BlackBoard import BlackBoard
from .GOAPBrain import GOAPBrain
from .PyAction import PyAction, ActionState
from . import Predicates as preds

import random as rand


class Agent(gp.Component):
    def __init__(self):
        super().__init__()
        self.blackboard = BlackBoard()
        self.blackboard.update(
            {
                "HP": 100,
                "Enemy": None,
                "ammo": 2,  # Начальное кол-во патронов
                "cooldown_remaining": 0,
            }
        )

        actions = [
            SearchForEnemyAction(),
            AimAction(),
            ShootAction(),
            ReloadAction(),
            FleeAction(),
            HealAction(),
            DodgeAction(),
        ]

        goal = gpGoap.WorldState({preds.target_dead.__name__: True})
        self.brain = GOAPBrain(goal=goal, bb=self.blackboard, actions=actions)

    def Init(self):
        print("Agent Init", flush=True)

    def BeginPlay(self):
        print("Agent BeginPlay", flush=True)

    def Tick(self, dt):
        if self.blackboard["cooldown_remaining"] > 0:
            self.blackboard["cooldown_remaining"] -= 1
        self.brain.Tick()


class SearchForEnemyAction(PyAction):
    def __init__(self):
        preconditions = {}
        effects = {preds.EnemySet: True}
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard):
        return True  # No preconditions needed

    def Tick(self, bb: BlackBoard):
        # Assume sphere trace finds enemy
        bb["Enemy"] = "EnemyObject"

        print("SearchForEnemyAction Ticked", flush=True)
        return ActionState.Completed


class AimAction(PyAction):
    def __init__(self):
        preconditions = {preds.EnemySet: True, preds.IsAimed: False}
        effects = {preds.IsAimed: True}
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard) -> bool:
        return preds.EnemySet(bb) and not preds.IsAimed(bb)

    def Tick(self, bb: BlackBoard) -> ActionState:
        bb["is_aimed"] = True
        print("AimAction Ticked", flush=True)
        return ActionState.Completed


class ShootAction(PyAction):
    def __init__(self):
        preconditions = {
            preds.EnemySet: True,
            preds.IsAimed: True,
            preds.HasAmmo: True,
            preds.CanShoot: True,
        }
        effects = {
            preds.target_dead: True
        }  # Условный эффект (убивает с одного выстрела)
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard) -> bool:
        return (
            preds.EnemySet(bb)
            and preds.IsAimed(bb)
            and bb.get("ammo", 0) > 0
            and bb.get("cooldown_remaining", 0) <= 0
        )

    def Tick(self, bb: BlackBoard) -> ActionState:
        bb["ammo"] -= 1
        bb["cooldown_remaining"] = 3  # Задержка 3 тика между выстрелами

        if rand.randint(0, 10) > 7:
            print("- hp", flush=True)
            bb["HP"] -= rand.randint(0, 10)

        if rand.randint(0, 10) > 7:
            print("target_dead", flush=True)
            bb["target_dead"] = True
            bb["Enemy"] = None

        print("ShootAction Ticked", flush=True)
        return ActionState.Completed


class ReloadAction(PyAction):
    def __init__(self):
        preconditions = {preds.HasAmmo: False}
        effects = {preds.HasAmmo: True, preds.CanShoot: True}
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard) -> bool:
        return bb.get("ammo", 0) == 0

    def Tick(self, bb: BlackBoard) -> ActionState:
        # Перезарядка занимает 2 тика
        if "reload_progress" not in bb:
            bb["reload_progress"] = 0

        bb["reload_progress"] += 1
        if bb["reload_progress"] >= 2:
            bb["ammo"] = 5  # Восстанавливаем патроны
            del bb["reload_progress"]
            return ActionState.Completed

        print("ShootAction Ticked", flush=True)
        return ActionState.InProgress


class FleeAction(PyAction):
    def __init__(self):
        preconditions = {
            preds.EnemySet: True,
            preds.IsHighHP: False,
        }  # Flee only when enemy present and HP is low
        effects = {preds.EnemySet: False}  # Removes enemy from blackboard
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard):
        return preds.EnemySet(bb) and not preds.IsHighHP(bb)

    def Tick(self, bb: BlackBoard):
        bb["Enemy"] = None
        if "is_healing" in bb:
            del bb["is_healing"]
        if "healing_progress" in bb:
            del bb["healing_progress"]

        print("ShootAction Ticked", flush=True)
        return ActionState.Completed


class DodgeAction(PyAction):
    def __init__(self):
        preconditions = {
            preds.EnemySet: True,
            preds.CanShoot: False,
        }
        effects = {preds.CanShoot: True}
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard):
        return preds.EnemySet(bb) and not preds.CanShoot(bb)

    def Tick(self, bb: BlackBoard):
        print("DodgeAction Ticked", flush=True)
        return ActionState.InProgress


class HealAction(PyAction):
    def __init__(self):
        preconditions = {
            preds.EnemySet: False,
            preds.IsHighHP: False,
        }  # Heal only when safe and HP is low
        effects = {preds.IsHighHP: True}  # Restores HP to high
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard):
        return not preds.EnemySet(bb) and not preds.IsHighHP(bb)

    def Tick(self, bb: BlackBoard):
        if not bb.get("is_healing", False):
            # Initialize healing state
            bb["is_healing"] = True
            bb["healing_progress"] = 0

        # Increment HP gradually (20 per tick)
        current_hp = bb.get("HP", 0)
        bb["HP"] = min(current_hp + 20, 100)
        bb["healing_progress"] += 1

        print("ShootAction Ticked", flush=True)

        if bb["HP"] >= 100:
            # Cleanup and mark healing complete
            del bb["is_healing"], bb["healing_progress"]
            return ActionState.Completed
        else:
            return ActionState.InProgress
