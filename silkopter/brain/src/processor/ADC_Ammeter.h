#pragma once

#include "HAL.h"
#include "common/stream/IADC.h"
#include "common/stream/ICurrent.h"
#include "common/node/IProcessor.h"

#include "Sample_Accumulator.h"
#include "Basic_Output_Stream.h"


namespace silk
{
namespace hal
{
struct ADC_Ammeter_Descriptor;
struct ADC_Ammeter_Config;
}
}

namespace silk
{
namespace node
{

class ADC_Ammeter : public IProcessor
{
public:
    ADC_Ammeter(HAL& hal);

    ts::Result<void> init(hal::INode_Descriptor const& descriptor) override;
    std::shared_ptr<const hal::INode_Descriptor> get_descriptor() const override;

    ts::Result<void> set_config(hal::INode_Config const& config) override;
    std::shared_ptr<const hal::INode_Config> get_config() const override;

    ts::Result<std::shared_ptr<hal::INode_Message>> send_message(hal::INode_Message const& message) override;

    ts::Result<void> start(Clock::time_point tp) override;

    ts::Result<void> set_input_stream_path(size_t idx, std::string const& path);
    auto get_inputs() const -> std::vector<Input>;
    auto get_outputs() const -> std::vector<Output>;

    void process();

private:
    ts::Result<void> init();

    HAL& m_hal;

    std::shared_ptr<hal::ADC_Ammeter_Descriptor> m_descriptor;
    std::shared_ptr<hal::ADC_Ammeter_Config> m_config;

    Sample_Accumulator<stream::IADC> m_accumulator;

    typedef Basic_Output_Stream<stream::ICurrent> Output_Stream;
    mutable std::shared_ptr<Output_Stream> m_output_stream;
};


}
}

