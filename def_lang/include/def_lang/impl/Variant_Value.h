#pragma once

#include "def_lang/IVariant_Type.h"
#include "def_lang/IVariant_Value.h"

namespace ts
{

class Variant_Value final : virtual public IVariant_Value
{
public:
    Variant_Value(std::shared_ptr<IVariant_Type const> type);
    ~Variant_Value();

    bool is_constructed() const override;

    Result<bool> is_equal(IValue const& other) const override;

    Result<void> construct(IInitializer_List const& initializer_list = Initializer_List({})) override;
    Result<void> copy_construct(IValue const& other) override;

    Result<void> copy_assign(IValue const& other) override;
    Result<void> copy_assign(IInitializer_List const& initializer_list) override;

    std::shared_ptr<IType const> get_type() const override;

    Result<void> parse_from_ui_string(std::string const& str) override;
    Result<std::string> get_ui_string() const override;

    std::shared_ptr<const IValue> select(Value_Selector&& selector) const override;
    std::shared_ptr<IValue> select(Value_Selector&& selector) override;

    Result<sz::Value> serialize() const override;
    Result<void> deserialize(sz::Value const&) override;

    std::shared_ptr<IVariant_Type const> get_specialized_type() const override;

    bool is_set() const override;

    Result<void> set_value_type(std::shared_ptr<const IType> type) override;
    Result<void> set_value_type_index(size_t idx) override;
    size_t get_value_type_index() const override;

    Result<void> set_value(std::shared_ptr<const IValue> value) override;
    std::shared_ptr<const IValue> get_value() const override;
    std::shared_ptr<IValue> get_value() override;

private:
    bool m_is_constructed = false;
    std::shared_ptr<IVariant_Type const> m_type;
    std::vector<std::shared_ptr<IValue>> m_values;
    size_t m_crt_type_index = 0;

    std::vector<boost::signals2::connection> m_value_changed_connections;
};

}
