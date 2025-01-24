from typing import List
import GiiGaPy.GOAP as gpGoap
from BlackBoard import BlackBoard
from PyAction import PyAction, ActionState


class GOAPBrain:
    def __init__(
        self, goal: gpGoap.WorldState, bb: BlackBoard, actions: List[PyAction]
    ):
        self.actions = actions
        self.bb = bb
        self.currentGoal = goal
        self.currentPlan: List[PyAction] = []
        self.currentActionIndex = -1

    def CurrentActionCanBeDone(self) -> bool:
        return self.currentPlan[self.currentActionIndex].CheckPreconditions(self.bb)

    def TickCurrentAction(self):
        act_state = self.currentPlan[self.currentActionIndex].Tick(self.bb)
        if act_state == ActionState.Completed:
            self.currentPlan.pop()
        elif act_state == ActionState.InProgress:
            pass
        elif act_state == ActionState.Abort:
            self.currentPlan = List[PyAction]

    def Tick(self):
        if (len(self.currentPlan) == 0) or (not self.CurrentActionCanBeDone()):
            self.MakePlan()
        else:
            self.TickCurrentAction()

    def ActionPrecondEffectToWorldState(self) -> gpGoap.WorldState:
        result = gpGoap.WorldState()  # actually as key we can use name of pred

        for action in self.actions:

            for precond_pred, _ in action.preconditions.items():
                if not result.hasKey(precond_pred.__name__):
                    result.setValue(precond_pred.__name__, precond_pred(self.bb))

            for effect_pred, _ in action.effects.items():
                if not result.hasKey(effect_pred.__name__):
                    result.setValue(effect_pred.__name__, effect_pred(self.bb))

        return result

    def MakePlan(self):
        world_state = self.ActionPrecondEffectToWorldState()
        self.currentPlan = gpGoap.Planner().plan(
            world_state,
            self.currentGoal,
            self.actions,
        )
        print(*self.currentPlan, flush=True)
