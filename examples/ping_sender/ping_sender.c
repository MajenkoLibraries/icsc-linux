#include <icsc.h>
#include <stdlib.h>
#include <stdio.h>

void pingReceived(icsc_ptr icsc, unsigned char from, char cmd, unsigned char len, char *data) {
    printf("PING reply from %d!\n", from);
}

void main() {
    int i;
//    icsc_enable_debug();
    // Station 50, GPIO49 = DE (BBB pin 23), 115200 baud, /dev/ttyO0 (BBB pins 24/26)
    icsc_ptr icsc = icsc_init_de("/dev/ttyO1", B115200, 50, 49);
    if (icsc == NULL) {
        exit(-1);
    }

    icsc_register_command(icsc, ICSC_SYS_PONG, pingReceived);

    for (i = 0; i < 10; i++) {
        sleep(1);
        icsc_send_array(icsc, 100, ICSC_SYS_PING, 0, NULL);
    }
    sleep(1);
    icsc_close(icsc);
}
