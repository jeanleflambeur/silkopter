#include "BrainStdAfx.h"
#include "PIGPIO.h"

#include "sz_math.hpp"
#include "sz_PIGPIO.hpp"

#ifdef RASPBERRY_PI
extern "C"
{
    #include "pigpio.h"
}
#endif

namespace silk
{
namespace node
{
namespace sink
{

const size_t PIGPIO::MAX_PWM_CHANNELS;


PIGPIO::PIGPIO(HAL& hal)
    : m_hal(hal)
    , m_init_params(new sz::PIGPIO::Init_Params())
    , m_config(new sz::PIGPIO::Config())
{
}

auto PIGPIO::get_name() const -> std::string const&
{
    return m_init_params->name;
}
auto PIGPIO::get_input_stream_count() const -> size_t
{
    return m_pwm_channels.size();
}
auto PIGPIO::get_input_stream(size_t idx) -> stream::IPWM_Value&
{
    return *m_pwm_channels[idx].stream;
}

auto PIGPIO::init(rapidjson::Value const& json) -> bool
{
    QLOG_TOPIC("pigpio::init");

    sz::PIGPIO::Init_Params sz;
    autojsoncxx::error::ErrorStack result;
    if (!autojsoncxx::from_value(sz, json, result))
    {
        std::ostringstream ss;
        ss << result;
        QLOGE("Cannot deserialize PIGPIO data: {}", ss.str());
        return false;
    }
    *m_init_params = sz;
    autojsoncxx::to_document(sz, m_init_params_json);

    return init();
}

auto PIGPIO::init() -> bool
{
    QLOG_TOPIC("pigpio::init");

#if defined (RASPBERRY_PI)
    size_t period = m_params.period.count();

    std::vector<size_t> periods = {1, 2, 4, 5, 8, 10};
    if (std::find(periods.begin(), periods.end(), period) == periods.end())
    {
        QLOGE("{}: Invalid period {}. Supported are: {}", get_name(), period, periods);
        return false;
    }

    std::vector<size_t> rates = { 40000, 20000, 10000, 8000, 5000, 4000, 2500, 2000, 1600, 1250, 1000, 800, 500, 400, 250, 200, 100, 50 };
    for (auto& f: rates)
    {
        auto x = math::round(double(f) / double(period));
        f = static_cast<size_t>(x);
    }

    std::vector<size_t> gpios = { 4, 17, 18, 21, 27, 22, 23, 24, 25 };

    //first validate
    for (size_t i = 0; i < m_params.pwm_channels.size(); i++)
    {
        auto& ch = m_params.pwm_channels[i];
        if (std::find(gpios.begin(), gpios.end(), ch.gpio) == gpios.end())
        {
            QLOGE("{}: channel {}: cannot use GPIO {}. Valid GPIOS are {}", get_name(), i, ch.gpio, gpios);
            return false;
        }
        if (ch.min >= ch.max)
        {
            QLOGE("{}: channel {} on GPIO {}: min ({}) is bigger than max ({})", get_name(), i, ch.gpio, ch.min, ch.max);
            return false;
        }
        if (ch.max > ch.range)
        {
            QLOGE("{}: channel {} on GPIO {}: max ({}) is bigger than the range ({})", get_name(), i, ch.gpio, ch.max, ch.range);
            return false;
        }
        if (std::find(rates.begin(), rates.end(), ch.rate) == rates.end())
        {
            QLOGE("{}: channel {} on GPIO {}: invalid rate {}. Supported are: {}", get_name(), i, ch.gpio, ch.rate, rates);
            return false;
        }
        if (!ch.stream)
        {
            QLOGE("{}: channel {} on GPIO {}: no valid stream", get_name(), i, ch.gpio);
            return false;
        }
        if (ch.stream->get_rate() > ch.rate)
        {
            QLOGE("{}: stream {} is too fast for the channel {} on GPIO {}: {}Hz > {}Hz",
                  get_name(), ch.stream->get_name(), i, ch.gpio, ch.stream->get_rate(), ch.rate);
            return false;
        }
    }

    QLOGI("{}: Initializing pigpio", get_name());
    if (gpioCfgClock(period, 1, 0) < 0 ||
        gpioCfgPermissions(static_cast<uint64_t>(-1)) ||
        gpioCfgInterfaces(PI_DISABLE_SOCK_IF | PI_DISABLE_FIFO_IF))
    {
        QLOGE("{}: Cannot configure pigpio", get_name());
        return false;
    }
    if (gpioInitialise() < 0)
    {
        QLOGE("{}: Cannot initialize pigpio", get_name());
        return false;
    }

    //first of all turn on the pull-down to that if the board is reset of powerred off the motors don't start spinning
    //after a restart the GPIO pins are configured as inputs so their state is floating. Most of the time this results in a high pin
    for (size_t i = 0; i < m_params.pwm_channels.size(); i++)
    {
        auto gpio = m_params.pwm_channels[i].gpio;
        if (gpioSetPullUpDown(gpio, PI_PUD_DOWN) < 0)
        {
            QLOGE("{}: channel {} on GPIO {}: Cannot set pull down mode", get_name(), i, ch.gpio);
            return false;
        }
        if (gpioSetMode(gpio, PI_OUTPUT) < 0)
        {
            QLOGE("{}: channel {} on GPIO {}: Cannot set GPIO mode to output", get_name(), i, ch.gpio);
            return false;
        }
     }

    //now configure the pin for PWM or servo
    for (size_t i = 0; i < m_params.pwm_channels.size(); i++)
    {
        auto& ch = m_params.pwm_channels[i];
        int gpio = ch.gpio;
        auto f = gpioSetPWMfrequency(gpio, ch.rate);
        if (f < 0)
        {
            QLOGE("{}: channel {} on GPIO {}: Cannot set pwm rate {}", get_name(), i, ch.gpio, ch.rate);
            return false;
        }
        if (gpioSetPWMrange(gpio, ch.range) < 0)
        {
            QLOGE("{}: channel {} on GPIO {}: Cannot set pwm range {} on gpio {}", get_name(), i, ch.gpio, ch.range);
            return false;
        }
        QLOGI("{}: channel {} on GPIO {}: rate {} range {}", get_name(), i, ch.gpio, ch.rate, ch.range);
    }

    return true;
#else
    QLOGE("{}: PIGPIO only supported on the raspberry pi", get_name());
    return false;
#endif
}

void PIGPIO::set_pwm_value(size_t idx, float value)
{
    QLOG_TOPIC("pigpio::set_pwm_value");

#if defined RASPBERRY_PI
    auto& ch = m_params.pwm_channels[idx];
    value = math::clamp(value, 0.f, 1.f);
    int pulse = value * (ch.max - ch.min);
    gpioPWM(ch.gpio, ch.min + pulse);
#else
    QUNUSED(idx);
    QUNUSED(value);
    QASSERT(0);
#endif
}

void PIGPIO::process()
{
    QLOG_TOPIC("pigpio::process");

    for (size_t i = 0; i < m_pwm_channels.size(); i++)
    {
        auto& ch = m_pwm_channels[i];
        QASSERT(ch.stream);
        auto const& samples = ch.stream->get_samples();
        if (!samples.empty())
        {
            if (samples.size() > 2)
            {
                QLOGW("{}: channel {} on GPIO {} is too slow. {} samples are queued", get_name(), i, ch.gpio, samples.size());
            }

            set_pwm_value(i, samples.back().value);
        }
    }
}

auto PIGPIO::set_config(rapidjson::Value const& json) -> bool
{
    sz::PIGPIO::Config sz;
    autojsoncxx::error::ErrorStack result;
    if (!autojsoncxx::from_value(sz, json, result))
    {
        std::ostringstream ss;
        ss << result;
        QLOGE("Cannot deserialize PIGPIO config data: {}", ss.str());
        return false;
    }

    *m_config = sz;
    autojsoncxx::to_document(*m_config, m_config_json);
    return true;
}
auto PIGPIO::get_config() -> boost::optional<rapidjson::Value const&>
{
    return m_config_json;
}
auto PIGPIO::get_init_params() -> boost::optional<rapidjson::Value const&>
{
    return m_init_params_json;
}

}
}
}