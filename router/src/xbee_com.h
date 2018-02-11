#ifndef XBEE_COM_H
#define XBEE_COM_H

#include "mbed.h"

#define START_DELEMITER 0x7E
// #define MAX_PACKET_LENGTH 65539
#define MAX_PACKET_LENGTH 512

enum ApiFrameType {
    AT_CMD = 0x08,
    AT_CMD_RES = 0x88
};

enum XbeeDeviceType {
    COORDINATOR = 0x00,
    ROUTER = 0x01,
    END_DEVICE = 0x02
};

struct BasicPacket {
    // Start Delemiter
    char start_delemiter = START_DELEMITER;

    // Length
    char length_high;
    char length_low;
    short length;

    // Checksum
    char cksm;

    // Metadata
    char raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    BasicPacket(const char _raw_packet[]) {

        start_delemiter = _raw_packet[0];

        length_high = _raw_packet[1];
        length_low = _raw_packet[2];
        length = (length_high << 8) + length_low;
        
        raw_packet_length = 3 + length + 1;

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);

    }
};
typedef struct BasicPacket BasicPacket;

char _GetCheckSum(
    BasicPacket packet
);

// Ref page 117
struct NodeIdentificationIndiactorPacket {
    // Start Delemiter
    char start_delemiter = START_DELEMITER;

    // Length
    char length_high;
    char length_low;
    short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    char src_mac_addr[8];
    char src_network_addr[2];
    char res_options;
    char res_src_network_addr[2];
    char res_mac_addr[8];
    char res_ni_string[2];
    char res_parent_network_addr[2];
    XbeeDeviceType res_device_type;
    char res_src_event;
    char digi_profile_id[2];
    char manifacturer_id[2];

    // Checksum
    char cksm;

    // Metadata
    char raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length = 36;

    NodeIdentificationIndiactorPacket(const char _raw_packet[]) {
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
    char start_delemiter = START_DELEMITER;

    // Length
    char length_high;
    char length_low;
    short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    char frame_id;
    char at_command[2];
    char at_command_value[256]; // optional

    // Checksum
    char cksm;

    // Metadata
    char raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length = 8;

    void _BuildFromRawPacket(const char _raw_packet[]) {
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

    AtCommandPacket(const char _raw_packet[]) {
        _BuildFromRawPacket(_raw_packet);
    }

    AtCommandPacket(
        const char frame_id,
        const char at_command[2]
    ) {

        char _raw_packet[MAX_PACKET_LENGTH];

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
        const char frame_id,
        const char at_command[2],
        const char *at_command_data,
        const short at_command_data_length
    ) {
        char _raw_packet[MAX_PACKET_LENGTH];

        _raw_packet[0] = START_DELEMITER;

        short _length = 4 + at_command_data_length;
        _raw_packet[1] = (char)(_length >> 8);
        _raw_packet[2] = (char) _length;
        
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

// Ref page 110
struct AtCommandResponsePacket {
    // Start Delemiter
    char start_delemiter = START_DELEMITER;

    // Length
    char length_high;
    char length_low;
    short length;

    // Frame-specific Data
    ApiFrameType frame_type;
    char frame_id;
    char at_command[2];
    char at_command_status;
    char at_command_data[256]; // optional

    // Checksum
    char cksm;

    // Metadata
    char raw_packet[MAX_PACKET_LENGTH];
    int raw_packet_length;

    AtCommandResponsePacket(const char _raw_packet[]) {
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
            short at_command_data_length = length - 5;
            memcpy(at_command_data, _raw_packet + 8, at_command_data_length);
        }

        cksm = _raw_packet[raw_packet_length - 1];

        memcpy(raw_packet, _raw_packet, raw_packet_length);
    }

};
typedef struct AtCommandResponsePacket AtCommandResponsePacket;

char _GetCheckSum(
    BasicPacket &packet
);

void _WriteString(
    Serial &s,
    const char raw_packet[],
    const int length
);

int _ReadRawPacket(
    Serial &s,
    char dest[]
);

void DiscoverDevices(
    Serial &s,
    Serial &debug,
    char device_mac[8]
);

#endif 
