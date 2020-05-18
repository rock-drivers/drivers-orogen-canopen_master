/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "NMTMinimalState.hpp"
#include <canopen_master/Objects.hpp>
#include <canopen_master/Slave.hpp>
#include <canopen_master/StateMachine.hpp>

using namespace canopen_master::test;

NMTMinimalState::NMTMinimalState(std::string const& name)
    : NMTMinimalStateBase(name)
{
}

NMTMinimalState::~NMTMinimalState()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See NMTMinimalState.hpp for more detailed
// documentation about them.

bool NMTMinimalState::configureHook()
{
    if (! NMTMinimalStateBase::configureHook()) {
        return false;
    }

    m_state_machine = new StateMachine(_node_id.get());
    m_slave = new Slave(*m_state_machine);
    return true;
}
bool NMTMinimalState::startHook()
{
    if (! NMTMinimalStateBase::startHook()) {
        return false;
    }
    return true;
}
void NMTMinimalState::updateHook()
{
    toNMTState(
        static_cast<canopen_master::NODE_STATE>(_state_target.get()),
        static_cast<canopen_master::NODE_STATE_TRANSITION>(_state_transition.get()),
        base::Time::fromMilliseconds(500));

    stop();
    NMTMinimalStateBase::updateHook();
}
void NMTMinimalState::errorHook()
{
    NMTMinimalStateBase::errorHook();
}
void NMTMinimalState::stopHook()
{
    NMTMinimalStateBase::stopHook();
}
void NMTMinimalState::cleanupHook()
{
    NMTMinimalStateBase::cleanupHook();
}
