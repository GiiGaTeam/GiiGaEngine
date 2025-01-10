import GiiGaPy as gp
import random as rand

class MyPyBeh1(gp.Component):
    def __init__(self):
        super().__init__()
        print("MyPyBeh1 Im Alive")

    def Init(self):
        print("MyPyBeh1 Init")

    def Tick(self, dt: float):
        self.owner.GetTransformComponent().AddLocation(
            gp.Vector3(
                dt*rand.randrange(-2,2),
                dt*rand.randrange(-2,2),
                dt*rand.randrange(-2,2)))
