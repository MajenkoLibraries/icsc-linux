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

struct termios _savedOptions;
int icsc_serial_open(const char *path, unsigned long baud) {
    int fd;
    struct termios options;
    fd = open(path, O_RDWR|O_NOCTTY);
    fcntl(fd, F_SETFL, 0);
    tcgetattr(fd, &_savedOptions);
    tcgetattr(fd, &options);
    cfmakeraw(&options);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag &= ~CBAUD;
    if (strcmp(path, "/dev/tty") == 0) {
        options.c_lflag |= ISIG;
    }
    options.c_cflag |= BOTHER;
    options.c_ispeed = baud;
    options.c_ospeed = baud;
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        fprintf(stderr, "ICSC: Can't set up serial: %s\n", strerror(errno));
        return -1;
    }
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
    return icsc_serial_wait_timeout(fd, 1);
}   
    
int icsc_serial_read(int fd) {
    if (!available()) {
        return -1;
    }
    uint8_t c;
    if (read(fd, &c, 1) <= 0) {
        return -1;
    }
    return c;
}       
void icsc_flush(int fd) {
    if (fd < 0) {
        return;
    }
    fsync(fd);
}   

size_t icsc_serial_write(int fd, uint8_t c) {
    if (fd < 0) {
        return 0;
    }
    write(fd, &c, 1);
    return 1;
}

