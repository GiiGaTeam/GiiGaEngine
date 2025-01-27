import GiiGaPy as gp
import random as rand
import math
from .PyBeh2 import MyPyBeh2


class MyPyBeh1(gp.Component):
    def __init__(self):
        super().__init__()
        self.time = 0
        self.created = False
        print("MyPyBeh1 Im Alive", flush=True)

    def Init(self):
        print("MyPyBeh1 Init", flush=True)

    def BeginPlay(self):
        # print("MyPyBeh1 BeginPlay start",flush=True)
        # if self.cam is not None:
        #    gp.Engine.Instance().RenderSystem().SetCamera(self.cam)
        # print("MyPyBeh1 BeginPlay end",flush=True)
        pass

    def Tick(self, dt: float):
        if self.time < 3:
            self.time += dt
        if self.time > 3 and not self.created:
            self.created = True
            sp = gp.SpawnParameters()
            sp.name = "Name"
            sp.owner = self.owner
            new_go: gp.GameObject = gp.GameObject.CreateEmptyGameObject(sp)
            # new_comp: MyPyBeh2 = MyPyBeh2()
            # new_go.AddComponent(new_comp)
            # new_comp.RegisterInWorld()
            new_comp = new_go.CreateComponent(MyPyBeh2)
        pass
