#include "xbee_com.h"

// void WriteATcmd(Serial s, char[2] cmd, )

char _GetCheckSum(
    BasicPacket packet
) {
    char cksm = 0xFF;

    for(int i=0; i<packet.length; i++) {
        cksm -= packet.raw_packet[3 + i];
    }

    return cksm;
}

void _WriteRawPacket(
    Serial &s,
    const char raw_packet[],
    const int length
) {

    for(int i=0; i<length; i++) {
        // s.printf("%.2x", raw_packet[i]);
        s.putc(raw_packet[i]);
    }

}

int _ReadRawPacket(
    Serial &s,
    char dest[]
) {
    dest[0] = s.getc();

    if(dest[0] != START_DELEMITER) {
        // TODO error ??
        return 1;
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
    dest[3 + length] = s.getc();

    // TODO verify checksum

    return 0;
}

void DiscoverDevices(
    Serial &s,
    Serial &debug,
    char device_mac[8]
) {
    // Send AT ND 
    AtCommandPacket packet(
        0x01,
        (const char[]){'N', 'D'}
    );
    _WriteRawPacket(
        s,
        packet.raw_packet,
        packet.raw_packet_length
    );

    // Wait for the response
    char buffer[256];

    _ReadRawPacket(s, buffer);

    AtCommandResponsePacket res(buffer);

    debug.printf(
        "Status -- %.2x -- %d\n\r",
        res.at_command_status,
        res.length
    );

    // // Decode the packet
    // NodeIdentificationIndiactorPacket nii_packet(buffer);

    // debug.printf(
    //     "%.2x -- %.2x -- %d -- %.2x\n\r",
    //     buffer[1],
    //     buffer[2],
    //     ((nii_packet.length_high << 8) + nii_packet.length_low),
    //     buffer[3]
    // );

    // Extract data
    // memcpy(device_mac, nii_packet.src_mac_addr, 8);

}