#include "QMLHUD.h"

static QGeoCoordinate llaToQGeoCoordinate(util::coordinates::LLA const& lla)
{
    double latitude = math::degrees(lla.latitude);
    if (latitude > 90.0)
    {
        latitude -= 360.0;
    }
    if (latitude < -90.0)
    {
        latitude += 360.0;
    }
    double longitude = math::degrees(lla.longitude);
    if (longitude > 180.0)
    {
        longitude -= 360.0;
    }
    if (longitude < -180.0)
    {
        longitude += 360.0;
    }

    return QGeoCoordinate(latitude, longitude, lla.altitude);
}

static QGeoCoordinate ecefToQGeoCoordinate(util::coordinates::ECEF const& ecef)
{
    util::coordinates::LLA lla = util::coordinates::ecef_to_lla(ecef);
    return llaToQGeoCoordinate(lla);
}



QMLHUD::QMLHUD(QObject *parent)
    : QObject(parent)
{
}

void QMLHUD::init(silk::HAL& hal)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    m_hal = &hal;

    m_multirotorState = m_hal->get_comms().get_multirotor_state();

    setMode(m_multirotorState.mode);

    //start up in a safe state if flying
    if (m_multirotorState.mode == Mode::FLY)
    {
        setVerticalMode(VerticalMode::ALTITUDE);
        setHorizontalMode(HorizontalMode::POSITION);
        setYawMode(YawMode::ANGLE);
    }

    m_isInitialized = true;
}


QMLHUD::Mode QMLHUD::mode() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorCommands.mode;
}
void QMLHUD::setMode(Mode newMode)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);

    silk::stream::IMultirotor_Commands::Mode oldMode = m_multirotorCommands.mode;
    if (oldMode == newMode && m_isInitialized)
    {
        return;
    }

    //m_last_mode_change_tp = Clock::now();

    if (newMode == Mode::IDLE)
    {
        //input.get_haptic().vibrate(k_alert_haptic);
        //m_idle_mode_data.is_pressed = false;
        setVerticalMode(VerticalMode::THRUST);
        setHorizontalMode(HorizontalMode::ANGLE);
    }

    if (oldMode == Mode::IDLE)
    {
        //input.get_haptic().vibrate(k_alert_haptic);
    }
    else if (oldMode == Mode::RETURN_HOME)
    {
        //when leaving RTH, these are the best modes
        setVerticalMode(VerticalMode::ALTITUDE);
        setHorizontalMode(HorizontalMode::POSITION);
        //no need to change the yaw as it's user controllable in RTH
    }

    bool oldConfirmed = isModeConfirmed();

    m_multirotorCommands.mode = newMode;
    emit modeChanged();

    if (isModeConfirmed() != oldConfirmed)
    {
        emit modeConfirmedChanged();
    }
}

bool QMLHUD::isModeConfirmed() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorCommands.mode == m_multirotorState.mode;
}

QMLHUD::VerticalMode QMLHUD::verticalMode() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorCommands.vertical_mode;
}
void QMLHUD::setVerticalMode(VerticalMode newMode)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    if (m_multirotorCommands.vertical_mode == newMode && m_isInitialized)
    {
        return;
    }

    //m_last_vertical_mode_change_tp = Clock::now();
    //input.get_haptic().vibrate(k_mode_change_haptic);

    if (newMode == VerticalMode::ALTITUDE)
    {
        //input.get_sticks().set_throttle_deadband_position(ISticks::Deadband_Position::MIDDLE);
        m_multirotorCommands.sticks.throttle = 0.5f;
    }
    else
    {
        //input.get_sticks().set_throttle_deadband_position(ISticks::Deadband_Position::LOW);
        m_multirotorCommands.sticks.throttle = m_multirotorState.throttle;
    }
    //input.get_stick_actuators().set_target_throttle(m_multirotor_commands.sticks.throttle);

    bool oldConfirmed = isVerticalModeConfirmed();

    m_multirotorCommands.vertical_mode = newMode;
    emit verticalModeChanged();

    if (isVerticalModeConfirmed() != oldConfirmed)
    {
        emit verticalModeConfirmedChanged();
    }
}
bool QMLHUD::isVerticalModeConfirmed() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return true;
}

QMLHUD::HorizontalMode QMLHUD::horizontalMode() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorCommands.horizontal_mode;
}
void QMLHUD::setHorizontalMode(HorizontalMode newMode)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    if (m_multirotorCommands.horizontal_mode == newMode && m_isInitialized)
    {
        return;
    }

    //m_last_horizontal_mode_change_tp = Clock::now();
    //input.get_haptic().vibrate(k_mode_change_haptic);

    bool oldConfirmed = isHorizontalModeConfirmed();

    m_multirotorCommands.horizontal_mode = newMode;
    emit horizontalModeChanged();

    if (isHorizontalModeConfirmed() != oldConfirmed)
    {
        emit horizontalModeConfirmedChanged();
    }
}
bool QMLHUD::isHorizontalModeConfirmed() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return true;
}

QMLHUD::YawMode QMLHUD::yawMode() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorCommands.yaw_mode;
}
void QMLHUD::setYawMode(YawMode newMode)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    if (m_multirotorCommands.yaw_mode == newMode && m_isInitialized)
    {
        return;
    }

    //m_last_yaw_mode_change_tp = Clock::now();
    //input.get_haptic().vibrate(k_mode_change_haptic);
    bool oldConfirmed = isYawModeConfirmed();

    m_multirotorCommands.yaw_mode = newMode;
    emit yawModeChanged();

    if (isYawModeConfirmed() != oldConfirmed)
    {
        emit yawModeConfirmedChanged();
    }
}
bool QMLHUD::isYawModeConfirmed() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return true;
}

QMLHUD::StreamQuality QMLHUD::streamQuality() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_cameraCommands.quality;
}
void QMLHUD::setStreamQuality(StreamQuality quality)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    if (m_cameraCommands.quality != quality)
    {
        bool oldConfirmed = isStreamQualityConfirmed();

        m_cameraCommands.quality = quality;
        emit streamQualityChanged();

        if (isStreamQualityConfirmed() != oldConfirmed)
        {
            emit streamQualityConfirmedChanged();
        }
    }
}
bool QMLHUD::isStreamQualityConfirmed() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return true;
}

bool QMLHUD::isRecording() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_cameraCommands.recording;
}
void QMLHUD::setRecording(bool recording)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    if (m_cameraCommands.recording != recording)
    {
        bool oldConfirmed = isRecordingConfirmed();

        m_cameraCommands.recording = recording;
        emit recordingChanged();

        if (isRecordingConfirmed() != oldConfirmed)
        {
            emit recordingConfirmedChanged();
        }
    }
}
bool QMLHUD::isRecordingConfirmed() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return true;
}

float QMLHUD::batteryChargeUsed() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorState.battery_state.charge_used;
}
float QMLHUD::batteryAverageVoltage() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorState.battery_state.average_voltage;
}
float QMLHUD::batteryAverageCurrent() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorState.battery_state.average_current;
}
float QMLHUD::batteryCapacityLeft() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorState.battery_state.capacity_left;
}

int QMLHUD::radioTxRSSI() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_hal->get_comms().get_tx_dBm();
}
int QMLHUD::radioRxRSSI() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_hal->get_comms().get_rx_dBm();
}
int QMLHUD::videoRxRSSI() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_hal->get_comms().get_video_streamer().get_input_dBm();
}

QVector3D QMLHUD::localFrameEuler() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    math::vec3f euler;
    m_multirotorState.local_frame.get_as_euler_zxy(euler);
    return QVector3D(euler.x, euler.y, euler.z);
}
QQuaternion QMLHUD::localFrame() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    math::quatf const& q = m_multirotorState.local_frame;
    return QQuaternion(q.w, q.x, q.y, q.z);
}
QGeoCoordinate QMLHUD::homePosition() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    if (!m_multirotorState.home_ecef_position.is_initialized())
    {
        return QGeoCoordinate();
    }
    return ecefToQGeoCoordinate(*m_multirotorState.home_ecef_position);
}
QGeoCoordinate QMLHUD::position() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return ecefToQGeoCoordinate(m_multirotorState.ecef_position);
}
QVector3D QMLHUD::localVelocity() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    math::quatf enuToLocal = math::inverse(m_multirotorState.local_frame);
    math::vec3f v = math::rotate(enuToLocal, m_multirotorState.enu_velocity);
    return QVector3D(v.x, v.y, v.z);
}
QVector3D QMLHUD::enuVelocity() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    math::vec3f const& v = m_multirotorState.enu_velocity;
    return QVector3D(v.x, v.y, v.z);
}
float QMLHUD::gimbalPitch() const
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    return m_multirotorCommands.gimbal_pitch;
}
void QMLHUD::setGimbalPitch(float pitch)
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
    if (m_multirotorCommands.gimbal_pitch != pitch)
    {
        m_multirotorCommands.gimbal_pitch = pitch;
        emit gimbalPitchChanged();
    }
}

void QMLHUD::process()
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);

    QASSERT(m_isInitialized);
    if (!m_isInitialized)
    {
        return;
    }

    Clock::time_point now = Clock::now();
//    Clock::duration dt = now - m_last_tp;
//    m_last_tp = now;

//    if (input.get_menu_switch().was_released())
//    {
//        //leave the uav in a safe state
//        if (m_multirotor_state.mode == stream::IMultirotor_State::Mode::FLY)
//        {
//            set_vertical_mode(input, Vertical_Mode::ALTITUDE);
//            set_horizontal_mode(input, Horizontal_Mode::POSITION);
//            set_yaw_mode(input, Yaw_Mode::ANGLE);
//        }

//        return false;
//    }

    silk::Comms& comms = m_hal->get_comms();

    m_multirotorState = comms.get_multirotor_state();

    emit telemetryChanged();

//    m_rx_strength = math::lerp<math::safe>(m_rx_strength, dBm_to_strength(comms.get_rx_dBm()), std::chrono::duration<float>(dt).count());
//    m_slow_rx_strength = math::lerp<math::safe>(m_slow_rx_strength, m_rx_strength, std::chrono::duration<float>(dt).count() / 5.f);

//    m_tx_strength = math::lerp<math::safe>(m_tx_strength, dBm_to_strength(comms.get_tx_dBm()), std::chrono::duration<float>(dt).count());
//    m_slow_tx_strength = math::lerp<math::safe>(m_slow_tx_strength, m_tx_strength, std::chrono::duration<float>(dt).count() / 5.f);

    silk::stream::IMultirotor_State::Value const& state = m_multirotorState;
    if (state.mode != m_multirotorCommands.mode)
    {
//        if (now - m_last_mode_change_tp >= std::chrono::seconds(2))
//        {
//            QLOGI("REVERTED to mode {}!!!", state.mode);
//            setMode(state.mode);
//            //input.get_haptic().vibrate(k_alert_haptic);
//            return true;
//        }
    }
    else
    {
        switch (m_multirotorCommands.mode)
        {
        case Mode::IDLE: processModeIdle(); break;
        case Mode::TAKE_OFF: processModeTakeOff(); break;
        case Mode::FLY: processModeFly(); break;
        case Mode::RETURN_HOME: processModeReturnHome(); break;
        case Mode::LAND: processModeLand(); break;
        }
    }

    //m_multirotorCommands.gimbal_pitch = m_hal.get_gimbal_control().get_pitch();

    comms.send_multirotor_commands_value(m_multirotorCommands);
    comms.send_camera_commands_value(m_cameraCommands);
}

void QMLHUD::processModeIdle()
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);
}

void QMLHUD::processModeTakeOff()
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);

}

void QMLHUD::processModeFly()
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);

    silk::stream::IMultirotor_State::Value const& state = m_multirotorState;
    QASSERT(state.mode == Mode::FLY);

//    m_multirotor_commands.sticks.yaw = sticks.get_yaw();
//    m_multirotor_commands.sticks.pitch = sticks.get_pitch();
//    m_multirotor_commands.sticks.roll = sticks.get_roll();

//    //only apply the throttle some time after the mode chang, to let the actuator settle
//    if (Clock::now() - m_last_vertical_mode_change_tp > std::chrono::milliseconds(200))
//    {
//        if (m_multirotor_commands.vertical_mode == VerticalMode::THRUST)
//        {
//            input.get_stick_actuators().set_target_throttle(boost::none);
//        }

//        m_multirotor_commands.sticks.throttle = sticks.get_throttle();
//    }
}

void QMLHUD::processModeReturnHome()
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);

}

void QMLHUD::processModeLand()
{
    std::lock_guard<std::recursive_mutex> lg(m_mutex);

}