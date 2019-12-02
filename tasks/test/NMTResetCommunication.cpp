/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "NMTResetCommunication.hpp"
#include <canopen_master/Objects.hpp>
#include <canopen_master/Slave.hpp>
#include <canopen_master/StateMachine.hpp>

using namespace canopen_master::test;

NMTResetCommunication::NMTResetCommunication(std::string const& name)
    : NMTResetCommunicationBase(name)
{
}

NMTResetCommunication::~NMTResetCommunication()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See NMTResetCommunication.hpp for more detailed
// documentation about them.

bool NMTResetCommunication::configureHook()
{
    if (! NMTResetCommunicationBase::configureHook()) {
        return false;
    }

    m_state_machine = new StateMachine(_node_id.get());
    m_slave = new Slave(*m_state_machine);
    return true;
}
bool NMTResetCommunication::startHook()
{
    if (! NMTResetCommunicationBase::startHook()) {
        return false;
    }
    return true;
}
void NMTResetCommunication::updateHook()
{
    toNMTState(canopen_master::NODE_PRE_OPERATIONAL,
               canopen_master::NODE_RESET_COMMUNICATION,
               base::Time::fromMilliseconds(500));
    stop();
    NMTResetCommunicationBase::updateHook();
}
void NMTResetCommunication::errorHook()
{
    NMTResetCommunicationBase::errorHook();
}
void NMTResetCommunication::stopHook()
{
    NMTResetCommunicationBase::stopHook();
}
void NMTResetCommunication::cleanupHook()
{
    NMTResetCommunicationBase::cleanupHook();
}
