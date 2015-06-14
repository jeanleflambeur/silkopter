#pragma once

#include "HAL.h"
#include "common/node/IGenerator.h"
#include "generator/Oscillator.h"

#include "Basic_Output_Stream.h"

#include "sz_math.hpp"
#include "sz_Scalar_Generator.hpp"

namespace silk
{
namespace node
{

template<class Stream_t>
class Scalar_Generator : public IGenerator
{
public:
    Scalar_Generator(HAL& hal);

    auto init(rapidjson::Value const& init_params) -> bool;
    auto get_init_params() const -> rapidjson::Document const&;

    auto set_config(rapidjson::Value const& json) -> bool;
    auto get_config() const -> rapidjson::Document;

    auto send_message(rapidjson::Value const& json) -> rapidjson::Document;

    void set_stream_input_path(size_t idx, q::Path const& path);
    auto get_stream_inputs() const -> std::vector<Stream_Input>;
    auto get_stream_outputs() const -> std::vector<Stream_Output>;

    void process();

private:
    auto init() -> bool;

    HAL& m_hal;

    rapidjson::Document m_init_paramsj;
    sz::Scalar_Generator::Init_Params m_init_params;
    sz::Scalar_Generator::Config m_config;

    std::weak_ptr<stream::IFloat> m_modulation_stream;
    q::Path m_modulation_stream_path;

    typedef Basic_Output_Stream<Stream_t> Output_Stream;
    mutable std::shared_ptr<Output_Stream> m_output_stream;
};


template<class Stream_t>
Scalar_Generator<Stream_t>::Scalar_Generator(HAL& hal)
    : m_hal(hal)
{
    autojsoncxx::to_document(m_init_params, m_init_paramsj);
}

template<class Stream_t>
auto Scalar_Generator<Stream_t>::init(rapidjson::Value const& init_params) -> bool
{
    QLOG_TOPIC("scalar_generator::init");

    sz::Scalar_Generator::Init_Params sz;
    autojsoncxx::error::ErrorStack result;
    if (!autojsoncxx::from_value(sz, init_params, result))
    {
        std::ostringstream ss;
        ss << result;
        QLOGE("Cannot deserialize Scalar_Generator data: {}", ss.str());
        return false;
    }
    jsonutil::clone_value(m_init_paramsj, init_params, m_init_paramsj.GetAllocator());
    m_init_params = sz;
    return init();
}

template<class Stream_t>
auto Scalar_Generator<Stream_t>::init() -> bool
{
    m_output_stream = std::make_shared<Output_Stream>();
    if (m_init_params.rate == 0)
    {
        QLOGE("Bad rate: {}Hz", m_init_params.rate);
        return false;
    }
    m_output_stream->set_rate(m_init_params.rate);
    m_output_stream->set_tp(q::Clock::now());
    return true;
}

template<class Stream_t>
auto Scalar_Generator<Stream_t>::get_init_params() const -> rapidjson::Document const&
{
    return m_init_paramsj;
}

template<class Stream_t>
void Scalar_Generator<Stream_t>::set_stream_input_path(size_t idx, q::Path const& path)
{
    QLOG_TOPIC("rate_controller::set_stream_input_path");

    auto modulation_stream = m_hal.get_streams().template find_by_name<stream::IFloat>(path.get_as<std::string>());
    if (modulation_stream && modulation_stream->get_rate() != m_output_stream->get_rate())
    {
        QLOGW("Bad modulation stream '{}'. Expected rate {}Hz, got {}Hz", path, m_output_stream->get_rate(), modulation_stream->get_rate());
        m_modulation_stream.reset();
        m_modulation_stream_path.clear();
    }
    else
    {
        m_modulation_stream = modulation_stream;
        m_modulation_stream_path = path;
    }
}

template<class Stream_t>
auto Scalar_Generator<Stream_t>::set_config(rapidjson::Value const& json) -> bool
{
    QLOG_TOPIC("scalar_generator::set_config");
    sz::Scalar_Generator::Config sz;
    autojsoncxx::error::ErrorStack result;
    if (!autojsoncxx::from_value(sz, json, result))
    {
        std::ostringstream ss;
        ss << result;
        QLOGE("Cannot deserialize Scalar_Generator config data: {}", ss.str());
        return false;
    }

    m_config = sz;

    return true;
}

template<class Stream_t>
auto Scalar_Generator<Stream_t>::send_message(rapidjson::Value const& /*json*/) -> rapidjson::Document
{
    return rapidjson::Document();
}
template<class Stream_t>
auto Scalar_Generator<Stream_t>::get_config() const -> rapidjson::Document
{
    rapidjson::Document json;
    autojsoncxx::to_document(m_config, json);
    return std::move(json);
}

template<class Stream_t>
auto Scalar_Generator<Stream_t>::get_stream_inputs() const -> std::vector<Stream_Input>
{
    std::vector<Stream_Input> inputs =
    {{
        { stream::IFloat::TYPE, m_init_params.rate, "Modulation", m_modulation_stream_path }
    }};
    return inputs;
}
template<class Stream_t>
auto Scalar_Generator<Stream_t>::get_stream_outputs() const -> std::vector<Stream_Output>
{
    std::vector<Stream_Output> outputs(1);
    outputs[0].type = Stream_t::TYPE;
    outputs[0].name = "Output";
    outputs[0].stream = m_output_stream;
    return outputs;
}

template<class Stream_t>
void Scalar_Generator<Stream_t>::process()
{
    QLOG_TOPIC("scalar_generator::process");

    m_output_stream->clear();

    auto modulation_stream = m_modulation_stream.lock();
    if (modulation_stream)
    {
        auto const& samples = modulation_stream->get_samples();
        for (auto const& s: samples)
        {
            m_output_stream->push_sample(m_config.value + s.value, s.is_healthy);
        }
    }
    else
    {
        auto samples_needed = m_output_stream->compute_samples_needed();
        while (samples_needed > 0)
        {
            m_output_stream->push_sample(m_config.value, true);
            samples_needed--;
        }
    }
}



}
}
