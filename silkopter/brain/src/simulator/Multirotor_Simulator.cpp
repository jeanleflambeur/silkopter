#include "BrainStdAfx.h"
#include "Multirotor_Simulator.h"
#include "uav_properties/IMultirotor_Properties.h"

#if !defined RASPBERRY_PI

#include "uav.def.h"
//#include "sz_Multirotor_Simulator.hpp"
//#include "sz_Multirotor_Simulator_Structs.hpp"

namespace silk
{
namespace node
{

Multirotor_Simulator::Multirotor_Simulator(UAV& uav)
    : m_uav(uav)
    , m_descriptor(new uav::Multirotor_Simulator_Descriptor())
    , m_config(new uav::Multirotor_Simulator_Config())
{
    m_angular_velocity_stream = std::make_shared<Angular_Velocity>();
    m_acceleration_stream = std::make_shared<Acceleration>();
    m_magnetic_field_stream = std::make_shared<Magnetic_Field>();
    m_pressure_stream = std::make_shared<Pressure>();
    m_temperature_stream = std::make_shared<Temperature>();
    m_distance_stream = std::make_shared<Distance>();
    m_gps_info_stream = std::make_shared<GPS_Info>();
    m_ecef_position_stream = std::make_shared<ECEF_Position>();
    m_ecef_velocity_stream = std::make_shared<ECEF_Velocity>();
}

auto Multirotor_Simulator::init(uav::INode_Descriptor const& descriptor) -> bool
{
    QLOG_TOPIC("multirotor_simulator::init");

    auto specialized = dynamic_cast<uav::Multirotor_Simulator_Descriptor const*>(&descriptor);
    if (!specialized)
    {
        QLOGE("Wrong descriptor type");
        return false;
    }
    *m_descriptor = *specialized;

    return init();
}
auto Multirotor_Simulator::init() -> bool
{
    std::shared_ptr<const IMultirotor_Properties> multirotor_properties = m_uav.get_specialized_uav_properties<IMultirotor_Properties>();
    if (!multirotor_properties)
    {
        QLOGE("No multi properties found");
        return false;
    }

    if (!m_simulation.init(1000))
    {
        return false;
    }

    if (!m_simulation.init_uav(multirotor_properties))
    {
        return false;
    }

    m_input_throttle_streams.resize(multirotor_properties->get_motors().size());
    m_input_throttle_stream_paths.resize(multirotor_properties->get_motors().size());

    m_angular_velocity_stream->rate = m_descriptor->get_angular_velocity_rate();
    m_angular_velocity_stream->dt = std::chrono::microseconds(1000000 / m_angular_velocity_stream->rate);

    m_acceleration_stream->rate = m_descriptor->get_acceleration_rate();
    m_acceleration_stream->dt = std::chrono::microseconds(1000000 / m_acceleration_stream->rate);

    m_magnetic_field_stream->rate = m_descriptor->get_magnetic_field_rate();
    m_magnetic_field_stream->dt = std::chrono::microseconds(1000000 / m_magnetic_field_stream->rate);

    m_pressure_stream->rate = m_descriptor->get_pressure_rate();
    m_pressure_stream->dt = std::chrono::microseconds(1000000 / m_pressure_stream->rate);

    m_temperature_stream->rate = m_descriptor->get_temperature_rate();
    m_temperature_stream->dt = std::chrono::microseconds(1000000 / m_temperature_stream->rate);

    m_distance_stream->rate = m_descriptor->get_distance_rate();
    m_distance_stream->dt = std::chrono::microseconds(1000000 / m_distance_stream->rate);

    m_gps_info_stream->rate = m_descriptor->get_gps_rate();
    m_gps_info_stream->dt = std::chrono::microseconds(1000000 / m_gps_info_stream->rate);

    m_ecef_position_stream->rate = m_descriptor->get_gps_rate();
    m_ecef_position_stream->dt = std::chrono::microseconds(1000000 / m_ecef_position_stream->rate);

    m_ecef_velocity_stream->rate = m_descriptor->get_gps_rate();
    m_ecef_velocity_stream->dt = std::chrono::microseconds(1000000 / m_ecef_velocity_stream->rate);


    return true;
}

auto Multirotor_Simulator::start(q::Clock::time_point tp) -> bool
{
    m_last_tp = tp;
    return true;
}

auto Multirotor_Simulator::get_inputs() const -> std::vector<Input>
{
    std::vector<Input> inputs(m_input_throttle_streams.size());
    for (size_t i = 0; i < m_input_throttle_streams.size(); i++)
    {
        inputs[i].type = stream::IThrottle::TYPE;
        inputs[i].rate = m_descriptor->get_throttle_rate();
        inputs[i].name = q::util::format<std::string>("Throttle/[{}]", i);
        inputs[i].stream_path = m_input_throttle_stream_paths[i];
    }
    return inputs;
}
auto Multirotor_Simulator::get_outputs() const -> std::vector<Output>
{
    std::vector<Output> outputs =
    {
        {"Angular Velocity", m_angular_velocity_stream},
        {"Acceleration", m_acceleration_stream},
        {"Magnetic Field", m_magnetic_field_stream},
        {"Pressure", m_pressure_stream},
        {"Temperature", m_temperature_stream},
        {"Sonar Distance", m_distance_stream},
        {"GPS Info", m_gps_info_stream},
        {"GPS Position (ecef)", m_ecef_position_stream},
        {"GPS Velocity (ecef)", m_ecef_velocity_stream},
    };
    return outputs;
}

void Multirotor_Simulator::process()
{
    QLOG_TOPIC("multirotor_simulator::process");

    m_angular_velocity_stream->samples.clear();
    m_acceleration_stream->samples.clear();
    m_magnetic_field_stream->samples.clear();
    m_pressure_stream->samples.clear();
    m_temperature_stream->samples.clear();
    m_distance_stream->samples.clear();
    m_gps_info_stream->samples.clear();
    m_ecef_position_stream->samples.clear();
    m_ecef_velocity_stream->samples.clear();

    for (size_t i = 0; i < m_input_throttle_streams.size(); i++)
    {
        auto throttle = m_input_throttle_streams[i].lock();
        if (throttle)
        {
            auto const& samples = throttle->get_samples();
            if (!samples.empty())
            {
                m_simulation.set_motor_throttle(i, samples.back().value);
            }
        }
    }

    auto now = q::Clock::now();
    auto dt = now - m_last_tp;
    if (dt < std::chrono::milliseconds(1))
    {
        return;
    }
    m_last_tp = now;

    static const util::coordinates::LLA origin_lla(math::radians(41.390205), math::radians(2.154007), 0.0);
    auto enu_to_ecef_trans = util::coordinates::enu_to_ecef_transform(origin_lla);
    auto enu_to_ecef_rotation = util::coordinates::enu_to_ecef_rotation(origin_lla);

    m_simulation.process(dt, [this, &enu_to_ecef_trans, &enu_to_ecef_rotation](Multirotor_Simulation& simulation, q::Clock::duration simulation_dt)
    {
        auto const& uav_state = simulation.get_uav_state();
        {
            Angular_Velocity& stream = *m_angular_velocity_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                math::vec3f noise(m_noise.angular_velocity(m_noise.generator), m_noise.angular_velocity(m_noise.generator), m_noise.angular_velocity(m_noise.generator));
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value = uav_state.angular_velocity + noise;
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
        {
            Acceleration& stream = *m_acceleration_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                math::vec3f noise(m_noise.acceleration(m_noise.generator), m_noise.acceleration(m_noise.generator), m_noise.acceleration(m_noise.generator));
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value = uav_state.acceleration + noise;
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
        {
            Magnetic_Field& stream = *m_magnetic_field_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                math::vec3f noise(m_noise.magnetic_field(m_noise.generator), m_noise.magnetic_field(m_noise.generator), m_noise.magnetic_field(m_noise.generator));
                stream.accumulated_dt -= stream.dt;
                QASSERT(math::is_finite(uav_state.magnetic_field));
                QASSERT(!math::is_zero(uav_state.magnetic_field, math::epsilon<float>()));
                stream.last_sample.value = uav_state.magnetic_field + noise;
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
        {
            Pressure& stream = *m_pressure_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                double noise = m_noise.pressure(m_noise.generator);
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value = uav_state.pressure + noise;
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
        {
            Temperature& stream = *m_temperature_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                float noise = m_noise.temperature(m_noise.generator);
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value = uav_state.temperature + noise;
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
        {
            Distance& stream = *m_distance_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                float noise = m_noise.ground_distance(m_noise.generator);
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value = uav_state.proximity_distance + noise;
                stream.last_sample.is_healthy = !math::is_zero(uav_state.proximity_distance, std::numeric_limits<float>::epsilon());
                stream.samples.push_back(stream.last_sample);
            }
        }

        {
            GPS_Info& stream = *m_gps_info_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value.fix = stream::IGPS_Info::Value::Fix::FIX_3D;
                stream.last_sample.value.visible_satellites = 4;
                stream.last_sample.value.fix_satellites = 4;
                stream.last_sample.value.pacc = m_noise.gps_pacc(m_noise.generator);
                stream.last_sample.value.vacc = m_noise.gps_vacc(m_noise.generator);
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
        {
            ECEF_Position& stream = *m_ecef_position_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                math::vec3d noise(m_noise.gps_position(m_noise.generator), m_noise.gps_position(m_noise.generator), m_noise.gps_position(m_noise.generator));
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value = math::transform(enu_to_ecef_trans, math::vec3d(uav_state.enu_position)) + noise;
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
        {
            ECEF_Velocity& stream = *m_ecef_velocity_stream;
            stream.accumulated_dt += simulation_dt;
            while (stream.accumulated_dt >= stream.dt)
            {
                math::vec3f noise(m_noise.gps_velocity(m_noise.generator), m_noise.gps_velocity(m_noise.generator), m_noise.gps_velocity(m_noise.generator));
                stream.accumulated_dt -= stream.dt;
                stream.last_sample.value = math::vec3f(math::transform(enu_to_ecef_rotation, math::vec3d(uav_state.enu_velocity))) + noise;
                stream.last_sample.is_healthy = true;
                stream.samples.push_back(stream.last_sample);
            }
        }
    });
}

void Multirotor_Simulator::set_input_stream_path(size_t idx, q::Path const& path)
{
    auto input_stream = m_uav.get_stream_registry().find_by_name<stream::IThrottle>(path.get_as<std::string>());
    auto rate = input_stream ? input_stream->get_rate() : 0u;
    if (rate != m_descriptor->get_throttle_rate())
    {
        QLOGW("Bad input stream '{}'. Expected rate {}Hz, got {}Hz", path, m_descriptor->get_throttle_rate(), rate);
        m_input_throttle_streams[idx].reset();
        m_input_throttle_stream_paths[idx].clear();
    }
    else
    {
        m_input_throttle_streams[idx] = input_stream;
        m_input_throttle_stream_paths[idx] = path;
    }
}

auto Multirotor_Simulator::set_config(uav::INode_Config const& config) -> bool
{
    QLOG_TOPIC("multirotor_simulator::set_config");

    auto specialized = dynamic_cast<uav::Multirotor_Simulator_Config const*>(&config);
    if (!specialized)
    {
        QLOGE("Wrong config type");
        return false;
    }

//    Simulation::UAV_Config uav_config;
//    uav_config.mass = sz.uav.mass;
//    uav_config.radius = sz.uav.radius;
//    uav_config.height = sz.uav.height;
//    uav_config.motors.resize(sz.motors.size());
//    for (size_t i = 0; i < uav_config.motors.size(); i++)
//    {
//        uav_config.motors[i].position = sz.motors[i].position;
//        uav_config.motors[i].clockwise = sz.motors[i].clockwise;
//        uav_config.motors[i].max_thrust = sz.motors[i].max_thrust;
//        uav_config.motors[i].max_rpm = sz.motors[i].max_rpm;
//        uav_config.motors[i].acceleration = sz.motors[i].acceleration;
//        uav_config.motors[i].deceleration = sz.motors[i].deceleration;
//    }

    std::shared_ptr<const IMultirotor_Properties> multirotor_properties = m_uav.get_specialized_uav_properties<IMultirotor_Properties>();
    if (!multirotor_properties)
    {
        QLOGE("No multi properties found");
        return false;
    }

    if (!m_simulation.init_uav(multirotor_properties))
    {
        return false;
    }
    *m_config = *specialized;

    m_simulation.set_gravity_enabled(m_config->get_gravity_enabled());
    m_simulation.set_ground_enabled(m_config->get_ground_enabled());
    m_simulation.set_drag_enabled(m_config->get_drag_enabled());
    m_simulation.set_simulation_enabled(m_config->get_simulation_enabled());

    auto const& noise = m_config->get_noise();

    m_noise.gps_position = Noise::Distribution<float>(-noise.get_gps_position()*0.5f, noise.get_gps_position()*0.5f);
    m_noise.gps_velocity = Noise::Distribution<float>(-noise.get_gps_velocity()*0.5f, noise.get_gps_velocity()*0.5f);
    m_noise.gps_pacc = Noise::Distribution<float>(-noise.get_gps_pacc()*0.5f, noise.get_gps_pacc()*0.5f);
    m_noise.gps_vacc = Noise::Distribution<float>(-noise.get_gps_vacc()*0.5f, noise.get_gps_vacc()*0.5f);
    m_noise.acceleration = Noise::Distribution<float>(-noise.get_acceleration()*0.5f, noise.get_acceleration()*0.5f);
    m_noise.angular_velocity = Noise::Distribution<float>(-noise.get_angular_velocity()*0.5f, noise.get_angular_velocity()*0.5f);
    m_noise.magnetic_field = Noise::Distribution<float>(-noise.get_magnetic_field()*0.5f, noise.get_magnetic_field()*0.5f);
    m_noise.pressure = Noise::Distribution<float>(-noise.get_pressure()*0.5f, noise.get_pressure()*0.5f);
    m_noise.temperature = Noise::Distribution<float>(-noise.get_temperature()*0.5f, noise.get_temperature()*0.5f);
    m_noise.ground_distance = Noise::Distribution<float>(-noise.get_ground_distance()*0.5f, noise.get_ground_distance()*0.5f);

    return true;
}
auto Multirotor_Simulator::get_config() const -> std::shared_ptr<const uav::INode_Config>
{
    return m_config;
}

auto Multirotor_Simulator::get_descriptor() const -> std::shared_ptr<const uav::INode_Descriptor>
{
    return m_descriptor;
}
//auto Multirotor_Simulator::send_message(rapidjson::Value const& json) -> rapidjson::Document
//{
//    rapidjson::Document response;

//    //todo - fix this
////    auto* messagej = jsonutil::find_value(json, std::string("message"));
////    if (!messagej)
////    {
////        jsonutil::add_value(response, std::string("error"), rapidjson::Value("Message not found"), response.GetAllocator());
////    }
////    else if (!messagej->IsString())
////    {
////        jsonutil::add_value(response, std::string("error"), rapidjson::Value("Message has to be a string"), response.GetAllocator());
////    }
////    else
////    {
////        std::string message = messagej->GetString();
////        if (message == "reset")
////        {
////            m_simulation.reset();
////        }
////        else if (message == "stop motion")
////        {
////            m_simulation.stop_motion();
////        }
////        else if (message == "get state")
////        {
////            auto const& state = m_simulation.get_uav_state();
////            autojsoncxx::to_document(state, response);
////        }
////        else
////        {
////            jsonutil::add_value(response, std::string("error"), rapidjson::Value("Unknown message"), response.GetAllocator());
////        }
////    }
//    return std::move(response);
//}


}
}

#endif
