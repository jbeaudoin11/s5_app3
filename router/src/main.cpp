#include <mbed.h>
#include <vector>

#include "xbee_com.h"

Serial xbee(p13, p14);

int main() {
    printf("\n\r\n\r== START ==\n\r");
    
    StateMachine(xbee);
}