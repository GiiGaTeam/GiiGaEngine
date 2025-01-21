import GiiGaPy as gp
import random as rand
import math

class MyPyBeh1(gp.Component):
    speed: float = 0
    trans: gp.TransformComponent = None
    def __init__(self):
        super().__init__()
        self.time = 0
        print("MyPyBeh1 Im Alive", flush=True)

    def Init(self):
        print("MyPyBeh1 Init", flush=True)
        
    def BeginPlay(self):
        print("MyPyBeh1 BeginPlay", flush=True)

    def Tick(self, dt: float):
        self.time += dt
        trans_loc = self.trans.GetLocation()
        self.owner.GetTransformComponent().SetLocation(gp.Vector3(self.speed * math.sin(self.time) + trans_loc.x, trans_loc.y, trans_loc.z))