import GiiGaPy as gp
import random as rand
import math


class Projectile(gp.Component):

    def __init__(self):
        super().__init__()
        print("Projectile Im Alive", flush=True)

    def Init(self):
        print("Projectile Init", flush=True)
        self.new_col: gp.CollisionComponent = self.CreateCollisionComponent()
        self.new_col.Layer = gp.Layer.Moving
        self.new_col.MotionType = gp.EMotionType.Dynamic
        self.owner.GetTransformComponent().SetScale(0.1, 0.1, 0.1)

    def BeginPlay(self):
        self.new_col.AddVelocity(
            self.owner.GetTransformComponent().Forward().MulFloat(10)
        )

    def Tick(self, dt: float):
        pass
