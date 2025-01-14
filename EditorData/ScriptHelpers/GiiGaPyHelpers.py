import inspect


def get_subclass_name_in_module(module, base_type) -> str:
    for name, obj in inspect.getmembers(module):
        if (
            inspect.isclass(obj)
            and issubclass(obj, base_type)
            and obj.__module__ == module.__name__
        ):
            return name
        
def IsEqOrSubClass(cls, base_type)-> bool:
    return type(cls) == type(base_type) or issubclass(cls,base_type)


# Example usage:
# import your_module
# class_names = get_matching_classes(your_module, BaseClass)
# print(class_names)
