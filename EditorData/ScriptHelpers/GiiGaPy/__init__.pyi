from __future__ import annotations
from _frozen_importlib import BuiltinImporter as __loader__
import typing
from . import GOAP
__all__ = ['CameraComponent', 'CollideInfo', 'CollisionComponent', 'Component', 'Dynamic', 'EMotionType', 'Engine', 'GOAP', 'GameObject', 'ICollision', 'Input', 'JsonValue', 'KeyA', 'KeyCode', 'KeyD', 'KeyE', 'KeyQ', 'KeyS', 'KeySpace', 'KeyW', 'Kinematic', 'Layer', 'Left', 'MouseButton', 'Moving', 'NoMoving', 'RenderSystem', 'Right', 'ShapeCast', 'ShapeCastResult', 'SpawnParameters', 'Static', 'Transform', 'TransformComponent', 'Trigger', 'Uuid', 'Vector3', 'Vector3FromJson', 'Vector3ToJson']
class CameraComponent(Component):
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class CollideInfo:
    baseOffset: Vector3
    depthPenetration: float
    normal: Vector3
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class CollisionComponent(ICollision):
    Layer: Layer
    MotionType: EMotionType
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def AddForce(self, arg0: Vector3) -> None:
        ...
    def AddImpulse(self, arg0: Vector3) -> None:
        ...
    def AddVelocity(self, arg0: Vector3) -> None:
        ...
    def __init__(self) -> None:
        ...
    @property
    def owner(self) -> GameObject:
        ...
    @owner.setter
    def owner(self, arg1: ...) -> None:
        ...
class Component:
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def BeginPlay(self) -> None:
        ...
    def Destroy(self) -> None:
        ...
    def Init(self) -> None:
        ...
    def OnBeginOverlap(self, arg0: ..., arg1: ...) -> None:
        ...
    def OnEndOverlap(self, arg0: ...) -> None:
        ...
    def OnOverlapping(self, arg0: ..., arg1: ...) -> None:
        ...
    def RegisterInWorld(self) -> None:
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
class EMotionType:
    """
    Members:
    
      Dynamic
    
      Kinematic
    
      Static
    """
    Dynamic: typing.ClassVar[EMotionType]  # value = <EMotionType.Dynamic: 2>
    Kinematic: typing.ClassVar[EMotionType]  # value = <EMotionType.Kinematic: 1>
    Static: typing.ClassVar[EMotionType]  # value = <EMotionType.Static: 0>
    __members__: typing.ClassVar[dict[str, EMotionType]]  # value = {'Dynamic': <EMotionType.Dynamic: 2>, 'Kinematic': <EMotionType.Kinematic: 1>, 'Static': <EMotionType.Static: 0>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class Engine:
    @staticmethod
    def Instance() -> Engine:
        ...
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def RenderSystem(self) -> RenderSystem:
        ...
class GameObject:
    name: str
    @staticmethod
    def CreateEmptyGameObject(arg0: SpawnParameters) -> GameObject:
        ...
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def AddChild(self, arg0: GameObject) -> None:
        ...
    def CreateComponent(self, arg0: typing.Any, *args, **kwargs) -> ...:
        """
        First arg Any Component subclass type, than args and kwargs
        """
    def Destroy(self) -> None:
        ...
    def GetChildren(self) -> list[GameObject]:
        ...
    def GetTransformComponent(self) -> ...:
        """
        Returns TransformComponent type
        """
    def RemoveChild(self, arg0: GameObject) -> None:
        ...
    def SetParent(self, arg0: GameObject, arg1: bool) -> None:
        ...
class ICollision(TransformComponent):
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def AddForce(self, arg0: Vector3) -> None:
        ...
class Input:
    @staticmethod
    def GetMouseDelta() -> ...:
        ...
    @staticmethod
    @typing.overload
    def IsKeyHeld(arg0: MouseButton) -> bool:
        ...
    @staticmethod
    @typing.overload
    def IsKeyHeld(arg0: KeyCode) -> bool:
        ...
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
class JsonValue:
    @staticmethod
    def FromStyledString(arg0: str) -> JsonValue:
        ...
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    @typing.overload
    def __init__(self) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: int) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: float) -> None:
        ...
    @typing.overload
    def __init__(self, arg0: str) -> None:
        ...
class KeyCode:
    """
    Members:
    
      KeyW
    
      KeyA
    
      KeyS
    
      KeyD
    
      KeyE
    
      KeyQ
    
      KeySpace
    """
    KeyA: typing.ClassVar[KeyCode]  # value = <KeyCode.KeyA: 1>
    KeyD: typing.ClassVar[KeyCode]  # value = <KeyCode.KeyD: 4>
    KeyE: typing.ClassVar[KeyCode]  # value = <KeyCode.KeyE: 5>
    KeyQ: typing.ClassVar[KeyCode]  # value = <KeyCode.KeyQ: 17>
    KeyS: typing.ClassVar[KeyCode]  # value = <KeyCode.KeyS: 19>
    KeySpace: typing.ClassVar[KeyCode]  # value = <KeyCode.KeySpace: 59>
    KeyW: typing.ClassVar[KeyCode]  # value = <KeyCode.KeyW: 23>
    __members__: typing.ClassVar[dict[str, KeyCode]]  # value = {'KeyW': <KeyCode.KeyW: 23>, 'KeyA': <KeyCode.KeyA: 1>, 'KeyS': <KeyCode.KeyS: 19>, 'KeyD': <KeyCode.KeyD: 4>, 'KeyE': <KeyCode.KeyE: 5>, 'KeyQ': <KeyCode.KeyQ: 17>, 'KeySpace': <KeyCode.KeySpace: 59>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class Layer:
    """
    Members:
    
      Moving
    
      Trigger
    
      NoMoving
    """
    Moving: typing.ClassVar[Layer]  # value = <Layer.Moving: 1>
    NoMoving: typing.ClassVar[Layer]  # value = <Layer.NoMoving: 0>
    Trigger: typing.ClassVar[Layer]  # value = <Layer.Trigger: 2>
    __members__: typing.ClassVar[dict[str, Layer]]  # value = {'Moving': <Layer.Moving: 1>, 'Trigger': <Layer.Trigger: 2>, 'NoMoving': <Layer.NoMoving: 0>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class MouseButton:
    """
    Members:
    
      Left
    
      Right
    """
    Left: typing.ClassVar[MouseButton]  # value = <MouseButton.Left: 1>
    Right: typing.ClassVar[MouseButton]  # value = <MouseButton.Right: 2>
    __members__: typing.ClassVar[dict[str, MouseButton]]  # value = {'Left': <MouseButton.Left: 1>, 'Right': <MouseButton.Right: 2>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class RenderSystem:
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def SetCamera(self, arg0: ...) -> None:
        ...
class ShapeCastResult:
    collisionComponent: CollisionComponent
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
class SpawnParameters:
    name: str
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
    @property
    def owner(self) -> ...:
        ...
    @owner.setter
    def owner(self, arg1: ...) -> None:
        ...
class Transform:
    __hash__: typing.ClassVar[None] = None
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def GetForward(self) -> Vector3:
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
    def Forward(self) -> Vector3:
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
class Uuid:
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self) -> None:
        ...
    def __str__(self) -> str:
        ...
class Vector3:
    Forward: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    One: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    Right: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    Up: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    Zero: typing.ClassVar[Vector3]  # value = <GiiGaPy.Vector3 object>
    __hash__: typing.ClassVar[None] = None
    x: float
    y: float
    z: float
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __add__(self, arg0: Vector3) -> Vector3:
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
    @typing.overload
    def __init__(self, arg0: JsonValue) -> None:
        ...
    def __isub__(self, arg0: Vector3) -> Vector3:
        ...
    def __itruediv__(self, arg0: float) -> Vector3:
        ...
    def __mul__(self, arg0: float) -> Vector3:
        ...
    def __ne__(self, arg0: Vector3) -> bool:
        ...
    def __neg__(self) -> Vector3:
        ...
    def __rmul__(self, arg0: float) -> Vector3:
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
def ShapeCast(arg0: float, arg1: Vector3, arg2: Vector3) -> ShapeCastResult:
    ...
def Vector3FromJson(arg0: JsonValue) -> Vector3:
    ...
def Vector3ToJson(arg0: Vector3) -> JsonValue:
    ...
Dynamic: EMotionType  # value = <EMotionType.Dynamic: 2>
KeyA: KeyCode  # value = <KeyCode.KeyA: 1>
KeyD: KeyCode  # value = <KeyCode.KeyD: 4>
KeyE: KeyCode  # value = <KeyCode.KeyE: 5>
KeyQ: KeyCode  # value = <KeyCode.KeyQ: 17>
KeyS: KeyCode  # value = <KeyCode.KeyS: 19>
KeySpace: KeyCode  # value = <KeyCode.KeySpace: 59>
KeyW: KeyCode  # value = <KeyCode.KeyW: 23>
Kinematic: EMotionType  # value = <EMotionType.Kinematic: 1>
Left: MouseButton  # value = <MouseButton.Left: 1>
Moving: Layer  # value = <Layer.Moving: 1>
NoMoving: Layer  # value = <Layer.NoMoving: 0>
Right: MouseButton  # value = <MouseButton.Right: 2>
Static: EMotionType  # value = <EMotionType.Static: 0>
Trigger: Layer  # value = <Layer.Trigger: 2>
