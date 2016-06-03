#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "icsc.h"

#include "config.h"

#define    BOTHER 0010000

#define BAUD_CHUNK(B) case B : \
            options.c_cflag &= ~CBAUD;  \
            options.c_cflag |= B;   \
            break;

struct termios _savedOptions;
int icsc_serial_open(const char *path, unsigned long baud) {
    int fd;
    struct termios options;
    fd = open(path, O_RDWR|O_NOCTTY);
    icsc_debug("UART open returned %d\n", fd);
    fcntl(fd, F_SETFL, 0);
    tcgetattr(fd, &_savedOptions);
    tcgetattr(fd, &options);
    cfmakeraw(&options);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CRTSCTS;
    if (strcmp(path, "/dev/tty") == 0) {
        options.c_lflag |= ISIG;
    }


    switch (baud) {
#ifdef B50
        BAUD_CHUNK(B50)
#endif
#ifdef B75
        BAUD_CHUNK(B75)
#endif
#ifdef B110
        BAUD_CHUNK(B110)
#endif
#ifdef B134
        BAUD_CHUNK(B134)
#endif
#ifdef B150
        BAUD_CHUNK(B150)
#endif
#ifdef B200
        BAUD_CHUNK(B200)
#endif
#ifdef B300
        BAUD_CHUNK(B300)
#endif
#ifdef B600
        BAUD_CHUNK(B600)
#endif
#ifdef B1200
        BAUD_CHUNK(B1200)
#endif
#ifdef B1800
        BAUD_CHUNK(B1800)
#endif
#ifdef B2400
        BAUD_CHUNK(B2400)
#endif
#ifdef B4800
        BAUD_CHUNK(B4800)
#endif
#ifdef B9600
        BAUD_CHUNK(B9600)
#endif
#ifdef B19200
        BAUD_CHUNK(B19200)
#endif
#ifdef B38400
        BAUD_CHUNK(B38400)
#endif
#ifdef B57600
        BAUD_CHUNK(B57600)
#endif
#ifdef B115200
        BAUD_CHUNK(B115200)
#endif
#ifdef B230400
        BAUD_CHUNK(B230400)
#endif
#ifdef B460800
        BAUD_CHUNK(B460800)
#endif
#ifdef B500000
        BAUD_CHUNK(B500000)
#endif
#ifdef B576000
        BAUD_CHUNK(B576000)
#endif
#ifdef B921600
        BAUD_CHUNK(B921600)
#endif
#ifdef B1000000
        BAUD_CHUNK(B1000000)
#endif
#ifdef B1152000
        BAUD_CHUNK(B1152000)
#endif
#ifdef B1500000
        BAUD_CHUNK(B1500000)
#endif
#ifdef B2000000
        BAUD_CHUNK(B2000000)
#endif
#ifdef B2500000
        BAUD_CHUNK(B2500000)
#endif
#ifdef B3000000
        BAUD_CHUNK(B3000000)
#endif
#ifdef B3500000
        BAUD_CHUNK(B3500000)
#endif
#ifdef B4000000
        BAUD_CHUNK(B4000000)
#endif
        default:
            icsc_error("Can't set up serial: invalid baud rate\n");
            return -1;
    }

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        icsc_error("Can't set up serial: %s\n", strerror(errno));
        return -1;
    }
    return fd;
}

int icsc_serial_wait_available(int fd, unsigned long timeout) {
    if (fd < 0) {
        return 0;
    }
    
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = timeout & 1000000;
    int retval = select(fd+1, &rfds, NULL, NULL, &tv);
    if (retval) {
        return 1; 
    }
    return 0;
}   

int icsc_serial_available(int fd) {
    return icsc_serial_wait_available(fd, 1);
}   
    
int icsc_serial_read(int fd) {
    if (icsc_serial_available(fd) <= 0) {
        return -1;
    }
    uint8_t c;
    if (read(fd, &c, 1) <= 0) {
        return -1;
    }
    return c;
}       
void icsc_serial_flush(int fd) {
    if (fd < 0) {
        return;
    }
    fsync(fd);
}   

size_t icsc_serial_write(int fd, uint8_t c) {
    if (fd < 0) {
        return 0;
    }
    icsc_debug("Writing 0x%02x to fd %d\n", c, fd);
    write(fd, &c, 1);
    return 1;
}

void icsc_serial_close(int fd) {
    if (fd < 0) {
        return;
    }
    close(fd);
}
