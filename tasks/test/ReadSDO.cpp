/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "ReadSDO.hpp"
#include <canopen_master/Objects.hpp>
#include <canopen_master/Slave.hpp>
#include <canopen_master/StateMachine.hpp>

using namespace canopen_master::test;

ReadSDO::ReadSDO(std::string const& name)
    : ReadSDOBase(name)
{
}

ReadSDO::~ReadSDO()
{
}

CANOPEN_DEFINE_OBJECT(0x210D, 0x02, TestObject, uint16_t);

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See ReadSDO.hpp for more detailed
// documentation about them.

bool ReadSDO::configureHook()
{
    if (! ReadSDOBase::configureHook()) {
        return false;
    }

    m_state_machine = new StateMachine(_node_id.get());
    m_slave = new Slave(*m_state_machine);
    return true;
}
bool ReadSDO::startHook()
{
    if (! ReadSDOBase::startHook()) {
        return false;
    }
    return true;
}
void ReadSDO::updateHook()
{
    readSDO(m_slave->queryUpload<TestObject>(),
            base::Time::fromMilliseconds(300));
    _received_values.write(m_slave->get<TestObject>());
    ReadSDOBase::updateHook();
}
void ReadSDO::errorHook()
{
    ReadSDOBase::errorHook();
}
void ReadSDO::stopHook()
{
    ReadSDOBase::stopHook();
}
void ReadSDO::cleanupHook()
{
    delete m_slave;
    m_slave = nullptr;
    delete m_state_machine;
    m_state_machine = nullptr;
    ReadSDOBase::cleanupHook();
}
