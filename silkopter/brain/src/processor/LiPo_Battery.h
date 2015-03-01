#pragma once

#include "common/node/processor/IProcessor.h"
#include "common/node/stream/IVoltage.h"
#include "common/node/stream/ICurrent.h"
#include "common/node/stream/IBattery_State.h"

#include "HAL.h"
#include "utils/Butterworth.h"

namespace sz
{
namespace LiPo_Battery
{
struct Init_Params;
struct Config;
}
}


namespace silk
{
namespace node
{
namespace processor
{

class LiPo_Battery : public IProcessor
{
public:
    LiPo_Battery(HAL& hal);

    auto init(rapidjson::Value const& init_params, rapidjson::Value const& config) -> bool;
    auto get_init_params() -> rapidjson::Document;

    auto set_config(rapidjson::Value const& json) -> bool;
    auto get_config() -> rapidjson::Document;

    auto get_input_stream_count() const -> size_t;
    auto get_input_stream(size_t idx) -> stream::IStream&;

    auto get_output_stream_count() const -> size_t;
    auto get_output_stream(size_t idx) -> stream::IBattery_State&;

    auto get_cell_count() const -> boost::optional<uint8_t>;

    auto get_name() const -> std::string const&;

    void process();

private:
    auto init() -> bool;

    HAL& m_hal;

    std::shared_ptr<sz::LiPo_Battery::Init_Params> m_init_params;
    std::shared_ptr<sz::LiPo_Battery::Config> m_config;

    stream::IVoltage* m_voltage_stream = nullptr;
    stream::ICurrent* m_current_stream = nullptr;

    std::vector<stream::IVoltage::Sample> m_voltage_samples;
    std::vector<stream::ICurrent::Sample> m_current_samples;
    q::Clock::duration m_dt = q::Clock::duration(0);

    auto compute_cell_count() -> boost::optional<uint8_t>;
    boost::optional<uint8_t> m_cell_count;

    util::Butterworth<1, stream::IVoltage::FILTER_CHANNELS> m_voltage_filter;
    util::Butterworth<1, stream::ICurrent::FILTER_CHANNELS> m_current_filter;
//    typename Stream_t::FILTER_CHANNEL_TYPE* m_channels[Stream_t::FILTER_CHANNELS] = { nullptr };

    struct Stream : public stream::IBattery_State
    {
        auto get_samples() const -> std::vector<Sample> const& { return samples; }
        auto get_rate() const -> uint32_t { return rate; }
        auto get_name() const -> std::string const& { return name; }

        uint32_t rate = 0;
        Sample last_sample;
        std::vector<Sample> samples;
        std::string name;
    } m_stream;
};


}
}
}
