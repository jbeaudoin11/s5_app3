#include "mbed.h"
 
Serial pc(USBTX, USBRX);
Serial uart(p13, p14);

DigitalOut w(p8);


int main() {

    w = 0;
    wait_ms(400);
    w = 1;

    while(1) {
        if(pc.readable()) {
            uart.putc(pc.getc());
        }
        if(uart.readable()) {
            pc.putc(uart.getc());
        }
    }
}