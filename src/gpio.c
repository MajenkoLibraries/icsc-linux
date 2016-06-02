#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "icsc.h"
#include "config.h"

static const char *gpio_root = "/sys/class/gpio";

int icsc_gpio_open(int num, int mode) {
    char temp[200];
    struct stat sb;
    int fd;

    // Check to see if the GPIO has been exported.
    snprintf(temp, 200, "%s/gpio%d", gpio_root, num);
    if (stat(temp, &sb) != 0) {
        // Not exported, so export it.
        snprintf(temp, 200, "%s/export", gpio_root);
        fd = open(temp, O_WRONLY);
        if (fd < 0) {
            fprintf(stderr, "ICSC: Unable to export GPIO%d: %s\n", num, strerror(errno));
            return -1;
        }
        snprintf(temp, 200, "%d\n", num);
        write(fd, temp, strlen(temp));
        close(fd);
    }

    snprintf(temp, 200, "%s/gpio%d/direction", gpio_root, num);
    fd = open(temp, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "ICSC: Unable to export GPIO%d: %s\n", num, strerror(errno));
        return -1;
    }

    switch(mode) {
        case ICSC_GPIO_OUTPUT:
            write(fd, "out\n", 4);
            break;
        case ICSC_GPIO_INPUT:
            write(fd, "in\n", 3);
            break;
    }

    close(fd);

    snprintf(temp, 200, "%s/gpio%d/value", gpio_root, num);
    fd = open(temp, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "ICSC: Unable to export GPIO%d: %s\n", num, strerror(errno));
        return -1;
    }
    close(fd);

    return num;
}

int icsc_gpio_read(int num) {
    char temp[200];
    char c;
    int fd;
    if (num < 0) {
        return;
    }
    snprintf(temp, 200, "%s/gpio%d/value", gpio_root, num);
    fd = open(temp, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "ICSC: Unable to read GPIO%d: %s\n", num, strerror(errno));
        return -1;
    }
    read(fd, &c, 1);
    close(fd);

    if (c == '1') {
        return 1;
    }
    return 0;
}

int icsc_gpio_write(int num, int level) {
    char temp[200];
    int fd;
    if (num < 0) {
        return;
    }
    snprintf(temp, 200, "%s/gpio%d/value", gpio_root, num);
    fd = open(temp, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "ICSC: Unable to write GPIO%d: %s\n", num, strerror(errno));
        return -1;
    }
    write(fd, level ? "1" : "0", 1);
    close(fd);
    return 0;
}

int icsc_gpio_close(int num) {
    char temp[200];
    int fd;
    if (num < 0) {
        return;
    }
    snprintf(temp, 200, "%s/unexport", gpio_root);
    fd = open(temp, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "ICSC: Unable to unexport GPIO%d: %s\n", num, strerror(errno));
        return -1;
    }
    snprintf(temp, 200, "%d\n", num);
    write(fd, temp, strlen(temp));
    close(fd);
    return 0;
}
