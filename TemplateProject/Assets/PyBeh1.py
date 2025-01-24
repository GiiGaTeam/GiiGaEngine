import GiiGaPy as gp
import random as rand
import math

class MyPyBeh1(gp.Component):
    speed: float = 0
    cam: gp.CameraComponent = None
    def __init__(self):
        super().__init__()
        self.time = 0
        print("MyPyBeh1 Im Alive", flush=True)

    def Init(self):
        print("MyPyBeh1 Init", flush=True)
        
    def BeginPlay(self):
        #print("MyPyBeh1 BeginPlay start",flush=True)
        #if self.cam is not None:
        #    gp.Engine.Instance().RenderSystem().SetCamera(self.cam)
        #print("MyPyBeh1 BeginPlay end",flush=True)
        pass

    def Tick(self, dt: float):
        pass