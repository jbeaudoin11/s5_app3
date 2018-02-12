#ifndef XBEE_COM_H
#define XBEE_COM_H

#include <vector>
#include <queue>

#include "mbed.h"
#include "Accel.h"

#define START_DELEMITER 0x7E
// #define MAX_PACKET_LENGTH 65539
#define MAX_PACKET_LENGTH 512
#define READ_TIMEOUT 5

#define byte unsigned char

enum ApplicationState {
    FUNCTION_LOOP,
    READ,
    READ_SENSORS,
    ON_ZIGBEE_TRANSMIT_STATUS_PACKET,
    ON_ZIGBEE_TRANSMIT_STATUS_PACKET_ERROR,
    INVERT_LED_1,    
    ON_ZIGBEE_RECEIVE_PACKET,
    ON_AT_ND_COMMAND_RESPONSE_PACKET,
    ON_AT_ND_COMMAND_RESPONSE_PACKET_ERROR,
    WRITE_POP,
    WRITE_NETWORK_DISCOVERY_PACKET,
    WRITE_ZIGBEE_TRANSMIT_REQUEST,
};

enum ApiFrameType {
    AT_CMD = 0x08,
    AT_CMD_RES = 0x88,
    ZIGBEE_TRANSMIT_REQUEST = 0x10,
    ZIGBEE_TRANSMIT_STATUS = 0x8B,
    ZIGBEE_RECEIVE = 0x90
};

enum XbeeDeviceType {
    COORDINATOR = 0x00,
    ROUTER = 0x01,
    END_DEVICE = 0x02
};

struct BasicPacket {
    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte content[MAX_PACKET_LENGTH];

    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    BasicPacket(const byte _raw_packet[]) {

        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;
        
        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);

        memcpy(content, _raw_packet + 3, length);

        raw_packet_length = 3 + length + 1;

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);

    }
};
typedef struct BasicPacket BasicPacket;

byte _GetCheckSum(
    BasicPacket packet
);

// Ref page 117
struct NodeIdentificationIndiactorPacket {
    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte src_mac_addr[8];
    byte src_network_addr[2];
    byte res_options;
    byte res_src_network_addr[2];
    byte res_mac_addr[8];
    byte res_ni_string[2];
    byte res_parent_network_addr[2];
    XbeeDeviceType res_device_type;
    byte res_src_event;
    byte digi_profile_id[2];
    byte manifacturer_id[2];

    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length = 36;

    NodeIdentificationIndiactorPacket(const byte _raw_packet[]) {
        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;

        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);

        memcpy(src_mac_addr, _raw_packet + 4, 8);

        src_network_addr[0] = _raw_packet[12];
        src_network_addr[1] = _raw_packet[13];

        res_options = _raw_packet[14];

        res_src_network_addr[0] = _raw_packet[15];
        res_src_network_addr[1] = _raw_packet[16];

        memcpy(res_mac_addr, _raw_packet + 17, 8);

        res_ni_string[0] = _raw_packet[25];
        res_ni_string[1] = _raw_packet[26];

        res_parent_network_addr[0] = _raw_packet[27];
        res_parent_network_addr[1] = _raw_packet[28];

        res_device_type = static_cast<XbeeDeviceType> (_raw_packet[29]);
        res_src_event = _raw_packet[30];
        
        digi_profile_id[0] = _raw_packet[31];
        digi_profile_id[1] = _raw_packet[32];
        
        manifacturer_id[0] = _raw_packet[33];
        manifacturer_id[1] = _raw_packet[34];

        cksm = _raw_packet[35];

        // Fixed size
        raw_packet_length = 36;
        memcpy(raw_packet, _raw_packet, raw_packet_length);

    }

};
typedef struct NodeIdentificationIndiactorPacket NodeIdentificationIndiactorPacket;

// Ref page 102-103
struct AtCommandPacket {
    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte frame_id;
    byte at_command[2];
    byte at_command_value[256]; // optional

    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length = 8;

    void _BuildFromRawPacket(const byte _raw_packet[]) {
        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;

        // Frame-specific Data
        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);
        frame_id = _raw_packet[4];
        at_command[0] = _raw_packet[5];
        at_command[1] = _raw_packet[6];

        raw_packet_length = 3 + length + 1;

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);
    }

    AtCommandPacket(const byte _raw_packet[]) {
        _BuildFromRawPacket(_raw_packet);
    }

    AtCommandPacket(
        const byte frame_id,
        const byte at_command[2]
    ) {

        byte _raw_packet[MAX_PACKET_LENGTH];

        _raw_packet[0] = START_DELEMITER;

        _raw_packet[1] = 0;
        _raw_packet[2] = 0x04;
        
        _raw_packet[3] = AT_CMD;
        _raw_packet[4] = frame_id;

        _raw_packet[5] = at_command[0];
        _raw_packet[6] = at_command[1];

        _raw_packet[7] = _GetCheckSum(BasicPacket(_raw_packet));

        _BuildFromRawPacket(_raw_packet);
    }

    AtCommandPacket(
        const byte frame_id,
        const byte at_command[2],
        const byte *at_command_data,
        const unsigned short at_command_data_length
    ) {
        byte _raw_packet[MAX_PACKET_LENGTH];

        _raw_packet[0] = START_DELEMITER;

        unsigned short _length = 4 + at_command_data_length;
        _raw_packet[1] = (byte)(_length >> 8);
        _raw_packet[2] = (byte) _length;
        
        _raw_packet[3] = AT_CMD;
        _raw_packet[4] = frame_id;

        _raw_packet[5] = at_command[0];
        _raw_packet[6] = at_command[1];

        if(at_command_data_length > 0) {
            memcpy(_raw_packet, at_command_data, at_command_data_length);
        }

        _raw_packet[7 + at_command_data_length] = _GetCheckSum(BasicPacket(_raw_packet));

        _BuildFromRawPacket(_raw_packet);        
    }
};
typedef struct AtCommandPacket AtCommandPacket;

// Ref page 110 & 117
struct AtCommandResponsePacket {
    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte frame_id;
    byte at_command[2];
    byte at_command_status;
    byte at_command_data[256]; // optional

    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    AtCommandResponsePacket(const byte _raw_packet[]) {
        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;

        // Frame-specific Data
        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);
        frame_id = _raw_packet[4];
        at_command[0] = _raw_packet[5];
        at_command[1] = _raw_packet[6];

        at_command_status = _raw_packet[7];

        raw_packet_length = 3 + length + 1;

        if(raw_packet_length > 9) {
            unsigned short at_command_data_length = length - 5;
            memcpy(at_command_data, _raw_packet + 8, at_command_data_length);
        }

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);
    }

};
typedef struct AtCommandResponsePacket AtCommandResponsePacket;

// Ref page 110
struct AtNDCommandResponsePacket {
    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte frame_id;
    byte at_command[2];
    byte at_command_status;
    byte at_command_data[256]; // optional

    byte res_src_network_addr[2];
    byte res_mac_addr[8];
    byte res_ni_string[2];
    byte res_parent_network_addr[2];
    XbeeDeviceType res_device_type;
    byte res_src_event;
    byte digi_profile_id[2];
    byte manifacturer_id[2];


    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    AtNDCommandResponsePacket(const byte _raw_packet[]) {
        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;

        // Frame-specific Data
        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);
        frame_id = _raw_packet[4];
        at_command[0] = _raw_packet[5];
        at_command[1] = _raw_packet[6];

        at_command_status = _raw_packet[7];

        raw_packet_length = 3 + length + 1;

        if(raw_packet_length > 9) {
            unsigned short at_command_data_length = length - 5;
            memcpy(at_command_data, _raw_packet + 8, at_command_data_length);

            res_src_network_addr[0] = _raw_packet[8];
            res_src_network_addr[1] = _raw_packet[9];

            memcpy(res_mac_addr, _raw_packet + 10, 8);

            res_ni_string[0] = _raw_packet[18];
            res_ni_string[1] = _raw_packet[19];

            res_parent_network_addr[0] = _raw_packet[20];
            res_parent_network_addr[1] = _raw_packet[21];

            res_device_type = static_cast<XbeeDeviceType> (_raw_packet[22]);
            res_src_event = _raw_packet[23];
            
            digi_profile_id[0] = _raw_packet[24];
            digi_profile_id[1] = _raw_packet[25];
            
            manifacturer_id[0] = _raw_packet[26];
            manifacturer_id[1] = _raw_packet[27];
        }

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);
    }

};
typedef struct AtNDCommandResponsePacket AtNDCommandResponsePacket;
//Page 104
struct ZigbeeTransmitRequestPacket {

    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte frame_id;
    byte dest_mac_addr[8];
    byte dest_network_addr[2];
    byte broadcast_radius;
    byte options; 
    byte rf_data[8];

    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    _BuildFromRawPacket(const byte _raw_packet[]){
        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;

        raw_packet_length = 3 + length + 1;

        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);
        frame_id = _raw_packet[4];
        memcpy(dest_mac_addr , _raw_packet + 5, 8);
        memcpy(dest_network_addr , _raw_packet + 13, 2);
        
        broadcast_radius = _raw_packet[15];
        options = _raw_packet[16];
        memcpy(rf_data, _raw_packet + 17, 8);

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);
    }

    ZigbeeTransmitRequestPacket(const byte _raw_packet[]) {
        _BuildFromRawPacket(_raw_packet);
    }

    ZigbeeTransmitRequestPacket(
        const byte frame_id,
        const byte dest_mac_addr[8],
        const byte dest_network_addr[2],
        const byte broadcast_radius,
        const byte options,
        const byte rf_data[8]
    ) {
        byte _raw_packet[MAX_PACKET_LENGTH];

        _raw_packet[0] = START_DELEMITER;

        _raw_packet[1] = 0;
        _raw_packet[2] = 0x16;
        
        _raw_packet[3] = ZIGBEE_TRANSMIT_REQUEST;
        _raw_packet[4] = frame_id;

        memcpy(_raw_packet + 5, dest_mac_addr, 8);
        memcpy(_raw_packet + 13, dest_network_addr, 2);
        
        _raw_packet[15] = broadcast_radius;

        _raw_packet[16] = options;
        memcpy(_raw_packet + 17, rf_data, 8);

        _raw_packet[25] = _GetCheckSum(BasicPacket(_raw_packet));

        _BuildFromRawPacket(_raw_packet);
    }
};
typedef struct ZigbeeTransmitRequestPacket ZigbeeTransmitRequestPacket;
//Page 111
struct ZigbeeTransmitStatusPacket {

    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte frame_id;
    byte dest_network_addr[2];
    byte transmit_retry_count;
    byte delivery_status; 
    byte discovery_status;

    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    ZigbeeTransmitRequestPacket(const byte _raw_packet[]){
        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;

        raw_packet_length = 3 + length + 1;

        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);
        frame_id = _raw_packet[4];
        memcpy(dest_network_addr , _raw_packet + 5, 2);
        
        transmit_retry_count = _raw_packet[7];
        delivery_status = _raw_packet[8];
        discovery_status = _raw_packet[9];

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);
    }

};
typedef struct ZigbeeTransmitStatusPacket ZigbeeTransmitStatusPacket;
//Page 112
struct ZigbeeReceivePacket {

    // Start Delemiter
    byte start_delemiter = START_DELEMITER;

    // Length
    byte length_high;
    byte length_low;
    unsigned short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    byte dest_mac_addr[8];
    byte dest_network_addr[2];
    byte options;
    byte data[6]; 

    // Checksum
    byte cksm;

    // Metadata
    byte raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    ZigbeeTransmitRequestPacket(const byte _raw_packet[]){
        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;

        raw_packet_length = 3 + length + 1;

        frame_type = static_cast<ApiFrameType> (_raw_packet[3]);
        memcpy(dest_mac_addr , _raw_packet + 4, 8);
        memcpy(dest_network_addr , _raw_packet + 12, 2);
        
        options = _raw_packet[14];
        memcpy(data, _raw_packet + 15, 6);

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);
    }
};
typedef struct ZigbeeReceivePacket ZigbeeReceivePacket;



byte _GetCheckSum(
    BasicPacket &packet
);

void _WriteRawPacket(
    Serial &s,
    const byte raw_packet[],
    const int length
);

enum ReadRawPacketStatus {
    OK,
    TIMEOUT,
    IGNORE,
    CKSM_ERROR
};
ReadRawPacketStatus _ReadRawPacket(
    Serial &s,
    byte dest[]
);

void _Read(
    Serial &xbee,
    const byte active_packet[MAX_PACKET_LENGTH],
    const bool coord_mac_addr_is_valid,
    ApplicationState &next_state
);

void _ReadAccelerometer(
    char data[]
);
void _ReadButton(
    char data[]
);
void _ReadSensors(
    byte to_write_packet[MAX_PACKET_LENGTH]
);

void _OnZigbeeTransmitStatusPacket(
    byte active_packet[MAX_PACKET_LENGTH],
    ApplicationState &next_state
);

void _OnZigbeeReceivePacket(
    byte active_packet[MAX_PACKET_LENGTH],
    ApplicationState &next_state
);

void _OnATNDCommandResponsePacket(
    byte active_packet[MAX_PACKET_LENGTH],
    byte coord_mac_addr[8],
    ApplicationState &next_state
);

struct DiscoveredDevice {
    XbeeDeviceType type;    
    byte mac_addr[8];
};
typedef struct DiscoveredDevice DiscoveredDevice;

vector<DiscoveredDevice> _DiscoverDevices(
    Serial &s
);

void _DiscoverCoordonator(
    Serial &s
);

void StateMachine(
    Serial &xbee
);

#endif 
