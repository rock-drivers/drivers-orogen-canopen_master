# frozen_string_literal: true

using_task_library "canopen_master"

describe OroGen.canopen_master.SlaveTask do
    run_live

    describe "SDO read" do
        attr_reader :task
        before do
            @task = syskit_deploy(
                OroGen.canopen_master.test.ReadSDO.deployed_as("read_sdo")
            )
            task.properties.node_id = 0x42
            syskit_configure(task)
            @sample = expect_execution { task.start! }
                      .to { have_one_new_sample task.can_out_port }
        end

        it "reads a SDO" do
            assert_sdo_upload(@sample, 0x42, 0x210D, 0x2)

            # SDO Upload Reply, 2 bytes, ID=0x210D Sub=2, data = 0x190
            sdo_transmit = make_sdo_upload_reply(0x42, 0x210D, 0x2, 0x1, 0x90)
            value = expect_execution { syskit_write task.can_in_port, sdo_transmit }
                    .to { have_one_new_sample task.received_values_port }
            assert_equal 0x190, value
        end

        it "times out of the SDO upload reply is not received" do
            expect_execution.to { emit task.sdo_timeout_event }
        end

        it "ignores SDO upload replies not for the read object ID" do
            sdo_transmit = make_sdo_upload_reply(0x42, 0x200D, 0x2, 0x1, 0x90)
            expect_execution { syskit_write task.can_in_port, sdo_transmit }
                .to { emit task.sdo_timeout_event }
        end

        it "ignores SDO upload replies not for the read object subID" do
            sdo_transmit = make_sdo_upload_reply(0x42, 0x210D, 0x1, 0x1, 0x90)
            expect_execution { syskit_write task.can_in_port, sdo_transmit }
                .to { emit task.sdo_timeout_event }
        end
    end

    describe "SDO write" do
        attr_reader :task
        before do
            @task = syskit_deploy(
                OroGen.canopen_master.test.WriteSDO.deployed_as("write_sdo")
            )
            task.properties.node_id = 0x42
            syskit_configure(task)
            @sample = expect_execution { task.start! }
                      .to { have_one_new_sample task.can_out_port }
        end

        it "writes a SDO" do
            assert_sdo_download(@sample, 0x42, 0x210D, 0x2, 0x1, 0x90)

            # SDO Download Reply, 2 bytes, ID=0x210D Sub=2
            sdo_reply = make_sdo_download_reply(0x42, 0x210D, 0x2)
            expect_execution { syskit_write task.can_in_port, sdo_reply }
                .to { emit task.stop_event }
        end

        it "times out of the SDO download reply is not received" do
            expect_execution.to { emit task.sdo_timeout_event }
        end

        it "ignores SDO download replies not for the read object ID" do
            sdo_reply = make_sdo_download_reply(0x42, 0x200D, 0x2)
            expect_execution { syskit_write task.can_in_port, sdo_reply }
                .to { emit task.sdo_timeout_event }
        end

        it "ignores SDO upload replies not for the read object subID" do
            sdo_reply = make_sdo_download_reply(0x42, 0x210D, 0x1)
            expect_execution { syskit_write task.can_in_port, sdo_reply }
                .to { emit task.sdo_timeout_event }
        end
    end

    describe "NMT RESET transition" do
        attr_reader :task
        before do
            @task = syskit_deploy(
                OroGen.canopen_master.test.NMTResetCommunication.deployed_as("nmt")
            )
            task.properties.node_id = 0x42
            syskit_configure(task)
            @sample = expect_execution { task.start! }
                      .to { have_one_new_sample task.can_out_port }
        end

        it "sends the NMT reset communication message and waits for the node "\
           "to send the bootup message" do
            assert_nmt(@sample, 0x42, 130)
            expect_on_can_in(make_heartbeat(0x42, 0))
                .to { emit task.stop_event }
        end

        it "ignores a heartbeat from another node" do
            expect_on_can_in(make_heartbeat(0x41, 0))
                .to { emit task.nmt_timeout_event }
        end

        it "ignores a heartbeat that indicate a different state than pre-operational" do
            expect_on_can_in(make_heartbeat(0x42, 1))
                .to { emit task.nmt_timeout_event }
        end
    end

    describe "NMT transition that is not RESET" do
        attr_reader :task
        before do
            @task = syskit_deploy(
                OroGen.canopen_master.test.NMTStateChange.deployed_as("nmt")
            )
            task.properties.node_id = 0x42
            syskit_configure(task)
            @sample = expect_execution { task.start! }
                      .to { have_one_new_sample task.can_out_port }
        end

        it "sets hearbeat, sends the NMT message and waits for the node to report "\
           "the new NMT state and then reinitializes the heartbeat period" do
            # heartbeat producer, 100ms
            assert_sdo_download(@sample, 0x42, 0x1017, 0, 0, 0x64)
            @sample = expect_on_can_in(make_sdo_download_reply(0x42, 0x1017, 0))
                      .to { have_one_new_sample task.can_out_port }
            assert_nmt(@sample, 0x42, 1) # start remote node
            # heartbeat, operational
            @sample = expect_on_can_in(make_heartbeat(0x42, 5))
                      .to { have_one_new_sample task.can_out_port }
            # heartbeat producer, 1s
            assert_sdo_download(@sample, 0x42, 0x1017, 0, 0x3, 0xE8)
            # heartbeat, operational
            expect_on_can_in(make_sdo_download_reply(0x42, 0x1017, 0))
                .to { emit task.stop_event }
        end

        it "times out if the heartbeat with the expected state is not received" do
            expect_on_can_in(make_sdo_download_reply(0x42, 0x1017, 0))
                .to { have_one_new_sample task.can_out_port }
            @sample = expect_on_can_in(make_heartbeat(0x42, 4))
                      .to do
                          emit task.nmt_timeout_event
                          have_one_new_sample task.can_out_port
                      end

            # Reestablish the timeout
            assert_sdo_download(@sample, 0x42, 0x1017, 0, 0x3, 0xE8)
            expect_on_can_in(make_sdo_download_reply(0x42, 0x1017, 0))
        end
    end

    def assert_sdo_upload(sample, node_id, object_id, object_sub_id)
        assert_equal (0x600 + node_id), sample.can_id
        assert_equal 8, sample.size

        expected_data = [
            0x40,
            object_id & 0xFF, ((object_id & 0xFF00) >> 8),
            object_sub_id,
            0, 0, 0, 0
        ]
        assert_equal expected_data, sample.data.to_a
    end

    def make_sdo_upload_reply(node_id, object_id, object_sub_id, *bytes) # rubocop:disable Metrics/AbcSize
        Types.canbus.Message.new(
            time: Time.now,
            can_id: (0x580 + node_id), size: 8,
            data: [
                0x43 | ((4 - bytes.size) << 2),
                (object_id & 0xFF), (object_id & 0xFF00) >> 8,
                object_sub_id,
                *bytes.reverse, *Array.new(4 - bytes.size, 0)
            ]
        )
    end

    def assert_sdo_download(sample, node_id, object_id, object_sub_id, *bytes) # rubocop:disable Metrics/AbcSize
        assert_equal (0x600 + node_id), sample.can_id, "unexpected CAN ID"
        assert_equal 8, sample.size, "unexpected CAN package size"

        expected_data = [
            0x23 | (4 - bytes.size) << 2,
            (object_id & 0xFF), (object_id & 0xFF00) >> 8,
            object_sub_id,
            *bytes.reverse, *Array.new(4 - bytes.size, 0)
        ]
        assert_equal expected_data, sample.data.to_a
    end

    def make_sdo_download_reply(node_id, object_id, object_sub_id)
        Types.canbus.Message.new(
            time: Time.now,
            can_id: (0x580 + node_id), size: 8,
            data: [
                0x60,
                (object_id & 0xFF), (object_id & 0xff00) >> 8,
                object_sub_id, 0, 0, 0, 0
            ]
        )
    end

    def assert_nmt(sample, node_id, command)
        assert_equal 0, sample.can_id, "unexpectedc CAN ID"
        assert_equal command, sample.data[0], "unexpected command ID"
        assert_equal node_id, sample.data[1], "unexpected node ID"
    end

    def make_heartbeat(node_id, state)
        Types.canbus.Message.new(
            time: Time.now,
            can_id: 1792 + node_id,
            size: 1,
            data: [state, 0, 0, 0, 0, 0, 0, 0]
        )
    end

    def expect_on_can_in(message)
        expect_execution { syskit_write task.can_in_port, message }
    end
end
