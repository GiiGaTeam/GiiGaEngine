from typing import List
import GiiGaPy as gp
import GiiGaPy.GOAP as gpGoap
from .BlackBoard import BlackBoard
from .GOAPBrain import GOAPBrain
from .PyAction import PyAction
from . import Predicates as preds


class Agent(gp.Component):
    def __init__(self):
        super().__init__()

        actions: List[PyAction] = []

        actions.append(
            PyAction(
                precond={
                    preds.target_acquired: False,
                    preds.target_lost: True,
                },
                effects={preds.target_acquired: True},
            )
        )

        actions.append(
            PyAction(
                precond={
                    preds.target_acquired: False,
                    preds.target_lost: False,
                },
                effects={preds.target_acquired: True},
            )
        )

        actions.append(
            PyAction(
                precond={
                    preds.target_acquired: True,
                    preds.target_dead: False,
                },
                effects={preds.target_in_warhead_range: True},
            )
        )

        actions.append(
            PyAction(
                precond={
                    preds.target_in_warhead_range: True,
                    preds.target_acquired: True,
                    preds.target_dead: False,
                },
                effects={preds.target_dead: True},
            )
        )

        goal = gpGoap.WorldState({preds.target_dead.__name__: True})

        self.blackboard = BlackBoard()
        self.brain = GOAPBrain(goal=goal, bb=self.blackboard, actions=actions)

    def Init(self):
        print("Agent Init", flush=True)

    def BeginPlay(self):
        print("Agent BeginPlay", flush=True)

    def Tick(self, dt):
        self.brain.Tick()
