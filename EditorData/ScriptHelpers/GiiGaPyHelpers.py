import inspect
import json
import GiiGaPy as gp

print("Helpers Import", flush=True)

class CustomDecoder(json.JSONDecoder):
    def __init__(self, *args, **kwargs):
        json.JSONDecoder.__init__(self, object_hook=self.object_hook, *args, **kwargs)
    def object_hook(self, dct):
        if 'Vector3' in dct:
            #not like this 
            js = gp.JsonValue.FromStyledString(str(dct))
            return gp.Vector3FromJson(js)
        return dct

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

def EncodeToJSONStyledString(obj: object)->gp.JsonValue:
    
    if isinstance(obj, gp.Vector3):
        return gp.Vector3ToJson(obj)
    
    # case for default types (int, str etc), looks strange
    return gp.JsonValue.FromStyledString(json.JSONEncoder().encode(obj))

def DecodeFromJSON(s: gp.JsonValue)->object:
    dec = CustomDecoder()
    return dec.decode(s)


# Example usage:
# import your_module
# class_names = get_matching_classes(your_module, BaseClass)
# print(class_names)
