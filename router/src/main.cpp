#include <mbed.h>
#include <vector>

#include "xbee_com.h"
#include "Accel.h"


Serial pc(USBTX, USBRX);
Serial xbee(p13, p14);

DigitalOut led_1(LED1);
DigitalOut led_2(LED2);
DigitalOut led_3(LED3);
DigitalOut led_4(LED4);

DigitalIn button(p20);

byte coord_mac_addr[8] = {0};
bool coord_mac_addr_is_valid = false;
Accel acc(0x1D);


void SetupCoordinatorMAC() {
    printf("Discovering...\n\r");

    vector<DiscoveredDevice> devices = DiscoverDevices(xbee, pc);

    // printf("%d discovered devices\n\r", devices.size());

    int nb_devices = devices.size();
    for(int i=0; i<nb_devices; i++) {

        if(devices[i].type == COORDINATOR) {
            memcpy(coord_mac_addr, devices[i].mac_addr, 8);
            coord_mac_addr_is_valid = true;

            pc.printf(
                "Coordinator found -- %.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n\r\n\r",
                devices[i].mac_addr[1],
                devices[i].mac_addr[2],
                devices[i].mac_addr[3],
                devices[i].mac_addr[4],
                devices[i].mac_addr[5],
                devices[i].mac_addr[6],
                devices[i].mac_addr[7],
                devices[i].mac_addr[8]
            );
            return;
        }

    }

    pc.printf("Coordinator not found\n\r");
}

int main() {
    pc.printf("\n\r\n\r== START ==\n\r");
    
    // Find the Coordinator mac address
    do {
        SetupCoordinatorMAC();
    } while(!coord_mac_addr_is_valid);

    while(1) {
        acc.update();

        pc.printf(
            "%d -- %d -- %d -- %d\n\r",
            acc.x(),
            acc.y(),
            acc.z(),
            (int) button
        );

        wait(1);
        // Thread::wait(2000);
    }
}