/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SlaveTask.hpp"
#include <canopen_master/SDO.hpp>
#include <canopen_master/Slave.hpp>

#include <random>

using namespace canopen_master;

SlaveTask::SlaveTask(std::string const& name)
    : SlaveTaskBase(name)
{
}

SlaveTask::~SlaveTask()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See SlaveTask.hpp for more detailed
// documentation about them.

bool SlaveTask::configureHook()
{
    if (! SlaveTaskBase::configureHook()) {
        return false;
    }
    return true;
}
bool SlaveTask::startHook()
{
    if (! SlaveTaskBase::startHook()) {
        return false;
    }
    return true;
}
void SlaveTask::updateHook()
{
    SlaveTaskBase::updateHook();
}
void SlaveTask::errorHook()
{
    SlaveTaskBase::errorHook();
}
void SlaveTask::stopHook()
{
    SlaveTaskBase::stopHook();
}
void SlaveTask::cleanupHook()
{
    SlaveTaskBase::cleanupHook();
}

static const int POLL_PERIOD_US = 10000;
static const int NMT_DEAD_TIME_US = 10000;

base::Time SlaveTask::getHeartbeatPeriod() const {
    return m_heartbeat_period;
}
void SlaveTask::setHeartbeatPeriod(base::Time const& time) {
    m_heartbeat_period = time;
}

void SlaveTask::toNMTState(NODE_STATE desiredState,
    NODE_STATE_TRANSITION transition,
    base::Time heartbeat_period,
    base::Time timeout
) {
    switch (transition) {
        case NODE_RESET:
        case NODE_RESET_COMMUNICATION:
        case NODE_STOP:
            toNMTStateInternal(desiredState, transition, timeout);
            break;
        default:
            writeProducerHeartbeatPeriod(heartbeat_period, timeout);
            try {
                toNMTStateInternal(desiredState, transition, timeout);
            }
            catch (...) {
                writeProducerHeartbeatPeriod(m_heartbeat_period, timeout);
                throw;
            }
            writeProducerHeartbeatPeriod(m_heartbeat_period, timeout);
            break;
    }
}

void SlaveTask::writeProducerHeartbeatPeriod(
    base::Time const& period, base::Time const& timeout
) {
    writeSDO(
        m_slave->queryDownload<ProducerHeartbeatTime>(period.toMilliseconds()),
        timeout
    );
}

void SlaveTask::toNMTStateInternal(
    NODE_STATE desiredState,
    NODE_STATE_TRANSITION transition,
    base::Time timeout
) {
    base::Time deadline = base::Time::now() + timeout;

    // Clear queued messages. We assume that if some heartbeat arrives in the
    // meantime with the desired state, it means we *are* in this state
    _can_in.clear();
    _can_out.write(m_slave->queryNodeStateTransition(transition));

    while (base::Time::now() < deadline) {
        usleep(NMT_DEAD_TIME_US);

        canbus::Message message;
        while (_can_in.read(message, false) == RTT::NewData) {
            auto update = m_slave->process(message);
            if (update.mode == StateMachine::PROCESSED_HEARTBEAT) {
                if (m_slave->getNodeState() == desiredState) {
                    return;
                }
            }
        }
    }
    exception(NMT_TIMEOUT);
    throw NMTTimeout();
}

void SlaveTask::readSDOs(std::vector<canbus::Message> const& queries,
    base::Time timeout)
{
    for (auto const& query : queries) {
        readSDO(query, timeout);
    }
}
void SlaveTask::readSDO(canbus::Message const& query,
    base::Time timeout)
{
    _can_out.write(query);
    int objectId = getSDOObjectID(query);
    int objectSubId = getSDOObjectSubID(query);

    base::Time deadline = base::Time::now() + timeout;
    while (true)
    {
        usleep(POLL_PERIOD_US);

        canbus::Message msg;
        if (_can_in.read(msg, false) == RTT::NewData) {
            auto update = m_slave->process(msg);
            if (update.mode == StateMachine::PROCESSED_SDO &&
                update.hasUpdatedObject(objectId, objectSubId)) {
                return;
            }
        }
        if (base::Time::now() > deadline) {
            exception(SDO_TIMEOUT);
            throw SDOReadTimeout();
        }
    }
}

void SlaveTask::writeSDOs(std::vector<canbus::Message> const& queries,
    base::Time timeout)
{
    for (auto const& query : queries) {
        writeSDO(query, timeout);
    }
}
void SlaveTask::writeSDO(canbus::Message const& query, base::Time timeout)
{
    _can_out.write(query);

    uint16_t objectId = getSDOObjectID(query);
    uint16_t objectSubId = getSDOObjectSubID(query);

    base::Time deadline = base::Time::now() + timeout;
    while (true)
    {
        canbus::Message msg;
        if (_can_in.read(msg, false) == RTT::NewData) {
            auto update = m_slave->process(msg);
            if (update.mode == StateMachine::PROCESSED_SDO_INITIATE_DOWNLOAD &&
                update.hasUpdatedObject(objectId, objectSubId)) {
                return;
            }
        }
        if (base::Time::now() > deadline) {
            exception(SDO_TIMEOUT);
            throw SDOWriteTimeout();
        }
        usleep(POLL_PERIOD_US);
    }
}
