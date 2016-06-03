#include <icsc.h>

void main() {
    icsc_enable_debug();
    icsc_ptr icsc = icsc_init("/dev/ttyO0", B115200, 12);
    sleep(10); // Wait 10 seconds
    icsc_close(icsc);
}
