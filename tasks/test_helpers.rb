# frozen_string_literalÇ true

module CANOpen
    module TestHelpers
        class SDONotSet < RuntimeError; end

        def setup
            @__canopen_objects = {}
            super
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
                unless packet.can_id == 0x600 + node_id
                    puts "Ignored packet"
                    pp packet
                    next
                end

                if packet.data[0] == 0x40
                    reply = canopen_handle_sdo_upload(node_id, packet)
                    can_in.write(reply)
                elsif packet.data[0] & 0x20 == 0x20
                    reply = canopen_handle_sdo_download(node_id, packet)
                    can_in.write(reply)
                end
            end
        end

        def canopen_handle_sdo_upload(node_id, packet)
            id, sub_id = canopen_sdo_object(packet)
            bytes = canopen_get_bytes(id, sub_id)
            canopen_make_sdo_upload_reply(node_id, id, sub_id, *bytes)
        end

        def canopen_sdo_object(packet)
            object_id_lsb = packet.data[1]
            object_id_msb = packet.data[2]
            object_sub_id = packet.data[3]
            [object_id_lsb + (object_id_msb << 8), object_sub_id]
        end

        def canopen_handle_sdo_download(node_id, packet)
            id, sub_id = canopen_sdo_object(packet)
            size = 4 - ((packet.data[0] >> 2) & 0b11)
            canopen_set_bytes(id, sub_id, packet.data[4, size].to_a.reverse)
            canopen_make_sdo_download_reply(node_id, id, sub_id)
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
end
