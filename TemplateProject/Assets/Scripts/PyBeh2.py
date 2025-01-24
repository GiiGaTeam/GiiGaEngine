import GiiGaPy as gp
import random as rand
from PyBeh1 import MyPyBeh1 

class MyPyBeh2(gp.Component):
    speed: float = 0
    beh1: MyPyBeh1 = None
    def __init__(self):
        super().__init__()
        print("MyPyBeh2 Im Alive", flush=True)

    def Init(self):
        print("MyPyBeh2 Init", flush=True)
        
    def BeginPlay(self):
        print("MyPyBeh2 BeginPlay", flush=True)

    def Tick(self, dt: float):
        #print("MyPyBeh2 say: beh1 owner name is" + self.beh1.owner.name,flush=True)
        pass