#pragma once

#include "common/node/INode.h"

namespace silk
{
namespace node
{

class ISource : public Node_Base<Type::SOURCE>
{
public:
    //sources don't have any inputs
    virtual void set_stream_input_path(size_t idx, q::Path const& path) {}
    virtual auto get_stream_inputs() const -> std::vector<Stream_Input> final { return std::vector<Stream_Input>(); }
};
DECLARE_CLASS_PTR(ISource);


}
}

