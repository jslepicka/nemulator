#pragma once
#include "..\mapper.h"

namespace nes
{

class c_mapper0 : public c_mapper, register_class<c_mapper_registry, c_mapper0>
{
  public:
    static std::vector<c_mapper::s_mapper_info> get_registry_info()
    {
        return {{
            .number = 0,
            .name = "NROM",
            .constructor = []() { return std::make_unique<c_mapper0>(); },
        }};
    }
};

} //namespace nes