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

int _ReadRawPacket(
    Serial &s,
    byte dest[]
) {
    Timer t;
    t.start();

    while(!s.readable()){
        if(t.read() > READ_TIMEOUT) {
            return 1; // TODO TIMEOUT
        }
    }

    dest[0] = s.getc();

    if(dest[0] != START_DELEMITER) {
        return 2; // TODO ignore ??
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
        return 3; // TODO cksm error
    }

    return 0;
}



vector<DiscoveredDevice> DiscoverDevices(
    Serial &s,
    Serial &debug
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

        // BasicPacket p(buffer);
        // for(int i=0; i<p.length; i++) {
        //     debug.printf("%.2x", p.content[i]);
        // }
        // debug.printf("\n\r");

        AtNDCommandResponsePacket res(buffer);

        DiscoveredDevice device;

        device.type = res.res_device_type;
        memcpy(device.mac_addr, res.res_mac_addr, 8);

        devices.push_back(device);
    }   

    return devices;    
}