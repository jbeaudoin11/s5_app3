#include <mbed.h>

#include "xbee_com.h"

Serial pc(USBTX, USBRX);
Serial xbee(p13, p14);

char ctrl_MAC[8] = {0};
bool ctrl_MAC_is_valid = false;


void SetupControllerMAC() {
    // DiscoverDevices(pc, ctrl_MAC);
    DiscoverDevices(xbee, pc, ctrl_MAC);

    pc.printf(
        "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n\r",
        ctrl_MAC[0],
        ctrl_MAC[1],
        ctrl_MAC[2],
        ctrl_MAC[3],
        ctrl_MAC[4],
        ctrl_MAC[5],
        ctrl_MAC[6],
        ctrl_MAC[7]
    );

}

int main() {
    pc.printf("\n\r\n\r== START ==\n\r");



    while(1) {
        SetupControllerMAC();

        wait(5);
        // Thread::wait(2000);
    }
}