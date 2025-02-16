#pragma once

template <typename T> class c_class_registry
{
  public:
    static T &get_registry()
    {
        static T registry;
        return registry;
    }
};

template <typename registry, typename class_name> class register_class
{
    static void _register()
    {
        registry::_register(class_name::get_registry_info());
    }

    struct s_registrar
    {
        s_registrar()
        {
            _register();
        }
    };
    
    static inline s_registrar registrar;
};