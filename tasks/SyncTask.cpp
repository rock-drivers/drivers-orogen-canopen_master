/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "SyncTask.hpp"
#include <canopen_master/StateMachine.hpp>

using namespace canopen_master;

SyncTask::SyncTask(std::string const& name)
    : SyncTaskBase(name)
{
}

SyncTask::SyncTask(std::string const& name, RTT::ExecutionEngine* engine)
    : SyncTaskBase(name, engine)
{
}

SyncTask::~SyncTask()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See SyncTask.hpp for more detailed
// documentation about them.

bool SyncTask::configureHook()
{
    if (! SyncTaskBase::configureHook())
        return false;
    return true;
}
bool SyncTask::startHook()
{
    if (! SyncTaskBase::startHook())
        return false;
    return true;
}
void SyncTask::updateHook()
{
    _can_out.write(canopen_master::StateMachine::sync());
    SyncTaskBase::updateHook();
}
void SyncTask::errorHook()
{
    SyncTaskBase::errorHook();
}
void SyncTask::stopHook()
{
    SyncTaskBase::stopHook();
}
void SyncTask::cleanupHook()
{
    SyncTaskBase::cleanupHook();
}
