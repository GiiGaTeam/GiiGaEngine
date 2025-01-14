from __future__ import annotations
from _frozen_importlib import BuiltinImporter as __loader__
import typing
__all__ = ['Component', 'GameObject', 'Transform', 'TransformComponent', 'Vector3']
class Component:
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def Init(self) -> None:
        ...
    def Tick(self, arg0: float) -> None:
        ...
    def __init__(self) -> None:
        ...
    @property
    def owner(self) -> GameObject:
        ...
    @owner.setter
    def owner(self, arg1: ...) -> None:
        ...
class GameObject:
    name: str
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def GetTransformComponent(self) -> ...:
        ...
class Transform:
    __hash__: typing.ClassVar[None] = None
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def GetForward(self) -> Vector3:
        ...
    def GetMatrix(self) -> ...:
        ...
    def GetRight(self) -> Vector3:
        ...
    def GetRotation(self) -> Vector3:
        ...
    def GetUp(self) -> Vector3:
        ...
    def SetRotation(self, arg0: Vector3) -> None:
        ...
    def __eq__(self, arg0: Transform) -> bool:
        ...
    @typing.overload
    def __init__(self, loc: Vector3 = ..., rot: Vector3 = ..., scale: Vector3 = ...) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: Transform) -> None:
        ...
    def __ne__(self, arg0: Transform) -> bool:
        ...
class TransformComponent(Component):
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def AddLocation(self, arg0: Vector3) -> None:
        ...
    def AddRotation(self, arg0: Vector3) -> None:
        ...
    def AddScale(self, arg0: Vector3) -> None:
        ...
    def AddWorldLocation(self, arg0: Vector3) -> None:
        ...
    def AddWorldRotation(self, arg0: Vector3) -> None:
        ...
    def AddWorldScale(self, arg0: Vector3) -> None:
        ...
    def GetInverseLocalMatrix(self) -> ...:
        ...
    def GetInverseWorldMatrix(self) -> ...:
        ...
    def GetLocalMatrix(self) -> ...:
        ...
    def GetLocation(self) -> Vector3:
        ...
    def GetParent(self) -> ...:
        ...
    def GetRotation(self) -> Vector3:
        ...
    def GetScale(self) -> Vector3:
        ...
    def GetTransform(self) -> Transform:
        ...
    def GetWorldLocation(self) -> Vector3:
        ...
    def GetWorldMatrix(self) -> ...:
        ...
    def GetWorldRotation(self) -> Vector3:
        ...
    def GetWorldScale(self) -> Vector3:
        ...
    def GetWorldTransform(self) -> Transform:
        ...
    def SetLocation(self, arg0: Vector3) -> None:
        ...
    def SetRotation(self, arg0: Vector3) -> None:
        ...
    def SetScale(self, arg0: Vector3) -> None:
        ...
    def SetTransform(self, arg0: Transform) -> None:
        ...
    def SetWorldLocation(self, arg0: Vector3) -> None:
        ...
    def SetWorldRotation(self, arg0: Vector3) -> None:
        ...
    def SetWorldScale(self, arg0: Vector3) -> None:
        ...
    def SetWorldTransform(self, arg0: Transform) -> None:
        ...
    def __init__(self) -> None:
        ...
class Vector3:
    Forward: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    One: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    Right: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    Up: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    Zero: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    __hash__: typing.ClassVar[None] = None
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, arg0: Vector3) -> bool:
        ...
    def __iadd__(self, arg0: Vector3) -> Vector3:
        ...
    @typing.overload
    def __imul__(self, arg0: Vector3) -> Vector3:
        ...
    @typing.overload
    def __imul__(self, arg0: float) -> Vector3:
        ...
    @typing.overload
    def __init__(self) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: float) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: float, arg1: float, arg2: float) -> None:
        ...
    def __isub__(self, arg0: Vector3) -> Vector3:
        ...
    def __itruediv__(self, arg0: float) -> Vector3:
        ...
    def __ne__(self, arg0: Vector3) -> bool:
        ...
    def __neg__(self) -> Vector3:
        ...
    def clamp(self, arg0: Vector3, arg1: Vector3) -> None:
        ...
    def cross(self, arg0: Vector3) -> Vector3:
        ...
    def dot(self, arg0: Vector3) -> float:
        ...
    def length(self) -> float:
        ...
    def length_squared(self) -> float:
        ...
    def normalize(self) -> None:
        ...
