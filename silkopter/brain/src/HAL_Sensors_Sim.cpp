#include "BrainStdAfx.h"
#include "HAL_Sensors_Sim.h"
#include "utils/Json_Helpers.h"

#ifndef RASPBERRY_PI

#include "sz_math.hpp"
#include "sz_hal_sensors_config_sim.hpp"


using namespace silk;
using namespace boost::asio;

HAL_Sensors_Sim::HAL_Sensors_Sim(Sim_Comms& sim_comms)
    : m_sim_comms(sim_comms)
{
    sim_comms.message_received_signal.connect(std::bind(&HAL_Sensors_Sim::handle_message, this, std::placeholders::_1, std::placeholders::_2));

    load_settings();
    save_settings();
}

auto HAL_Sensors_Sim::load_settings() -> bool
{
    autojsoncxx::ParsingResult result;
    HAL_Sensors_Sim_Config cfg;
    if (!autojsoncxx::from_json_file("sensors_sim.cfg", cfg, result))
    {
        SILK_WARNING("Failed to load sensors_sim.cfg: {}", result.description());
        return false;
    }

    m_calibration_config.accelerometer_bias = cfg.accelerometer_bias;
    m_calibration_config.accelerometer_scale = cfg.accelerometer_scale;
    m_calibration_config.gyroscope_bias = cfg.gyroscope_bias;
    m_calibration_config.compass_bias = cfg.compass_bias;

    return true;
}
void HAL_Sensors_Sim::save_settings()
{
    HAL_Sensors_Sim_Config cfg;

    cfg.accelerometer_bias = m_calibration_config.accelerometer_bias;
    cfg.accelerometer_scale = m_calibration_config.accelerometer_scale;
    cfg.gyroscope_bias = m_calibration_config.gyroscope_bias;
    cfg.compass_bias = m_calibration_config.compass_bias;

    autojsoncxx::to_pretty_json_file("sensors_sim.cfg", cfg);
}

void HAL_Sensors_Sim::set_accelerometer_calibration_data(math::vec3f const& bias, math::vec3f const& scale)
{
    m_calibration_config.accelerometer_bias = bias;
    m_calibration_config.accelerometer_scale = scale;
}
void HAL_Sensors_Sim::get_accelerometer_calibration_data(math::vec3f &bias, math::vec3f &scale) const
{
    bias = m_calibration_config.accelerometer_bias;
    scale = m_calibration_config.accelerometer_scale;
}

void HAL_Sensors_Sim::set_gyroscope_calibration_data(math::vec3f const& bias)
{
    m_calibration_config.gyroscope_bias = bias;
}
void HAL_Sensors_Sim::get_gyroscope_calibration_data(math::vec3f &bias) const
{
    bias = m_calibration_config.gyroscope_bias;
}

void HAL_Sensors_Sim::set_compass_calibration_data(math::vec3f const& bias)
{
    m_calibration_config.compass_bias = bias;
}
void HAL_Sensors_Sim::get_compass_calibration_data(math::vec3f &bias) const
{
    bias = m_calibration_config.compass_bias;
}

auto HAL_Sensors_Sim::get_accelerometer_samples() const -> std::vector<Accelerometer_Sample> const&
{
    return m_accelerometer_samples;
}
auto HAL_Sensors_Sim::get_gyroscope_samples() const -> std::vector<Gyroscope_Sample> const&
{
    return m_gyroscope_samples;
}
auto HAL_Sensors_Sim::get_compass_samples() const -> std::vector<Compass_Sample> const&
{
    return m_compass_samples;
}
auto HAL_Sensors_Sim::get_barometer_samples() const -> std::vector<Barometer_Sample> const&
{
    return m_barometer_samples;
}
auto HAL_Sensors_Sim::get_sonar_samples() const -> std::vector<Sonar_Sample> const&
{
    return m_sonar_samples;
}
auto HAL_Sensors_Sim::get_thermometer_samples() const -> std::vector<Thermometer_Sample> const&
{
    return m_thermometer_samples;
}
auto HAL_Sensors_Sim::get_voltage_samples() const -> std::vector<Voltage_Sample> const&
{
    return m_voltage_samples;
}
auto HAL_Sensors_Sim::get_current_samples() const -> std::vector<Current_Sample> const&
{
    return m_current_samples;
}
auto HAL_Sensors_Sim::get_gps_samples() const -> std::vector<GPS_Sample> const&
{
    return m_gps_samples;
}

size_t HAL_Sensors_Sim::get_error_count() const
{
    return m_error_count + m_sim_comms.get_error_count();
}

template<class SAMPLE_T>
auto HAL_Sensors_Sim::unpack_sensor_sample(Sim_Comms::Channel& channel, SAMPLE_T& sample, std::vector<SAMPLE_T>& samples) -> bool
{
    uint8_t dt = 0;
    decltype(sample.value) v;
    if (!channel.unpack_param(dt) || !channel.unpack_param(v))
    {
        SILK_WARNING("Failed to receive sensor sample");
        m_error_count++;
        return false;
    }

    sample.value = v;
    sample.dt = std::chrono::milliseconds(dt);
    sample.sample_idx++;
    samples.push_back(sample);
    return true;
}

auto HAL_Sensors_Sim::process_accelerometer_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_accelerometer_sample, m_accelerometer_samples);
}
auto HAL_Sensors_Sim::process_gyroscope_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_gyroscope_sample, m_gyroscope_samples);
}
auto HAL_Sensors_Sim::process_compass_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_compass_sample, m_compass_samples);
}

auto HAL_Sensors_Sim::process_barometer_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_barometer_sample, m_barometer_samples);
}
auto HAL_Sensors_Sim::process_thermometer_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_thermometer_sample, m_thermometer_samples);
}
auto HAL_Sensors_Sim::process_sonar_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_sonar_sample, m_sonar_samples);
}
auto HAL_Sensors_Sim::process_voltage_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_voltage_sample, m_voltage_samples);
}
auto HAL_Sensors_Sim::process_current_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_current_sample, m_current_samples);
}
auto HAL_Sensors_Sim::process_gps_sensor(Sim_Comms::Channel& channel) -> bool
{
    return unpack_sensor_sample(channel, m_gps_sample, m_gps_samples);
}

void HAL_Sensors_Sim::process_message_sensor_data(Sim_Comms::Channel& channel)
{
    bool result = false;
    if (channel.begin_unpack())
    {
        Sim_Comms::Sensors sensors;
        if (channel.unpack_param(sensors))
        {
            if (sensors.test(Sim_Comms::Sensor::ACCELEROMETER))
            {
                result = process_accelerometer_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::GYROSCOPE))
            {
                result = process_gyroscope_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::COMPASS))
            {
                result = process_compass_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::BAROMETER))
            {
                result = process_barometer_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::THERMOMETER))
            {
                result = process_thermometer_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::SONAR))
            {
                result = process_sonar_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::VOLTAGE))
            {
                result = process_voltage_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::CURRENT))
            {
                result = process_current_sensor(channel);
            }
            if (sensors.test(Sim_Comms::Sensor::GPS))
            {
                result = process_gps_sensor(channel);
            }
        }
    }
    channel.end_unpack();

    if (!result)
    {
        SILK_WARNING("Failed to receive sensor data");
        m_error_count++;
    }
}
//----------------------------------------------------------------------------------

void HAL_Sensors_Sim::process()
{
    if (!m_sim_comms.is_connected())
    {
        m_sim_comms.connect();
    }
    else
    {
        m_sim_comms.process();
    }
}


void HAL_Sensors_Sim::handle_message(Sim_Comms::Message message, Sim_Comms::Channel& channel)
{
    switch (message)
    {
    case Sim_Comms::Message::SENSOR_DATA: process_message_sensor_data(channel); break;
    }
}

#endif