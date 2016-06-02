#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <endian.h>

#include "icsc.h"
#include "config.h"

static command_ptr commandList = NULL;
static uint8_t myStation = 0;
static int dePin = -1;
static int uartFD = -1;

int icsc_register_command(char command, callbackFunction func) {
    command_ptr newcmd;
    command_ptr scan;

    newcmd = (command_ptr)calloc(1, sizeof(command_t));

    if (newcmd == NULL) {
        fprintf(stderr, "ICSC: Cannot allocate command: %s\n", strerror(errno));
        return -1;
    }

    newcmd->commandCode = command;
    newcmd->callback = func;
    newcmd->next = NULL;

    if (commandList == NULL) {
        commandList = newcmd;
        return 0;
    }

    for (scan = commandList; scan->next; scan = scan->next);
    scan->next = newcmd;

    return 0;
}

int icsc_unregister_command(char command) {
    command_ptr scan;
    command_ptr tmp;

    if (commandList == NULL) {
        return -1;
    }

    // Is it the head?
    if (commandList->commandCode == command) {
        tmp = commandList->next;
        free(commandList);
        commandList = tmp;
        return 0;
    }

    // Scan through for it.
    for (scan = commandList; scan->next; scan = scan->next) {
        if (scan->next->commandCode == command) {
            tmp = scan->next->next;
            free(scan->next);
            scan->next = tmp;
            return 0;
        }
    }

    return 0;
}

int icsc_init_de(const char *uart, unsigned long baud, uint8_t station, int de) {
    myStation = station;
    dePin = de;

    // First try and open the UART.
    uartFD = icsc_serial_open(uart, baud);
    if (uartFD < 0) {
        return uartFD;
    }

    // If we have a GPIO pin specified then open it and set it to listen mode.
    if (dePin >= 0) {
        int rc = icsc_gpio_open(dePin, ICSC_GPIO_OUTPUT);
        if (rc < 0) {
            icsc_serial_close(uartFD);
            return rc;
        }
        rc = icsc_gpio_write(dePin, 0);
        if (rc < 0) {
            icsc_serial_close(uartFD);
            return rc;
        }
    }

    return 0;
}

int icsc_init(const char *uart, unsigned long baud, uint8_t station) {
    return icsc_init_de(uart, baud, station, -1);
}

void icsc_assert_de() {
    if (dePin < 0) {
        return;
    }
    icsc_gpio_write(dePin, 1);
}

void icsc_deassert_de() {
    if (dePin < 0) {
        return;
    }
    icsc_gpio_write(dePin, 0);
}

int icsc_send_raw(uint8_t origin, unsigned char station, char command, uint8_t len, char *data) {
    int i;
    uint8_t cs = 0;

    if (uartFD < 0) {
        return -1;
    }

    icsc_assert_de();

    for (i = 0; i < ICSC_SOH_START_COUNT; i++) {
        icsc_serial_write(uartFD, SOH);
    }

    icsc_serial_write(uartFD, station);
    cs += station;

    icsc_serial_write(uartFD, origin);
    cs += origin;

    icsc_serial_write(uartFD, command);
    cs += command;

    icsc_serial_write(uartFD, len);
    cs += len;

    icsc_serial_write(uartFD, STX);

    for (i = 0; i < len; i++) {
        icsc_serial_write(uartFD, data[i]);
        cs += data[i];
    }

    icsc_serial_write(uartFD, ETX);
    icsc_serial_write(uartFD, cs);
    icsc_serial_write(uartFD, EOT);

    icsc_serial_flush(uartFD);
    icsc_deassert_de();
    return 0;
}

int icsc_send_array(uint8_t station, char command, uint8_t len, char *data) {
    return icsc_send_raw(myStation, station, command, len, data);
}

int icsc_send_string(uint8_t station, char command, char *str) {
    return icsc_send_raw(myStation, station, command, (uint8_t)strlen(str), str);
}

// In ICSC a "long" is a signed 32-bit value. Must be little-endian.
int icsc_send_long(uint8_t station, char command, int32_t data) {
    int32_t le = htole32(data);
    return icsc_send_raw(myStation, station, command, 4, (char *)&le); 
}

// In ICSC an "int" is a signed 16-bit value. Must be little-endian.
int icsc_send_int(uint8_t station, char command, int16_t data) {
    int16_t le = htole16(data);
    return icsc_send_raw(myStation, station, command, 2, (char *)&le); 
}

int icsc_send_char(uint8_t station, char command, int8_t data) {
    return icsc_send_raw(myStation, station, command, 1, (char *)&data); 
}


int icsc_broadcast_array(char command, uint8_t len, char *data) {
    return icsc_send_raw(myStation, ICSC_BROADCAST, command, len, data);
}

int icsc_broadcast_string(char command, char *str) {
    return icsc_send_raw(myStation, ICSC_BROADCAST, command, (uint8_t)strlen(str), str);
}

// In ICSC a "long" is a signed 32-bit value. Must be little-endian.
int icsc_broadcast_long(char command, int32_t data) {
    int32_t le = htole32(data);
    return icsc_send_raw(myStation, ICSC_BROADCAST, command, 4, (char *)&le); 
}

// In ICSC an "int" is a signed 16-bit value. Must be little-endian.
int icsc_broadcast_int(char command, int16_t data) {
    int16_t le = htole16(data);
    return icsc_send_raw(myStation, ICSC_BROADCAST, command, 2, (char *)&le); 
}

int icsc_broadcast_char(char command, int8_t data) {
    return icsc_send_raw(myStation, ICSC_BROADCAST, command, 1, (char *)&data); 
}


