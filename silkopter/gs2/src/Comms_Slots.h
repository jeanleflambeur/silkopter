#pragma once

#include "Comms.h"
#include "common/config/Multirotor_Config.h"

class Comms_Slots
{
public:
    Comms_Slots(silk::Comms& comms);

    virtual void refresh();

private:
    virtual void slot_clock_received(silk::Manual_Clock::time_point tp) = 0;
    virtual void slot_reset() = 0;
    virtual void slot_node_defs_reset() = 0;
    virtual void slot_node_defs_added(std::vector<silk::Comms::Node_Def> const&) = 0;
    virtual void slot_nodes_reset() = 0;
    virtual void slot_nodes_added(std::vector<silk::Comms::Node> const&) = 0;
    virtual void slot_node_changed(silk::Comms::Node const&) = 0;
    virtual void slot_node_removed(std::string const& name) = 0;
    virtual void slot_node_message_received(std::string const& name, rapidjson::Value const& message) = 0;
    virtual void slot_node_config_received(std::string const& name, rapidjson::Value const& json) = 0;
    virtual void slot_stream_data_received(silk::Comms::IStream_Data const&) = 0;
    virtual void slot_uav_config_received(boost::optional<silk::UAV_Config&> config) = 0;

    struct Connections
    {
        q::util::Scoped_Connection clock_received;
        q::util::Scoped_Connection reset;
        q::util::Scoped_Connection node_defs_reset;
        q::util::Scoped_Connection node_defs_added;
        q::util::Scoped_Connection nodes_reset;
        q::util::Scoped_Connection nodes_added;
        q::util::Scoped_Connection node_changed;
        q::util::Scoped_Connection node_removed;
        q::util::Scoped_Connection node_message_received;
        q::util::Scoped_Connection node_config_received;
        q::util::Scoped_Connection stream_data_received;
        q::util::Scoped_Connection uav_config_received;
    } m_connections;

    silk::Comms& m_comms;
};

