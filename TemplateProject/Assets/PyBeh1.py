import GiiGaPy as gp
import random as rand

class MyPyBeh1(gp.Component):
    speed: int = 0
    vec: gp.Vector3 = gp.Vector3()
    def __init__(self):
        super().__init__()
        print("MyPyBeh1 Im Alive", flush=True)

    def Init(self):
        print("MyPyBeh1 Init", flush=True)
        
    def BeginPlay(self):
        print("MyPyBeh1 BeginPlay", flush=True)

    def Tick(self, dt: float):
        self.owner.GetTransformComponent().AddLocation(
            gp.Vector3(
                dt*rand.randrange(-2,2),
                dt*rand.randrange(-2,2),
                dt*rand.randrange(-2,2)))
