/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef CANOPEN_MASTER_SLAVETASK_TASK_HPP
#define CANOPEN_MASTER_SLAVETASK_TASK_HPP

#include "canopen_master/SlaveTaskBase.hpp"
#include <base/Time.hpp>
#include <canopen_master/Frame.hpp>

namespace canopen_master{
    class Slave;
    struct HeartbeatScope;

    struct NMTTimeout : std::runtime_error
    {
        NMTTimeout()
            : std::runtime_error("NMT Transition Timed Out") {}
    };

    struct SDOWriteTimeout : std::runtime_error
    {
        SDOWriteTimeout()
            : std::runtime_error("SDO Write Timeout") {}
    };

    struct SDOReadTimeout : std::runtime_error
    {
        SDOReadTimeout()
            : std::runtime_error("SDO Read Timeout") {}
    };

    /*! \class SlaveTask
     * \brief The task context provides and requires services. It uses an ExecutionEngine to perform its functions.
     * Essential interfaces are operations, data flow ports and properties. These interfaces have been defined using the oroGen specification.
     * In order to modify the interfaces you should (re)use oroGen and rely on the associated workflow.
     * Task containing basic features needed to integrate a canopen_master driver
     * \details
     * The name of a TaskContext is primarily defined via:
     \verbatim
     deployment 'deployment_name'
         task('custom_task_name','canopen_master::SlaveTask')
     end
     \endverbatim
     *  It can be dynamically adapted when the deployment is called with a prefix argument.
     */
    class SlaveTask : public SlaveTaskBase
    {
        friend class SlaveTaskBase;
        friend class HeartbeatScope;

        void toNMTStateInternal(
            NODE_STATE desiredState,
            NODE_STATE_TRANSITION transition,
            base::Time timeout = base::Time::fromSeconds(1)
        );

    protected:
        Slave* m_slave = nullptr;
        base::Time m_heartbeat_period = base::Time::fromSeconds(1);

        base::Time getHeartbeatPeriod() const;
        void setHeartbeatPeriod(base::Time const& time);

        NODE_STATE getNMTState(base::Time deadline);
        void toNMTState(
            NODE_STATE desiredState,
            NODE_STATE_TRANSITION transition,
            base::Time heartbeat_period = base::Time::fromMilliseconds(100),
            base::Time timeout = base::Time::fromSeconds(1)
        );
        void readSDOs(std::vector<canbus::Message> const& queries,
                      base::Time timeout = base::Time::fromSeconds(1));
        void readSDO(canbus::Message const& query,
                     base::Time timeout = base::Time::fromSeconds(1));
        void writeSDOs(std::vector<canbus::Message> const& queries,
                       base::Time timeout = base::Time::fromSeconds(1));
        void writeSDO(canbus::Message const& query,
                      base::Time timeout = base::Time::fromSeconds(1));

        void writeProducerHeartbeatPeriod(
            base::Time const& period, base::Time const& timeout
        );

    public:
        /** TaskContext constructor for SlaveTask
         * \param name Name of the task. This name needs to be unique to make it identifiable via nameservices.
         * \param initial_state The initial TaskState of the TaskContext. Default is Stopped state.
         */
        SlaveTask(std::string const& name = "canopen_master::SlaveTask");

        /** Default deconstructor of SlaveTask
         */
	~SlaveTask();

        /** This hook is called by Orocos when the state machine transitions
         * from PreOperational to Stopped. If it returns false, then the
         * component will stay in PreOperational. Otherwise, it goes into
         * Stopped.
         *
         * It is meaningful only if the #needs_configuration has been specified
         * in the task context definition with (for example):
         \verbatim
         task_context "TaskName" do
           needs_configuration
           ...
         end
         \endverbatim
         */
        bool configureHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to Running. If it returns false, then the component will
         * stay in Stopped. Otherwise, it goes into Running and updateHook()
         * will be called.
         */
        bool startHook();

        /** This hook is called by Orocos when the component is in the Running
         * state, at each activity step. Here, the activity gives the "ticks"
         * when the hook should be called.
         *
         * The error(), exception() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeError, Exception and
         * FatalError states.
         *
         * In the first case, updateHook() is still called, and recover() allows
         * you to go back into the Running state.  In the second case, the
         * errorHook() will be called instead of updateHook(). In Exception, the
         * component is stopped and recover() needs to be called before starting
         * it again. Finally, FatalError cannot be recovered.
         */
        void updateHook();

        /** This hook is called by Orocos when the component is in the
         * RunTimeError state, at each activity step. See the discussion in
         * updateHook() about triggering options.
         *
         * Call recover() to go back in the Runtime state.
         */
        void errorHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Running to Stopped after stop() has been called.
         */
        void stopHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to PreOperational, requiring the call to configureHook()
         * before calling start() again.
         */
        void cleanupHook();
    };
}

#endif

