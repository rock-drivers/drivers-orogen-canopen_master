/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "WriteSDO.hpp"
#include <canopen_master/Objects.hpp>
#include <canopen_master/Slave.hpp>
#include <canopen_master/StateMachine.hpp>

using namespace canopen_master::test;

WriteSDO::WriteSDO(std::string const& name)
    : WriteSDOBase(name)
{
}

WriteSDO::~WriteSDO()
{
}

CANOPEN_DEFINE_OBJECT(0x210D, 0x02, TestObject, uint16_t);

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See WriteSDO.hpp for more detailed
// documentation about them.

bool WriteSDO::configureHook()
{
    if (! WriteSDOBase::configureHook()) {
        return false;
    }

    m_state_machine = new StateMachine(_node_id.get());
    m_slave = new Slave(*m_state_machine);
    return true;
}
bool WriteSDO::startHook()
{
    if (! WriteSDOBase::startHook()) {
        return false;
    }
    return true;
}
void WriteSDO::updateHook()
{
    m_slave->set<TestObject>(0x190);
    writeSDO(m_slave->queryDownload<TestObject>(),
             base::Time::fromMilliseconds(300));
    stop();
    WriteSDOBase::updateHook();
}
void WriteSDO::errorHook()
{
    WriteSDOBase::errorHook();
}
void WriteSDO::stopHook()
{
    WriteSDOBase::stopHook();
}
void WriteSDO::cleanupHook()
{
    WriteSDOBase::cleanupHook();
}
