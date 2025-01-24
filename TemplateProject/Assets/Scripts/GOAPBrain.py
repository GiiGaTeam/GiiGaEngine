from typing import List
import GiiGaPy.GOAP as gpGoap
from .BlackBoard import BlackBoard
from .PyAction import PyAction, ActionState


class GOAPBrain:
    def __init__(
        self, goal: gpGoap.WorldState, bb: BlackBoard, actions: List[PyAction]
    ):
        self.actions = actions
        self.bb = bb
        self.currentGoal = goal
        self.currentPlan: List[PyAction] = []

    def CurrentActionCanBeDone(self) -> bool:
        return self.currentPlan[0].CheckPreconditions(self.bb)

    def TickCurrentAction(self):
        act_state = self.currentPlan[0].Tick(self.bb)
        if act_state == ActionState.Completed:
            print("pop", flush=True)
            self.currentPlan.pop(0)
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
        world_state.print()
        self.currentPlan = gpGoap.Planner().plan(
            world_state,
            self.currentGoal,
            self.actions,
        )
        if len(self.currentPlan) != 0:
            print(*self.currentPlan, flush=True)
