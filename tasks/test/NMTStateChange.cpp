/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "NMTStateChange.hpp"
#include <canopen_master/Objects.hpp>
#include <canopen_master/Slave.hpp>
#include <canopen_master/StateMachine.hpp>

using namespace canopen_master::test;

NMTStateChange::NMTStateChange(std::string const& name)
    : NMTStateChangeBase(name)
{
}

NMTStateChange::~NMTStateChange()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See NMTStateChange.hpp for more detailed
// documentation about them.

bool NMTStateChange::configureHook()
{
    if (! NMTStateChangeBase::configureHook()) {
        return false;
    }

    m_state_machine = new StateMachine(_node_id.get());
    m_slave = new Slave(*m_state_machine);
    return true;
}
bool NMTStateChange::startHook()
{
    if (! NMTStateChangeBase::startHook()) {
        return false;
    }
    return true;
}
void NMTStateChange::updateHook()
{
    toNMTState(canopen_master::NODE_OPERATIONAL,
               canopen_master::NODE_START,
               base::Time::fromMilliseconds(100),
               base::Time::fromMilliseconds(500));
    stop();
    NMTStateChangeBase::updateHook();
}
void NMTStateChange::errorHook()
{
    NMTStateChangeBase::errorHook();
}
void NMTStateChange::stopHook()
{
    NMTStateChangeBase::stopHook();
}
void NMTStateChange::cleanupHook()
{
    NMTStateChangeBase::cleanupHook();
}
