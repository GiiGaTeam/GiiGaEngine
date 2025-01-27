import GiiGaPy as gp
import random as rand
import math


class MyPyBeh1(gp.Component):
    mesh_handl: gp.AssetHandle = None
    mat_handl: gp.AssetHandle = None

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
            new_comp: gp.StaticMeshComponent = new_go.CreateComponent(
                gp.StaticMeshComponent
            )
            new_comp.MeshHandle = self.mesh_handl
            new_comp.MaterialHandle = self.mat_handl
            new_go.GetTransformComponent().SetLocation(gp.Vector3(0, 3, 0))
