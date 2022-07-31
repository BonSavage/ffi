#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <algorithm>
#include <functional>
#include <csignal>

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif

template<class T>
struct type_info
{
};

#define export_type(T,str) \
template<> \
struct type_info<T> \
{ \
    static constexpr char* name = str;\
}; \

#define e(T,str) export_type(T,str)

template<>
struct type_info<class T*>
{

};

template<>
struct type_info<class T&>
{

};

struct dummy
{
};

template<class T>
class IteratorInterface
{
public:
    virtual T get(void) = 0;
    virtual void shift(void) = 0;
    virtual bool empty() = 0;
    virtual ~IteratorInterface<T>(void) {}
};

template<class SequenceType,class T>
class IteratorWrapper : public IteratorInterface<T>
{
    using iter = typename SequenceType::const_iterator;
public:
    IteratorWrapper<SequenceType,T>(const SequenceType& seq) : current_{seq.cbegin()},seq_{seq}  {}
    T get(void) {return *current_;}
    void shift(void) {++current_;}
    bool empty(void) {return current_ == seq_.cend();}
private:
    iter current_;
    const SequenceType& seq_;
};

e(int,":int")
e(char,":char")
e(bool,":bool")
e(short,":short")
e(void,":void")
//e(u8,":uint8")
e(dummy," ")
e(char*,":string")
e(unsigned int,":uint")

#undef export_type

#define export_type(T,str) \
template<> \
struct type_info<T*> \
{ \
    static constexpr char* name = str;\
}; \
template<> \
struct type_info<T&> \
{\
        static constexpr char name[] = "(" str " t)";\
};\

namespace ffi
{

template<class cls>
void finalizer(cls* ptr)
{
    delete ptr;
}

template<typename to>
struct smart_cast_res
{
    static to get(to* val)
    {
        return *val;
    }
    static to get(to& val)
    {
        return val;
    }
};

template<typename to>
struct smart_cast_res<to&>
{
    static to& get(to* val)
    {
        return *val;
    }
    static to& get(to& val)
    {
        return val;
    }
};

template<class to>
struct smart_cast_res<to&&>
{
    static to&& get(to* val)
    {
        return std::move(*val); //not quite elegant
    }
};

template<typename to>
struct smart_cast_res<to*>
{
/*   static to* get(typename std::remove_const<to>::type& val)
    {
        return &val;
    } */ // Danger!
    static to* get(typename std::remove_const<to>::type* val)
    {
        return val;
    }
    static to* get(to&& val)
    {
        return new to(val);
    }
    static to* get(const to& val)
    {
        return new to(val); //to beware segfault
    }
    static to* get(const to* val)
    {
        to* res = new to(std::move(*val));
        delete val;
        return res;
    }
};

template<class T>
struct process_type
{
    using value = typename std::remove_const<typename std::conditional<std::is_fundamental<T>::value,T,T*>::type>::type;
};

template<class T>
struct process_type<T&&>
{
    using value = typename std::remove_const<typename std::conditional<std::is_fundamental<T>::value,T,T*>::type>::type;
};

template<class T>
struct process_type<T&>
{
    using value = T&;
};

template<class T>
struct process_type<const T&>
{
    using value = T*;
};

template<class T>
struct process_type<T*>
{
    using value = typename std::remove_const<T>::type*;
};

template<class to,class from>
to smart_cast(from&& val)
{
    return smart_cast_res<to>::get(val);
}

template<class param = dummy,class ... params>
struct push_params
{
    static std::vector<std::string>&& call(std::vector<std::string>* vec = new std::vector<std::string>())
    {
 //       static_assert(!std::is_pointer<param>::value && !std::is_reference<param>::value);
        vec->push_back(type_info<param>::name);
        return push_params<params...>::call(vec);
    }
};

template<class param>
struct push_params<param>
{
    static std::vector<std::string>&& call(std::vector<std::string>* vec = new std::vector<std::string>())
    {
 //       static_assert(!std::is_pointer<param>::value && !std::is_reference<param>::value);
        vec->push_back(type_info<param>::name);
        return std::move(*vec);
    }
};



char* translate_string(std::string& ,const std::vector<std::string>&);

template<class cls,class ... T_List>
cls* constructor(typename process_type<T_List>::value ... args)
{
    return new cls(smart_cast<T_List>(args)...);
}

struct test_struct
{
    char& ch;
};
static_assert(sizeof(test_struct) == sizeof(char*));
template<class ret,class ... params>
struct wrap_function
{
    using ret_type = typename process_type<ret>::value;
    static_assert(!std::is_const<ret>::value);
    static_assert(!std::is_const<ret_type>::value);
    static_assert(!std::is_const<typename std::remove_pointer<ret>>::value);
//    using arg_types = typename process_type<params>::value...;
    static ret_type value(std::function<ret(params...)>* f,typename process_type<params>::value ... args)
    {
        std::signal(SIGSEGV,segf_processor);
        return smart_cast<ret_type>((*f)(smart_cast<params>(args)...));
    }
};


template<class ... params>
struct wrap_void_function
{
    using ret_type = void;
    static void value(std::function<void(params...)>* f,typename process_type<params>::value ... args)
    {
        std::signal(SIGSEGV,segf_processor);
        (*f)(smart_cast<params>(args)...);
    }
};

template<class T,class BASE>
struct wrap_place //CURRENTLY NOT USED
{
    using ret_type = typename process_type<T>::value;
    static ret_type value(T BASE::*p,BASE* instance)
    {
        return smart_cast<ret_type>(instance->*p);
    }
};

class ExportedInterface
{
public:
    virtual char* get_fun_list(char* name) = 0;
    virtual char* get_place_list(char* name) = 0;
    virtual char* get_constructor_list() = 0;

    virtual void* get_fun_ptr(char*) = 0;
    virtual void* get_fun_wrapper(char*) = 0;
    virtual void* get_constructor() = 0;
    virtual void* get_finalizer() = 0;
    virtual short get_place(char*) = 0;

    static ExportedInterface* get_exported(char* name) {return (*(exported_.find(name))).second;}
protected:
    inline static std::map<std::string,ExportedInterface*> exported_;
};

template<class cls>
class Exported : public ExportedInterface
{
public:
    using self = Exported<cls>;

    static auto make(void)
    {
        return new Exported<cls>();
    }

    template<class ... constructor_params>
    auto add_constructor(void)
    {
        constructor_ = {(void*)&constructor<cls,constructor_params...>,push_params<typename process_type<constructor_params>::value...>::call()};
        return this;
    }

    template<class ret,class ... params>
    auto add_fun(const char* name,ret(cls::*f)(params...))
    {
        insert_function(name,new std::function<ret(cls*,params...)>(f));
        return this;
    }

    template<class ret,class ... params>
    auto add_fun(const char* name,ret(cls::*f)(params...) const)
    {
        insert_function(name,new std::function<ret(const cls*,params...)>(f));
        return this;
    }

    template<class ret,class ... params>
    auto add_fun(const char* name,ret(*f)(cls*,params...))
    {
        insert_function(name,new std::function(f));
        return this;
    }

    template<class ret,class first,class ... params>
    auto add_fun(const char* name,ret(*f)(first,params...))
    {
        using raw_first = typename std::remove_const<typename std::remove_reference<first>::type>::type;
        static_assert(std::is_same<raw_first,cls>::value);
        insert_function(name,new std::function(f));
        return this;
    }

    template<class T,class BASE>
    auto add_place(const char* name,T BASE::*n)
    {
        static_assert(std::is_base_of<BASE,cls>::value);
        place_info place = {type_info<T>::name,*((short*)&n)};
        places_.emplace(name,std::move(place));
        return this;
    }

    char* get_constructor_list(void)
    {
        std::string str = "";
        return translate_string(str,constructor_.args);
    }

    char* get_fun_list(char* name)
    {
        method_info& info = methods_.find(name)->second;
        std::string lst = info.return_name;
        return translate_string(lst,info.args);
    }

    char* get_place_list(char* name)
    {
        place_info info = places_.find(name)->second;
        std::string lst = std::to_string(info.offset) + " " + info.type;
        char* res = new char[lst.length() + 1];
        std::strcpy(res,lst.c_str());
        return res;
    }

    void* get_fun_ptr(char* name)
    {
        return methods_.find(name)->second.adress;
    }

    void* get_finalizer()
    {
        return (void*)&finalizer<cls>;
    }

    void* get_constructor()
    {
        return constructor_.adress;
    }

    void* get_fun_wrapper(char* name)
    {
        return methods_.find(name)->second.wrapper;
    }

    short get_place(char* name)
    {
        return places_.find(name)->second.offset;
    }

    struct place_info
    {
//        char* name; //name of the place
        const char* const type; //name of the type
        short offset; //offset to access class member
    };
    struct method_info
    {
//        const char* const name; //name of the method
        const char* const return_name; //name of the return type
        void* wrapper; //the function that invokes std::function
        void* adress; //pointer to std::function
        std::vector<std::string> args; //names of the types of the args
    };

private:

    template<class first,class ... params>
    void insert_function(const char* name,std::function<void(first,params...)>* f)
    {
        using wrapped = wrap_void_function<first,params...>;
//        static_assert(std::is_same<cls*,process_type<first>::value>::value);
        method_info method = {type_info<void>::name,(void*)&wrapped::value,(void*)f,push_params<typename process_type<params>::value...>::call()};
        methods_.emplace(name,std::move(method));
    }

    template<class ret,class first,class ... params>
    void insert_function(const char* name,std::function<ret(first,params...)>* f)
    {
        using wrapped = wrap_function<ret,first,params...>;
        using ret_type = typename wrapped::ret_type;
//        static_assert(std::is_same<cls*,process_type<first>::value>::value);
        method_info method = {type_info<ret_type>::name,(void*)&wrapped::value,(void*)f,push_params<typename process_type<params>::value...>::call()};
        methods_.emplace(name,std::move(method));
    }

    Exported(void)
    {
        std::string str(type_info<cls*>::name);
        exported_.emplace(str,static_cast<ExportedInterface*>(this));
    }

    struct {
        void* adress;
        std::vector<std::string> args;
    } constructor_;
    std::map<std::string,place_info> places_;
    std::map<std::string,method_info> methods_;
};

}
