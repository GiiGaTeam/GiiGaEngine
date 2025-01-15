import inspect
import json
import GiiGaPy as gp

print("Helpers Import", flush=True)

def get_subclass_name_in_module(module, base_type) -> str:
    for name, obj in inspect.getmembers(module):
        if (
            inspect.isclass(obj)
            and issubclass(obj, base_type)
            and obj.__module__ == module.__name__
        ):
            return name
        
def get_type_with_name_in_module(module, name:str) -> object:
    attr = getattr(module, name)
    print(str(attr),flush=True)
    return attr

def IsEqOrSubClass(cls, base_type)-> bool:
    return cls == base_type or issubclass(cls,base_type)

def EncodeToJSONValue(obj: object)->gp.JsonValue:
    
    if isinstance(obj, gp.Vector3):
        return gp.Vector3ToJson(obj)
    
    if isinstance(obj, gp.Uuid):
        return gp.JsonValue(str(obj))
    
    # case for default types (int, str etc), looks strange
    return gp.JsonValue(obj)

# Example usage:
# import your_module
# class_names = get_matching_classes(your_module, BaseClass)
# print(class_names)
