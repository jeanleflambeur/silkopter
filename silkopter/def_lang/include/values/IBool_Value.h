#pragma once

#include "Result.h"
#include "IValue.h"
#include "ITemplate_Argument.h"

namespace ts
{

class IBool_Type;

class IBool_Value : virtual public IValue, public virtual ITemplate_Argument
{
public:
    typedef IBool_Type type_type;
    typedef bool fundamental_type;

    virtual auto get_specialized_type() const -> std::shared_ptr<type_type> = 0;

    virtual auto set_value(fundamental_type value) -> Result<void> = 0;
    virtual auto get_value() const -> fundamental_type = 0;
};

}
