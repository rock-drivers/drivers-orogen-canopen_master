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

class canopen_master::HeartbeatScope
{
    SlaveTask& m_task;
    Slave& m_slave;
    base::Time m_period;
    base::Time m_current_period;
    base::Time m_timeout;

public:
    HeartbeatScope(SlaveTask& task,
                   base::Time const& period,
                   base::Time const& current_period,
                   base::Time const& timeout = base::Time::fromSeconds(1))
        : m_task(task)
        , m_slave(*task.m_slave)
        , m_period(period)
        , m_current_period(current_period)
        , m_timeout(timeout) {

        m_task.writeSDO(
            m_slave.queryDownload<ProducerHeartbeatTime>(
                period.toMilliseconds()
            ),
            m_timeout
        );
    }

    ~HeartbeatScope() {
        if (m_period == m_current_period) {
            return;
        }

        m_task.writeSDO(
            m_slave.queryDownload<ProducerHeartbeatTime>(
                m_current_period.toMilliseconds()
            ),
            m_timeout
        );
    }
};

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
    if (transition == NODE_RESET || transition == NODE_RESET_COMMUNICATION) {
        toNMTStateInternal(desiredState, transition, timeout);
    }
    else {
        HeartbeatScope heartbeat(
            *this, heartbeat_period, m_heartbeat_period, timeout
        );
        toNMTStateInternal(desiredState, transition, timeout);
    }
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
