#include "xbee_com.h"

// void WriteATcmd(Serial s, byte[2] cmd, )

byte _GetCheckSum(
    BasicPacket packet
) {
    byte cksm = 0xFF;

    for(int i=0; i<packet.length; i++) {
        cksm -= packet.raw_packet[3 + i];
    }

    return cksm;
}

void _WriteRawPacket(
    Serial &s,
    const byte raw_packet[],
    const int length
) {

    for(int i=0; i<length; i++) {
        // s.printf("%.2x", raw_packet[i]);
        s.putc(raw_packet[i]);
    }

}

ReadRawPacketStatus _ReadRawPacket(
    Serial &s,
    byte dest[]
) {
    Timer t;
    t.start();

    while(!s.readable()){
        if(t.read() > READ_TIMEOUT) {
            return TIMEOUT;
        }
    }

    dest[0] = s.getc();

    if(dest[0] != START_DELEMITER) {
        return IGNORE;
    }

    // Read length_high
    dest[1] = s.getc();

    // Read length_low
    dest[2] = s.getc();

    // Read data
    int length = (dest[1] << 8) + dest[2];
    for(int i=0; i<length; i++) {
        dest[3 + i] = s.getc();
    }

    // Read cksm
    byte cksm = s.getc();
    dest[3 + length] = cksm;

    if(_GetCheckSum(BasicPacket(dest)) != cksm) {
        return CKSM_ERROR; // TODO cksm error
    }

    return OK;
}

void _Read(
    Serial &xbee,
    const byte active_packet[MAX_PACKET_LENGTH],
    const bool coord_mac_addr_is_valid,
    ApplicationState &next_state
) {

    ReadRawPacketStatus status = _ReadRawPacket(xbee, active_packet);
    BasicPacket packet(active_packet);    


    if(coord_mac_addr_is_valid) {

        if(status != OK) {
            next_state = FUNCTION_LOOP;
        } else {

            switch(packet.frame_type) {
                case ZIGBEE_TRANSMIT_STATUS: {
                    next_state = ON_ZIGBEE_TRANSMIT_STATUS_PACKET;
                    break;
                }
                case ZIGBEE_RECEIVE: {
                    next_state = ON_ZIGBEE_RECEIVE_PACKET;
                    break;
                }
                default: {
                    next_state = FUNCTION_LOOP;
                }
            }
        }


    } else {

        if(status != OK) {
            next_state = WRITE_NETWORK_DISCOVERY_PACKET;
        } else {

            switch(packet.frame_type) {
                case AT_CMD_RES: {

                    AtCommandResponsePacket at_cmd_res_packet(active_packet);

                    if(
                        at_cmd_res_packet.at_command[0] == 'N' &&
                        at_cmd_res_packet.at_command[1] == 'D'
                    ) {
                        next_state = ON_AT_ND_COMMAND_RESPONSE_PACKET;                        
                    } else {
                        next_state = WRITE_NETWORK_DISCOVERY_PACKET;
                    }

                    break;
                }
                default: {
                    next_state = WRITE_NETWORK_DISCOVERY_PACKET;
                }
            }

        }

    }

}

DigitalIn _button(p20);
Accel _acc(0x1D);

void _ReadAccelerometer(
    char data[]
) {
    _acc.update();

    data[0] = acc.x();
    data[1] = acc.y();
    data[2] = acc.z();

}

void _ReadButton(
    char data[]
) {
    data[3] = (char) button;
}

const int _sensor_functions_length = 2;
typedef void (*VoidFunctionCharArray) (char[] data);
VoidFunctionCharArray _sensor_functions[] {
    _ReadAccelerometer,
    _ReadButton
};

void _ReadSensors(
    byte to_write_packet[MAX_PACKET_LENGTH]
) {
    char data[4];

    for(int i=0; i<_sensor_functions_length; i++) {
        _sensor_functions[i](data);
    }
    
    // TODO build ZIGBEE_TRANSMIT_REQUEST

}

void _OnZigbeeTransmitStatusPacket(
    byte active_packet[MAX_PACKET_LENGTH],
    ApplicationState &next_state
) {

    

}

void _OnZigbeeReceivePacket(
    byte active_packet[MAX_PACKET_LENGTH],
    ApplicationState &next_state
) {

}

void _OnATNDCommandResponsePacket(
    byte active_packet[MAX_PACKET_LENGTH],
    byte coord_mac_addr[8],
    ApplicationState &next_state
) {

}

vector<DiscoveredDevice> _DiscoverDevices(
    Serial &s
) {
    // Send AT ND 
    AtCommandPacket packet(
        0x01,
        (const byte[]){'N', 'D'}
    );
    _WriteRawPacket(
        s,
        packet.raw_packet,
        packet.raw_packet_length
    );

    vector<DiscoveredDevice> devices;

    // Response buffer
    byte buffer[MAX_PACKET_LENGTH];

    // Wait for responses
    while(true) {
        if(_ReadRawPacket(s, buffer)) {
            break;
        }

        AtNDCommandResponsePacket res(buffer);

        DiscoveredDevice device;

        device.type = res.res_device_type;
        memcpy(device.mac_addr, res.res_mac_addr, 8);

        devices.push_back(device);
    }   

    return devices;    
}

void _DiscoverCoordonator(
    Serial &s
) {

}

void StateMachine(
    Serial &xbee
) {

    ApplicationState current_state = WRITE_NETWORK_DISCOVERY_PACKET;
    ApplicationState next_state;

    byte coord_mac_addr[8] = {0};
    bool coord_mac_addr_is_valid = false;

    byte active_packet[MAX_PACKET_LENGTH];
    byte to_write_packet[MAX_PACKET_LENGTH];

    queue<BasicPacket> write_queue;

    Timer led_4_timer;
    led_4_timer.start();

    while(true) {
        current_state = next_state;

        if((int) led_4 && led_4_timer.read() >= 1) {
            led_4 = 0;
        }

        switch(current_state) {
            case FUNCTION_LOOP: {
                switch(function_index++) {
                    case 0: {
                        current_state = READ;
                        break;
                    }
                    case 1: {
                        current_state = WRITE_POP;
                        break;
                    }
                    default: {
                        current_state = READ_SENSORS;
                        break;
                    }
                }
                break;
            }
            case READ: {
                _Read(
                    xbee,
                    active_packet,
                    next_state
                );
                break;
            }
            case READ_SENSORS: {
                _ReadSensors(to_write_packet);
                next_state = WRITE_ZIGBEE_TRANSMIT_REQUEST;
                break;
            }
            case ON_ZIGBEE_TRANSMIT_STATUS_PACKET: {
                _OnZigbeeTransmitStatusPacket(active_packet, next_state); 
                break;
            }
            case ON_ZIGBEE_TRANSMIT_STATUS_PACKET_ERROR: {
                led_4 = 1;
                led_4_timer.reset();
                next_state = FUNCTION_LOOP;
                break;
            }
            case INVERT_LED_1: {
                led_1 = !led_1;
                next_state = FUNCTION_LOOP;
                break;
            }
            case ON_ZIGBEE_RECEIVE_PACKET: {
                _OnZigbeeReceivePacket(active_packet, next_state);                 
                break;
            }
            case ON_AT_ND_COMMAND_RESPONSE_PACKET: {
                _OnATNDCommandResponsePacket(
                    active_packet,
                    coord_mac_addr,
                    coord_mac_addr_is_valid,
                    next_state
                );                 
                break;
            }
            case ON_AT_ND_COMMAND_RESPONSE_PACKET_ERROR: {
                led_4 = 1;
                led_4_timer.reset();
                next_state = FUNCTION_LOOP;
                break;
            }
            case WRITE_POP: {
                _WriteRawPacket(Serial xbee, write_queue.pop_front().raw_packet);

                if(coord_mac_addr_is_valid) {
                    next_state = FUNCTION_LOOP;
                } else {
                    next_state = READ;
                }

                break;
            }
            case WRITE_NETWORK_DISCOVERY_PACKET: {
                write_queue.push_back(packet_to_write);
                next_state = FUNCTION_LOOP;
                break;
            }
            case WRITE_ZIGBEE_TRANSMIT_REQUEST: {
                write_queue.push_back(packet_to_write);
                next_state = FUNCTION_LOOP;
                break;
            }
        }

    }

}