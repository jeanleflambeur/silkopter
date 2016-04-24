#include "def_lang/impl/Vector_Type.h"
#include "def_lang/impl/Vector_Value.h"
#include "def_lang/IAttribute.h"

namespace ts
{

Vector_Type::Vector_Type(std::string const& name)
    : Symbol_EP(name)
{

}

Result<void> Vector_Type::init(std::vector<std::shared_ptr<const ITemplate_Argument>> const& arguments)
{
    if (arguments.size() != 1)
    {
        return Error("Expected only one template argument, got " + std::to_string(arguments.size()));
    }

    m_inner_type = std::dynamic_pointer_cast<const IType>(arguments[0]);
    if (!m_inner_type)
    {
        return Error("Invalid template argument. Expected type");
    }

    return success;
}

Result<void> Vector_Type::validate_attribute(IAttribute const& attribute)
{
    return Error("Attribute " + attribute.get_name() + " not supported");
}

std::shared_ptr<IType> Vector_Type::clone(std::string const& name) const
{
    return std::shared_ptr<IType>(new Vector_Type(*this));
}

std::string Vector_Type::get_template_instantiation_string() const
{
    return get_name();
}

std::shared_ptr<const IType> Vector_Type::get_inner_type() const
{
    return m_inner_type;
}

std::shared_ptr<IValue> Vector_Type::create_value() const
{
    return create_specialized_value();
}

std::shared_ptr<Vector_Type::value_type> Vector_Type::create_specialized_value() const
{
    return std::make_shared<Vector_Value>(shared_from_this());
}


}