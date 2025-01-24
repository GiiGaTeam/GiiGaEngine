from . import Predicates as pred
from .BlackBoard import BlackBoard
from GiiGaPy.GOAP import Action
from enum import Enum


class ActionState(Enum):
    """Docstring for ActionState."""

    InProgress = "InProgress"
    Abort = "Abort"
    Completed = "Completed"


class PyAction(Action):
    def __init__(
        self,
        precond: dict[pred.pred_type, bool],
        effects: dict[pred.pred_type, bool],
    ):
        precond_str_bool: dict[str, bool] = {
            pred.__name__: val for pred, val in precond.items()
        }
        effects_str_bool: dict[str, bool] = {
            pred.__name__: val for pred, val in effects.items()
        }

        super().__init__(precond_str_bool, effects_str_bool, 1)

        self.preconditions: dict[pred.pred_type, bool] = precond
        self.effects: dict[pred.pred_type, bool] = effects
        ##### example ####
        """
        self.preconditions[pred.EnemySet] = True
        self.preconditions[pred.IsHighHP] = True
        self.effects[pred.IsHighHP] = False"""

    def CheckPreconditions(self, bb: BlackBoard):
        return True

    def Tick(self, bb: BlackBoard):
        return ActionState.Completed
