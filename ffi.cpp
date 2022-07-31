#include "ffi.h"
using namespace ffi;

char* lisp::translate_string(std::string& lst,const std::vector<std::string>& vec)
{
        for (std::string x : vec) {
            lst.push_back(' ');
            lst += x;
        }
        char* new_str = new char[lst.length()+1];
        std::strcpy(new_str,lst.c_str());
        return new_str;
}

extern "C"
{

DLL_EXPORT char* get_place_list(ExportedInterface* cls,char* name)
{
    return cls->get_place_list(name);
}

DLL_EXPORT char* get_method_list(ExportedInterface* cls,char* name)
{
    return cls->get_fun_list(name);

}

DLL_EXPORT char* get_constructor_list(ExportedInterface* cls)
{
    return cls->get_constructor_list();
}

DLL_EXPORT ExportedInterface* get_class_ptr(char* name)
{
    return ExportedInterface::get_exported(name);
}

DLL_EXPORT void* get_method_ptr(ExportedInterface* cls,char* name)
{
    return cls->get_fun_ptr(name);
}

DLL_EXPORT void* get_method_wrapper(ExportedInterface* cls,char* name)
{
    return cls->get_fun_wrapper(name);
}

DLL_EXPORT void* get_constructor(ExportedInterface* cls)
{
    return cls->get_constructor();
}

DLL_EXPORT void* get_finalizer(ExportedInterface* cls)
{
    return cls->get_finalizer();
}

}
