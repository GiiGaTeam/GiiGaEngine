from typing import List
import GiiGaPy as gp
import GiiGaPy.GOAP as gpGoap
from .BlackBoard import BlackBoard
from .GOAPBrain import GOAPBrain
from .PyAction import PyAction, ActionState
from . import Predicates as preds

class Agent(gp.Component):
    def __init__(self):
        super().__init__()
        self.blackboard = BlackBoard()
        self.blackboard.update({
            "HP": 100,
            "Enemy": None,
            "ammo": 2,  # Начальное кол-во патронов
            "cooldown_remaining": 0
        })
        
        actions = [
            SearchForEnemyAction(),
            AimAction(),
            ShootAction(),
            ReloadAction(),
            FleeAction(),   
            HealAction()    
        ]
        
        goal = gpGoap.WorldState({target_dead.__name__: True})
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
        effects = {EnemySet: True}
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard):
        return True  # No preconditions needed

    def Tick(self, bb: BlackBoard):
        # Assume sphere trace finds enemy
        bb["Enemy"] = "EnemyObject"
        return ActionState.Completed

class AimAction(PyAction):
    def __init__(self):
        preconditions = {EnemySet: True, IsAimed: False}
        effects = {IsAimed: True}
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard) -> bool:
        return EnemySet(bb) and not IsAimed(bb)

    def Tick(self, bb: BlackBoard) -> ActionState:
        bb["is_aimed"] = True
        return ActionState.Completed


class ShootAction(PyAction):
    def __init__(self):
        preconditions = {EnemySet: True, IsAimed: True, HasAmmo: True, CanShoot: True}
        effects = {target_dead: True}  # Условный эффект (убивает с одного выстрела)
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard) -> bool:
        return (
            EnemySet(bb) and
            IsAimed(bb) and
            bb.get("ammo", 0) > 0 and
            bb.get("cooldown_remaining", 0) <= 0
        )

    def Tick(self, bb: BlackBoard) -> ActionState:
        bb["ammo"] -= 1
        bb["cooldown_remaining"] = 3  # Задержка 3 тика между выстрелами
        
        bb["target_dead"] = True
        return ActionState.Completed

class ReloadAction(PyAction):
    def __init__(self):
        preconditions = {HasAmmo: False}
        effects = {HasAmmo: True}
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
        return ActionState.InProgress

class FleeAction(PyAction):
    def __init__(self):
        preconditions = {EnemySet: True, IsHighHP: False}  # Flee only when enemy present and HP is low
        effects = {EnemySet: False}  # Removes enemy from blackboard
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard):
        return EnemySet(bb) and not IsHighHP(bb)

    def Tick(self, bb: BlackBoard):
        bb["Enemy"] = None
        if "is_healing" in bb:
            del bb["is_healing"]
        if "healing_progress" in bb:
            del bb["healing_progress"]
        return ActionState.Completed

class HealAction(PyAction):
    def __init__(self):
        preconditions = {EnemySet: False, IsHighHP: False}  # Heal only when safe and HP is low
        effects = {IsHighHP: True}  # Restores HP to high
        super().__init__(preconditions, effects)

    def CheckPreconditions(self, bb: BlackBoard):
        return not EnemySet(bb) and not IsHighHP(bb)

    def Tick(self, bb: BlackBoard):
        if not bb.get("is_healing", False):
            # Initialize healing state
            bb["is_healing"] = True
            bb["healing_progress"] = 0

        # Increment HP gradually (20 per tick)
        current_hp = bb.get("HP", 0)
        bb["HP"] = min(current_hp + 20, 100)
        bb["healing_progress"] += 1

        if bb["HP"] >= 100:
            # Cleanup and mark healing complete
            del bb["is_healing"], bb["healing_progress"]
            return ActionState.Completed
        else:
            return ActionState.InProgress

def IsHighHP(bb: BlackBoard) -> bool:
    return bb.get("HP", 0) > 50

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