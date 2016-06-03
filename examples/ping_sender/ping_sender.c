#include <icsc.h>

void main() {
    icsc_ptr icsc = icsc_init("/dev/ttyO1", B115200, 12);
    icsc_close(icsc);
}
