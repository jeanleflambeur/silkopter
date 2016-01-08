#pragma once

#include "HAL.h"
#include "common/node/ISource.h"
#include "common/stream/IPressure.h"
#include "common/stream/ITemperature.h"
#include "common/bus/II2C.h"
#include "common/bus/ISPI.h"

#include "Basic_Output_Stream.h"

namespace sz
{
namespace MS5611
{
struct Init_Params;
struct Config;
}
}


namespace silk
{
namespace node
{

class MS5611 : public ISource
{
public:
    MS5611(HAL& hal);

    auto init(rapidjson::Value const& init_params) -> bool;
    auto get_init_params() const -> rapidjson::Document;

    auto set_config(rapidjson::Value const& json) -> bool;
    auto get_config() const -> rapidjson::Document;

    auto send_message(rapidjson::Value const& json) -> rapidjson::Document;

    auto start(q::Clock::time_point tp) -> bool override;

    auto get_outputs() const -> std::vector<Output>;

    void process();

private:
    auto init() -> bool;

    HAL& m_hal;
    bus::II2C_wptr m_i2c;
    bus::ISPI_wptr m_spi;

    struct Buses
    {
        bus::II2C_ptr i2c;
        bus::ISPI_ptr spi;
    };
    //i use 2 vectors to avoid memsetting the tx_data to zero all the time
    std::vector<uint8_t> m_dummy_tx_data;
    std::vector<uint8_t> m_dummy_rx_data;

    auto lock(Buses& buses) -> bool;
    void unlock(Buses& buses);
    auto bus_read(Buses& buses, uint8_t reg, uint8_t* data, uint32_t size) -> bool;
    auto bus_read_u8(Buses& buses, uint8_t reg, uint8_t& dst) -> bool;
    auto bus_read_u16(Buses& buses, uint8_t reg, uint16_t& dst) -> bool;
    auto bus_write(Buses& buses, uint8_t reg, uint8_t const* data, uint32_t size) -> bool;
    auto bus_write_u8(Buses& buses, uint8_t reg, uint8_t const& t) -> bool;
    auto bus_write_u16(Buses& buses, uint8_t reg, uint16_t const& t) -> bool;

    std::shared_ptr<sz::MS5611::Init_Params> m_init_params;
    std::shared_ptr<sz::MS5611::Config> m_config;

    template<class Base>
    struct Common : public Base
    {
        double      reading = 0;
    };

    typedef Basic_Output_Stream<Common<stream::IPressure>> Pressure_Stream;
    mutable std::shared_ptr<Pressure_Stream> m_pressure;

    typedef Basic_Output_Stream<Common<stream::ITemperature>> Temperature_Stream;
    mutable std::shared_ptr<Temperature_Stream> m_temperature;

    double		m_c1 = 0;
    double		m_c2 = 0;
    double		m_c3 = 0;
    double		m_c4 = 0;
    double		m_c5 = 0;
    double		m_c6 = 0;

    uint8_t         m_stage = 0;
    q::Clock::time_point m_last_reading_tp = q::Clock::now();
};


}
}
