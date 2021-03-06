#pragma once

#include "HAL.h"
#include "common/node/ISource.h"
#include "common/stream/IADC.h"
#include "common/bus/II2C.h"

#include "Basic_Output_Stream.h"


namespace silk
{
namespace hal
{
struct ADS1115_Descriptor;
struct ADS1115_Config;
}
}



namespace silk
{
namespace node
{

class ADS1115_Source : public ISource
{
public:
    ADS1115_Source(HAL& hal);

    ts::Result<void> init(hal::INode_Descriptor const& descriptor) override;
    std::shared_ptr<const hal::INode_Descriptor> get_descriptor() const override;

    ts::Result<void> set_config(hal::INode_Config const& config) override;
    std::shared_ptr<const hal::INode_Config> get_config() const override;

    ts::Result<std::shared_ptr<messages::INode_Message>> send_message(messages::INode_Message const& message) override;

    ts::Result<void> start(Clock::time_point tp) override;

    auto get_outputs() const -> std::vector<Output>;

    void process();

private:
    ts::Result<void> init();

    bool read_sensor(util::hw::II2C& i2c, float& o_value);

    HAL& m_hal;
    std::weak_ptr<bus::II2C_Bus> m_i2c_bus;

    std::shared_ptr<hal::ADS1115_Descriptor> m_descriptor;
    std::shared_ptr<hal::ADS1115_Config> m_config;

    Clock::time_point m_schedule_tp = Clock::now();

    typedef Basic_Output_Stream<stream::IADC> Output_Stream;

    struct ADC
    {
        std::shared_ptr<Output_Stream> stream;
        size_t pin_index = 0;
        Clock::time_point last_tp = Clock::now();
    };

    mutable std::vector<ADC> m_adcs;

    auto set_config_register(util::hw::II2C& i2c) -> bool;

    struct Config_Register
    {
        uint16_t gain = 0;
        uint16_t mux = 0;
        uint16_t status = 0;
        uint16_t mode = 0;
        uint16_t rate = 0;
        uint16_t comparator = 0;
        uint16_t polarity = 0;
        uint16_t latch = 0;
        uint16_t queue = 0;
    } m_config_register;

    uint8_t m_crt_adc_idx = 0;
};


}
}
