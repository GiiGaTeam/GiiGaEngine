import inspect
import json
import GiiGaPy as gp

'''
class CustomEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, gp.Vector3):
            return gp.Vector3ToJson(obj)
        elif isinstance(obj, gp.JsonValue):
            return obj.toStyledString()
        return super().default(obj)
'''
    
class CustomDecoder(json.JSONDecoder):
    def default(self, s):
        if isinstance(s, gp.Vector3):
            return gp.Vector3FromJson(s)
        if isinstance(s, gp.JsonValue):
            return gp.JsonValue.FromStyledString(s)
        return super().default(s)

def get_subclass_name_in_module(module, base_type) -> str:
    for name, obj in inspect.getmembers(module):
        if (
            inspect.isclass(obj)
            and issubclass(obj, base_type)
            and obj.__module__ == module.__name__
        ):
            return name
        
def IsEqOrSubClass(cls, base_type)-> bool:
    return cls == base_type or issubclass(cls,base_type)

def EncodeToJSONStyledString(obj: object)->str:
    
    if isinstance(obj, gp.Vector3):
        return gp.Vector3ToJson(obj).toStyledString()
    
    enc = json.JSONEncoder()
    return enc.encode(obj)

def DecodeFromJSON(s: str)->object:
    dec = CustomDecoder()
    return dec.decode(s)


# Example usage:
# import your_module
# class_names = get_matching_classes(your_module, BaseClass)
# print(class_names)
