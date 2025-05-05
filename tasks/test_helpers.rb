# frozen_string_literalÇ true

module CANOpen
    NMT_COMMAND_NODE_START = 0x01
    NMT_COMMAND_NODE_STOP  = 0x02
    NMT_COMMAND_NODE_ENTER_PRE_OPERATIONAL = 0x80
    NMT_COMMAND_NODE_RESET = 0x81
    NMT_COMMAND_NODE_RESET_COMMUNICATION = 0x82

    NMT_STATE_INITIALIZING = 0
    NMT_STATE_STOPPED = 4
    NMT_STATE_OPERATIONAL = 5
    NMT_STATE_PRE_OPERATIONAL = 0x7f

    module TestHelpers
        class SDONotSet < RuntimeError; end

        def setup
            @__canopen_objects = {}
            @__canopen_state = NMT_STATE_PRE_OPERATIONAL
            super
        end

        def canopen_set_state(state)
            @__canopen_state = state
        end

        def canopen_set_bytes(id, sub_id, bytes)
            @__canopen_objects[[id, sub_id]] = bytes
        end

        def canopen_get_bytes(id, sub_id)
            @__canopen_objects.fetch([id, sub_id])
        rescue KeyError
            raise SDONotSet, "trying to read SDO 0x#{id.to_s(16)} 0x#{sub_id.to_s(16)}, " \
                             "but it is not set"
        end

        def expect_canopen_interaction(can_in, can_out, node_id, &block)
            can_in_w = syskit_create_writer(can_in)
            can_out_r = syskit_create_reader(can_out)

            expect_execution(&block)
                .poll { canopen_handle_interaction(node_id, can_in_w, can_out_r) }
        end

        def canopen_handle_interaction(node_id, can_in, can_out)
            while (packet = can_out.read_new)
                if canopen_sdo_upload?(node_id, packet)
                    canopen_handle_sdo_upload(can_in, node_id, packet)
                elsif canopen_sdo_download?(node_id, packet)
                    canopen_handle_sdo_download(can_in, node_id, packet)
                elsif canopen_nmt_command?(node_id, packet)
                    canopen_handle_nmt_command(can_in, node_id, packet)
                else
                    puts "Ignored packet"
                    pp packet
                end
            end
        end

        def canopen_sdo_upload?(node_id, packet)
            packet.can_id == 0x600 + node_id &&
                packet.data[0] == 0x40
        end

        def canopen_handle_sdo_upload(can_in, node_id, packet)
            id, sub_id = canopen_sdo_object(packet)
            bytes = canopen_get_bytes(id, sub_id)
            reply = canopen_make_sdo_upload_reply(node_id, id, sub_id, *bytes)
            can_in.write(reply)
        end

        def canopen_sdo_object(packet)
            object_id_lsb = packet.data[1]
            object_id_msb = packet.data[2]
            object_sub_id = packet.data[3]
            [object_id_lsb + (object_id_msb << 8), object_sub_id]
        end

        def canopen_sdo_download?(node_id, packet)
            packet.can_id == 0x600 + node_id && packet.data[0] & 0x20 == 0x20
        end

        def canopen_handle_sdo_download(can_in, node_id, packet)
            id, sub_id = canopen_sdo_object(packet)
            size = 4 - ((packet.data[0] >> 2) & 0b11)
            canopen_set_bytes(id, sub_id, packet.data[4, size].to_a.reverse)
            reply = canopen_make_sdo_download_reply(node_id, id, sub_id)
            can_in.write(reply)
        end

        def canopen_nmt_command?(node_id, packet)
            packet.can_id == 0 && packet.data[1] == node_id
        end

        def canopen_handle_nmt_command(can_in, node_id, packet)
            case packet.data[0]
            when NMT_COMMAND_NODE_START
                @__canopen_state = NMT_STATE_OPERATIONAL
            when NMT_COMMAND_NODE_RESET
                can_in.write(canopen_make_heartbeat(node_id, NMT_STATE_INITIALIZING))
                @__canopen_state = NMT_STATE_PRE_OPERATIONAL
            when NMT_COMMAND_NODE_RESET_COMMUNICATION
                @__canopen_state = NMT_STATE_PRE_OPERATIONAL
            when NMT_COMMAND_NODE_STOP
                @__canopen_state = NMT_STATE_STOPPED
            when NMT_COMMAND_NODE_ENTER_PRE_OPERATIONAL
                @__canopen_state = NMT_STATE_PRE_OPERATIONAL
            end

            can_in.write(canopen_make_heartbeat(node_id))
        end

        def assert_canopen_sdo_upload(sample, node_id, object_id, object_sub_id)
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

        def canopen_make_sdo_upload_reply(node_id, object_id, object_sub_id, *bytes) # rubocop:disable Metrics/AbcSize
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

        def assert_canopen_sdo_download(sample, node_id, object_id, object_sub_id, *bytes) # rubocop:disable Metrics/AbcSize
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

        def canopen_make_sdo_download_reply(node_id, object_id, object_sub_id)
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

        def assert_canopen_nmt(sample, node_id, command)
            assert_equal 0, sample.can_id, "unexpected CAN ID"
            assert_equal command, sample.data[0], "unexpected command ID"
            assert_equal node_id, sample.data[1], "unexpected node ID"
        end

        # Generate a CANOpen heartbeat packet announcing the given node state
        #
        # @param [Integer] node_id
        # @param [String,Symbol,Integer] state the node state, either directly as its
        #   value in the heartbeat packet, or as its name (one of initializing, stopped,
        #   operational or pre_operational)
        def canopen_make_heartbeat(node_id, state = @__canopen_state)
            if state.respond_to?(:to_sym)
                state = CANOPEN_STATE_NAME_TO_ID.fetch(state.to_sym)
            end

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

        CANOPEN_STATE_NAME_TO_ID = {
            initializing: 0,
            stopped: 4,
            operational: 5,
            pre_operational: 0x7f
        }.freeze
    end
end
