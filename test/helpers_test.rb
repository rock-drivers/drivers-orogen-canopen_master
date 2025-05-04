# frozen_string_literal: true

require_relative "../tasks/test_helpers"

using_task_library "canopen_master"

describe OroGen.canopen_master.SlaveTask do
    run_live

    include CANOpen::TestHelpers

    describe "SDO Upload" do
        attr_reader :task
        before do
            @task = syskit_deploy(
                OroGen.canopen_master.test.ReadSDO.deployed_as("read_sdo")
            )
            task.properties.node_id = 0x42
            syskit_configure(task)

            @can_in = task.can_in_port
            @can_out = task.can_out_port
        end

        it "reads a SDO" do
            canopen_set_bytes 0x210D, 0x2, [0x1, 0x90]
            value = expect_canopen_interaction(@can_in, @can_out, 0x42) { task.start! }
                .to { have_one_new_sample task.received_values_port }
            assert_equal 0x190, value
        end

        it "fails if the SDO is not set" do
            assert_raises(CANOpenTestHelpers::SDONotSet) do
                expect_canopen_interaction(@can_in, @can_out, 0x42) { task.start! }
                    .to { have_one_new_sample task.received_values_port }
            end
        end
    end

    describe "SDO Download" do
        attr_reader :task
        before do
            @task = syskit_deploy(
                OroGen.canopen_master.test.WriteSDO.deployed_as("read_sdo")
            )
            task.properties.node_id = 0x42
            syskit_configure(task)

            @can_in = task.can_in_port
            @can_out = task.can_out_port
        end

        it "receives a SDO write" do
            value = expect_canopen_interaction(@can_in, @can_out, 0x42) { task.start! }
                .to { emit task.stop_event }
            assert_equal [0x1, 0x90], canopen_get_bytes(0x210D, 0x2)
        end
    end
end
