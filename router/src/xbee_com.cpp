// beaj2031 - merj2607
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

void _WriteBasicPacket(
    Serial &xbee,
    const BasicPacket &packet_to_write
) {
    DebugLog(DEBUG_WriteBasicPacket, "[DEBUG] ====== START _WriteBasicPacket ======\n\r");      

    DebugLog(DEBUG_WriteBasicPacket, "[DEBUG] Writing %d bytes\n\r", packet_to_write.raw_packet_length);      
    for(int i=0; i<packet_to_write.raw_packet_length; i++) {
        xbee.putc(packet_to_write.raw_packet[i]);
        DebugLog(DEBUG_WriteBasicPacket, "%.2x ", packet_to_write.raw_packet[i]);      
    }
    DebugLog(DEBUG_WriteBasicPacket, "\n\r");      

    DebugLog(DEBUG_WriteBasicPacket, "[DEBUG] ====== END _WriteBasicPacket ======\n\r");      
}

ReadRawPacketStatus _ReadRawPacket(
    Serial &xbee,
    byte dest[MAX_PACKET_LENGTH]
) {
    Timer t;
    t.start();

    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] ====== START _ReadRawPacket ======\n\r");      
    
    while(!xbee.readable()){
        if(t.read() > READ_TIMEOUT) {
            DebugLog(DEBUG_ReadRawPacket, "[DEBUG] TIMEOUT %0.2f sec\n\r", t.read());      
            return TIMEOUT;
        }
    }

    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] First byte\n\r");    
    dest[0] = xbee.getc();
    if(dest[0] != START_DELEMITER) {
        DebugLog(DEBUG_ReadRawPacket, "[DEBUG] First byte is not 0xFE (Ignore)\n\r");       
        return IGNORE;
    }

    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] Read length_high \n\r");    
    // Read length_high
    dest[1] = xbee.getc();

    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] Read length_low\n\r");       
    // Read length_low
    dest[2] = xbee.getc();

    // Read data
    unsigned short length = (dest[1] << 8) + dest[2];
    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] Read data, %d bytes\n\r", length);     
    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] ");     
    for(int i=0; i<length; i++) {
        dest[3 + i] = xbee.getc();
        DebugLog(DEBUG_ReadRawPacket, "%.2x ", dest[3 + i]);     
    }
    DebugLog(DEBUG_ReadRawPacket, "\n\r");     
 
    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] Read cksm\n\r");      
    // Read cksm
    byte cksm = xbee.getc();
    dest[3 + length] = cksm;

    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] Verify cksm\n\r");      
    if(_GetCheckSum(BasicPacket(dest)) != cksm) {
    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] CKSM_ERROR\n\r");      
        printf("[DEBUG] CKSM_ERROR\n\r");            
        return CKSM_ERROR; // TODO cksm error
    }

    DebugLog(DEBUG_ReadRawPacket, "[DEBUG] ====== End _ReadRawPacket ======\n\r");  

    return OK;
}

byte _read_retry = 0;
void _Read(
    Serial &xbee,
    byte active_packet[MAX_PACKET_LENGTH],
    const bool coord_mac_addr_is_valid,
    ApplicationState &next_state
) {
    DebugLog(DEBUG_Read, "[DEBUG] ====== START _Read ======\n\r");     

    ReadRawPacketStatus status = _ReadRawPacket(xbee, active_packet);

    BasicPacket packet(active_packet);

    DebugLog(DEBUG_Read, "[DEBUG] frame_type %.2x\n\r", packet.frame_type);                                      

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

        if(status == TIMEOUT && _read_retry < MAX_READ_RETRY) {
            _read_retry++;
            next_state = READ;
        } else if(status != OK) {
            _read_retry = 0;                        
            next_state = WRITE_NETWORK_DISCOVERY_PACKET;
        } else {

            switch(packet.frame_type) {
                case AT_CMD_RES: {

                    AtCommandResponsePacket at_cmd_res_packet(active_packet);

                    if(
                        at_cmd_res_packet.at_command[0] == 'N' &&
                        at_cmd_res_packet.at_command[1] == 'D'
                    ) {
                        _read_retry = 0;
                        next_state = ON_AT_ND_COMMAND_RESPONSE_PACKET;                        
                    } else if(_read_retry >= MAX_READ_RETRY) {
                        _read_retry = 0;                        
                        next_state = WRITE_NETWORK_DISCOVERY_PACKET;
                    } else {
                        _read_retry++;
                        next_state = READ;
                    }

                    break;
                }
                default: {
                    if(_read_retry >= MAX_READ_RETRY) {
                        _read_retry = 0;
                        next_state = WRITE_NETWORK_DISCOVERY_PACKET;
                    } else {
                        _read_retry++;
                        next_state = READ;
                    }
                }
            }

        }

    }

    DebugLog(DEBUG_Read, "[DEBUG] ====== END _Read ======\n\r");                                  
}

DigitalIn _button(p20);
Accel _acc(0x1D);

void _ReadAccelerometer(
    byte data[]
) {
    _acc.update();

    data[1] = (byte) _acc.x();
    data[2] = (byte) _acc.y();
    data[3] = (byte) _acc.z();

}

void _ReadButton(
    byte data[]
) {
    data[4] = (char) _button;
}

const int _sensor_functions_length = 2;
typedef void (*VoidFunctionByteArray) (byte[]);
VoidFunctionByteArray _sensor_functions[] {
    _ReadAccelerometer,
    _ReadButton
};

void _ReadSensors(
    byte data[]
) {
    for(int i=0; i<_sensor_functions_length; i++) {
        _sensor_functions[i](data);
    }
}

void _OnZigbeeTransmitStatusPacket(
    byte active_packet[MAX_PACKET_LENGTH],
    ApplicationState &next_state
) {

    ZigbeeTransmitStatusPacket packet(active_packet);

    if(packet.delivery_status == 0x00) {
        next_state = FUNCTION_LOOP;
    } else {
        next_state = ON_ZIGBEE_TRANSMIT_STATUS_PACKET_ERROR;
    }

}

void _OnZigbeeReceivePacket(
    byte active_packet[MAX_PACKET_LENGTH],
    ApplicationState &next_state
) {

    ZigbeeReceivePacket packet(active_packet);

    if(packet.app_command == 0x00) {
        next_state = INVERT_LED_1;
    } else {
        next_state = FUNCTION_LOOP;
    }

}

void _OnATNDCommandResponsePacket(
    byte active_packet[MAX_PACKET_LENGTH],
    byte coord_mac_addr[8],
    bool &coord_mac_addr_is_valid,
    ApplicationState &next_state
) {
    AtNDCommandResponsePacket packet(active_packet);

    if(
        packet.at_command_status == 0x00 &&
        packet.res_device_type == COORDINATOR
    ) {

        memcpy(coord_mac_addr, packet.res_mac_addr, 8);
        coord_mac_addr_is_valid = true;
        next_state = FUNCTION_LOOP;

    } else {

        coord_mac_addr_is_valid = false;
        next_state = ON_AT_ND_COMMAND_RESPONSE_PACKET_ERROR;

    }

}

Serial _pc(USBTX, USBRX, NULL, 115200);
void DebugLog(
    bool active,
    char *msg,
    ...
) {
    if(active) {
        va_list args;
        va_start(args, msg);
        _pc.vprintf(msg, args);
        va_end(args);
    }
}

void _PrintState(
    const ApplicationState &state
) {

    switch(state) {
        case FUNCTION_LOOP: {
            printf("FUNCTION_LOOP\n\r");
            break;
        }
        case READ: {
            printf("READ\n\r");
            break;
        }
        case READ_SENSORS: {
            printf("READ_SENSORS\n\r");
            break;
        }
        case ON_ZIGBEE_TRANSMIT_STATUS_PACKET: {
            printf("ON_ZIGBEE_TRANSMIT_STATUS_PACKET\n\r");
            break;
        }
        case ON_ZIGBEE_TRANSMIT_STATUS_PACKET_ERROR: {
            printf("ON_ZIGBEE_TRANSMIT_STATUS_PACKET_ERROR\n\r");
            break;
        }
        case INVERT_LED_1: {
            printf("INVERT_LED_1\n\r");
            break;
        }
        case ON_ZIGBEE_RECEIVE_PACKET: {
            printf("ON_ZIGBEE_RECEIVE_PACKET\n\r");
            break;
        }
        case ON_AT_ND_COMMAND_RESPONSE_PACKET: {
            printf("ON_AT_ND_COMMAND_RESPONSE_PACKET\n\r");
            break;
        }
        case ON_AT_ND_COMMAND_RESPONSE_PACKET_ERROR: {
            printf("ON_AT_ND_COMMAND_RESPONSE_PACKET_ERROR\n\r");
            break;
        }
        case WRITE_POP: {
            printf("WRITE_POP\n\r");
            break;
        }
        case WRITE_NETWORK_DISCOVERY_PACKET: {
            printf("WRITE_NETWORK_DISCOVERY_PACKET\n\r");
            break;
        }
        case WRITE_APP_COMMAND_0x01_PACKET: {
            printf("WRITE_APP_COMMAND_0x01_PACKET\n\r");
            break;
        }
        default: {
            printf("UNKNOW %d\n\r", (int) state);
            break;
        }
        
    }

}


DigitalOut led_1(LED1); //  app_command 0x00
DigitalOut led_2(LED2);
DigitalOut led_3(LED3);
DigitalOut led_4(LED4); // Error led 4

void StateMachine(
    Serial &xbee
) {

    ApplicationState current_state;
    ApplicationState next_state = WRITE_NETWORK_DISCOVERY_PACKET;

    byte coord_mac_addr[8] = {0};
    bool coord_mac_addr_is_valid = false;
    int function_index = 0;

    byte active_packet[MAX_PACKET_LENGTH];
    byte sensors_data[8] = {0x01}; /// 0x01 == sensors data app command

    queue<BasicPacket> write_queue;

    Timer led_4_timer;
    led_4_timer.start();

    bool sensors_new_data = false;
    Timer sensors_timer;
    sensors_timer.start();

    while(true) {
        current_state = next_state;

        // _PrintState(current_state);

        // Reset error led_4 after 1 sec (might be longer if a function take more time to process like Read)
        if((int) led_4 && led_4_timer.read() >= 1) {
            led_4 = 0;
        }

        switch(current_state) {
            case FUNCTION_LOOP: {
                switch(function_index) {
                    case 0: {
                        next_state = READ;
                        break;
                    }
                    case 1: {
                        next_state = WRITE_POP;
                        break;
                    }
                    case 2: {
                        next_state = READ_SENSORS;
                        break;
                    }
                }

                // printf("function_index %d\n\n", function_index);
                function_index = (function_index + 1) % 3;

                // if(coord_mac_addr_is_valid && function_index == 0) {
                //     wait(10);
                // }

                break;
            }
            case READ: {
                _Read(
                    xbee,
                    active_packet,
                    coord_mac_addr_is_valid,
                    next_state
                );
                break;
            }
            case READ_SENSORS: {
                if(sensors_timer.read() > 1) {
                    _ReadSensors(sensors_data);
                    sensors_new_data = true;
                }
                next_state = WRITE_APP_COMMAND_0x01_PACKET;
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
                next_state = READ;
                break;
            }
            case WRITE_POP: {

                if(write_queue.size() > 0) {
                    _WriteBasicPacket(xbee, write_queue.front());
                    write_queue.pop();
                }

                if(coord_mac_addr_is_valid) {
                    next_state = FUNCTION_LOOP;
                } else {
                    next_state = READ;
                }

                break;
            }
            case WRITE_NETWORK_DISCOVERY_PACKET: {
                // Send AT ND 
                AtCommandPacket packet_to_write(
                    0x01,
                    (const byte[]){'N', 'D'}
                );

                write_queue.push(BasicPacket(packet_to_write.raw_packet));
                next_state = WRITE_POP;
                break;
            }
            case WRITE_APP_COMMAND_0x01_PACKET: {

                if(sensors_new_data) {
                    ZigbeeTransmitRequestPacket packet_to_write(
                        0x01,
                        coord_mac_addr,
                        (const byte[]) {0xFF, 0xFE},
                        0x00,
                        0x00,
                        sensors_data
                    );

                    write_queue.push(BasicPacket(packet_to_write.raw_packet));

                    sensors_new_data = false;
                    sensors_timer.reset();
                }
                
                next_state = FUNCTION_LOOP;
                break;
            }
        }

    }

}