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
    if (! SlaveTaskBase::configureHook())
        return false;
    return true;
}
bool SlaveTask::startHook()
{
    if (! SlaveTaskBase::startHook())
        return false;
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
    uint32_t m_current_period;

public:
    HeartbeatScope(SlaveTask& task,
                   base::Time const& period,
                   base::Time const& timeout = base::Time::fromSeconds(1))
        : m_task(task)
        , m_slave(*task.m_slave)
        , m_current_period(m_slave.get<ProducerHeartbeatTime>()) {

        m_task.writeSDO(
            m_slave.queryDownload<ProducerHeartbeatTime>(period.toMilliseconds()),
            timeout
        );
    }
};

NODE_STATE SlaveTask::getNMTState(base::Time deadline) {
    // Setup a fast heartbeat time to get
    HeartbeatScope heartbeat(*this, base::Time::fromMilliseconds(10));
    _can_out.write(m_slave->queryNodeState());

    while(base::Time::now() < deadline)
    {
        usleep(POLL_PERIOD_US);

        canbus::Message msg;
        while (_can_in.read(msg, false) == RTT::NewData) {
            auto update = m_slave->process(msg);
            if (update == StateMachine::PROCESSED_HEARTBEAT) {
                return m_slave->getNodeState();
            }
        }
    }

    throw NMTTimeout();
}

void SlaveTask::toNMTState(NODE_STATE desiredState,
    NODE_STATE_TRANSITION transition,
    base::Time timeout)
{
    base::Time deadline = base::Time::now() + timeout;
    auto currentState = getNMTState(deadline);
    if (currentState == desiredState)
        return;

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1000, 5000);

    _can_out.write(m_slave->queryNodeStateTransition(transition));
    while (base::Time::now() < deadline) {
        usleep(NMT_DEAD_TIME_US);
        try {
            auto state = getNMTState(base::Time::now() + base::Time::fromMilliseconds(100));
            if (desiredState == NODE_PRE_OPERATIONAL && state == 0x10) {
                return;
            }
            if (state == desiredState) {
                return;
            }
        }
        catch(NMTTimeout const&) {
            usleep(distribution(generator));
            _can_out.write(m_slave->queryNodeStateTransition(transition));
        }
        catch(EmergencyMessageReceived const&) {
            usleep(distribution(generator));
            _can_out.write(m_slave->queryNodeStateTransition(transition));
        }
    }
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
    while(true)
    {
        usleep(POLL_PERIOD_US);

        canbus::Message msg;
        if (_can_in.read(msg, false) == RTT::NewData) {
            auto update = m_slave->process(msg);
            if (update == StateMachine::PROCESSED_SDO &&
                update.hasUpdatedObject(objectId, objectSubId)) {
                return;
            }
        }
        if (base::Time::now() > deadline) {
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
    while(true)
    {
        canbus::Message msg;
        if (_can_in.read(msg, false) == RTT::NewData) {
            auto update = m_slave->process(msg);
            if (update == StateMachine::PROCESSED_SDO_INITIATE_DOWNLOAD &&
                update.hasUpdatedObject(objectId, objectSubId)) {
                return;
            }
        }
        if (base::Time::now() > deadline) {
            throw SDOWriteTimeout();
        }
        usleep(POLL_PERIOD_US);
    }
}


