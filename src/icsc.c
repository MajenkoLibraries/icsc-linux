#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <endian.h>
#include <pthread.h>
#include <stdarg.h>

#include "icsc.h"
#include "config.h"

int doDebug = 0;

void icsc_enable_debug() { doDebug = 1; }
void icsc_disable_debug() { doDebug = 0; }

void icsc_debug(const char *f, ...) {
    va_list arg;
    if (doDebug == 1) {
        va_start(arg, f);
        char format[strlen(f) + 15];
        sprintf(format, "ICSC DEBUG: %s", f);
        vfprintf(stderr, format, arg);
        va_end(arg);
    }
}

void icsc_error(const char *f, ...) {
    va_list arg;
    va_start(arg, f);
    char format[strlen(f) + 15];
    sprintf(format, "ICSC ERROR: %s", f);
    vfprintf(stderr, format, arg);
    va_end(arg);
}

static void icsc_assert_de(icsc_ptr icsc) {
    if (icsc->dePin < 0) {
        return;
    }
    icsc_gpio_write(icsc->dePin, 1);
}

static void icsc_deassert_de(icsc_ptr icsc) {
    if (icsc->dePin < 0) {
        return;
    }
    icsc_gpio_write(icsc->dePin, 0);
}

static int icsc_send_raw(icsc_ptr icsc, uint8_t origin, unsigned char station, char command, uint8_t len, const char *data) {
    int i;
    uint8_t cs = 0;

    if (icsc->uartFD < 0) {
        return -1;
    }

    pthread_mutex_lock(&icsc->uartMutex);

    icsc_assert_de(icsc);

    for (i = 0; i < ICSC_SOH_START_COUNT; i++) {
        icsc_serial_write(icsc->uartFD, SOH);
    }

    icsc_serial_write(icsc->uartFD, station);
    cs += station;

    icsc_serial_write(icsc->uartFD, origin);
    cs += origin;

    icsc_serial_write(icsc->uartFD, command);
    cs += command;

    icsc_serial_write(icsc->uartFD, len);
    cs += len;

    icsc_serial_write(icsc->uartFD, STX);

    for (i = 0; i < len; i++) {
        icsc_serial_write(icsc->uartFD, data[i]);
        cs += data[i];
    }

    icsc_serial_write(icsc->uartFD, ETX);
    icsc_serial_write(icsc->uartFD, cs);
    icsc_serial_write(icsc->uartFD, EOT);

    icsc_serial_flush(icsc->uartFD);
    icsc_deassert_de(icsc);
    pthread_mutex_unlock(&icsc->uartMutex);
    return 0;
}

static int icsc_respond_to_ping(icsc_ptr icsc, uint8_t station, uint8_t len, const char *data) {
    return icsc_send_raw(icsc, icsc->station, station, ICSC_SYS_PONG, len, data);
}

static int icsc_process(icsc_ptr icsc, unsigned long timeout) {
    char inch;
    int i;
    command_ptr scan;

    if (icsc == NULL) {
        return -1;
    }

    if (icsc->uartFD < 0) {
        return -1;
    }

    if (icsc_serial_wait_available(icsc->uartFD, timeout) <= 0) {
        return 0;
    }

    while (icsc_serial_available(icsc->uartFD) > 0) {
        inch = icsc_serial_read(icsc->uartFD);

        icsc_debug("Received 0x%02x from FD %d in phase %d\n", inch, icsc->uartFD, icsc->recPhase);

        switch (icsc->recPhase) {
            case 0: // Looking for header
                memcpy(&(icsc->header[0]), &(icsc->header[1]), 5);
                icsc->header[5] = inch;
                if ((icsc->header[0] == SOH) && (icsc->header[5] == STX) && (icsc->header[1] != icsc->header[2])) {
                    icsc->recCalcCS = 0;
                    icsc->recStation = icsc->header[1];
                    icsc->recSender = icsc->header[2];
                    icsc->recCommand = icsc->header[3];
                    icsc->recLen = icsc->header[4];

                    icsc_debug("Found valid header from %d to %d\n", icsc->recSender, icsc->recStation);
                    pthread_mutex_lock(&icsc->uartMutex);

                    for (i = 1; i < 5; i++) {
                        icsc->recCalcCS += icsc->header[i];
                    }
                    icsc->recPhase = 1;
                    icsc->recPos = 0;

                    if ((icsc->recStation != icsc->station) &&
                        (icsc->recStation != ICSC_BROADCAST ||
                         icsc->recStation != ICSC_SYS_RELAY )) {
                        icsc_reset(icsc);
                        break;
                    }

                    icsc_debug("Packet is for me!\n");

                    if (icsc->recLen == 0) {
                        icsc_debug("No payload. Skipping to phase 2\n");
                        icsc->recPhase = 2;
                    } else {
                        icsc_debug("Payload length %d\n", icsc->recLen);
                        icsc->buffer = (char *)malloc(icsc->recLen);
                    }
                }
                break;

            case 1: // Receive data
                icsc->buffer[icsc->recPos++] = inch;
                icsc->recCalcCS += inch;
                if (icsc->recPos == icsc->recLen) {
                    icsc_debug("Finished receiving data\n");
                    icsc->recPhase = 2;
                }
                break;

            case 2: // Check for ETX
                if (inch == ETX) {
                    icsc_debug("Got ETX\n");
                    icsc->recPhase = 3;
                } else {
                    icsc_debug("Expecting ETX but got 0x%02x\n", inch);
                    icsc_reset(icsc);
                }
                break;

            case 3: // Grab the checksum
                icsc->recCS = inch;
                icsc->recPhase = 4;
                icsc_debug("Received checksum byte of 0x%02x\n", inch);
                break;

            case 4: // Check for ETX and check the checksum.
                pthread_mutex_unlock(&icsc->uartMutex);
                if (inch == EOT) {
                    icsc_debug("Got EOT\n");
                    if (icsc->recCS == icsc->recCalcCS) {
                        icsc_debug("Checksum is valid.\n");

                        switch (icsc->recCommand) {
                            case ICSC_SYS_PING:
                                icsc_debug("Responding to ping\n");
                                icsc_respond_to_ping(icsc, icsc->recSender, icsc->recLen, icsc->buffer);
                                break;
                        }

                        for (scan = icsc->commandList; scan; scan = scan->next) {
                            if ((scan->commandCode == icsc->recCommand || scan->commandCode == ICSC_CATCH_ALL) && scan->callback) {
                                icsc_debug("Executing callback for command %c\n", scan->commandCode);
                                scan->callback(icsc, icsc->recSender, icsc->recCommand, icsc->recLen, icsc->buffer);
                            }
                        }
                    } else {
                        icsc_debug("Checksum isn't valid.\n");
                    }
                } 
                icsc_reset(icsc);
        }
            
    }

    return 0;
}

static void *icsc_read_thread(void *arg) {
    icsc_ptr icsc = (icsc_ptr)arg;

    icsc_debug("Read thread executing\n");

    icsc->readThreadRunning = 1;
    while (icsc->readThreadRunning == 1) {
        icsc_process(icsc, 100000); // 100ms timeout
    }

    icsc_debug("Read thread finishing\n");
}

int icsc_register_command(icsc_ptr icsc, char command, callbackFunction func) {
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

    if (icsc->commandList == NULL) {
        icsc->commandList = newcmd;
        return 0;
    }

    for (scan = icsc->commandList; scan->next; scan = scan->next);
    scan->next = newcmd;

    icsc_debug("Registered new command for code '%c'\n", newcmd->commandCode);

    return 0;
}

int icsc_unregister_command(icsc_ptr icsc, char command) {
    command_ptr scan;
    command_ptr tmp;

    if (icsc->commandList == NULL) {
        return -1;
    }

    // Is it the head?
    if (icsc->commandList->commandCode == command) {
        tmp = icsc->commandList->next;
        free(icsc->commandList);
        icsc->commandList = tmp;
        return 0;
    }

    // Scan through for it.
    for (scan = icsc->commandList; scan->next; scan = scan->next) {
        if (scan->next->commandCode == command) {
            icsc_debug("Unregistered command for code '%c'\n", scan->next->commandCode);
            tmp = scan->next->next;
            free(scan->next);
            scan->next = tmp;
            return 0;
        }
    }

    return 0;
}

icsc_ptr icsc_init_de(const char *uart, unsigned long baud, uint8_t station, int de) {
    icsc_ptr newicsc;
    int rc;

    newicsc = (icsc_ptr)calloc(1, sizeof(icsc_t));
    
    if (newicsc == NULL) {
        fprintf(stderr, "ICSC: Cannot allocate new ICSC endpoint: %s\n", strerror(errno));
        return NULL;
    }

    icsc_debug("Endpoint allocated\n");

    // First try and open the UART.
    newicsc->uartFD = icsc_serial_open(uart, baud);
    if (newicsc->uartFD < 0) {
        free(newicsc);
        return NULL;
    }

    icsc_debug("UART %s Opened. FD: %d\n", uart, newicsc->uartFD);

    newicsc->station = station;
    newicsc->dePin = de;

    // If we have a GPIO pin specified then open it and set it to listen mode.
    if (newicsc->dePin >= 0) {
        rc = icsc_gpio_open(newicsc->dePin, ICSC_GPIO_OUTPUT);
        if (rc < 0) {
            icsc_serial_close(newicsc->uartFD);
            free(newicsc);
            return NULL;
        }
        rc = icsc_gpio_write(newicsc->dePin, 0);
        if (rc < 0) {
            icsc_serial_close(newicsc->uartFD);
            free(newicsc);
            return NULL;
        }
    }

    // Now start the reading thread. 

    icsc_debug("Starting read thread\n");

    pthread_mutex_init(&newicsc->uartMutex, NULL);

    pthread_attr_t attr;
    rc = pthread_attr_init(&attr);
    if (rc != 0) {
        fprintf(stderr, "ICSC: Cannot start read thread: %s\n", strerror(errno));
        icsc_serial_close(newicsc->uartFD);
        free(newicsc);
        return NULL;
    }

    rc = pthread_create(&newicsc->readThread, &attr, &icsc_read_thread, newicsc);
    if (rc != 0) {
        fprintf(stderr, "ICSC: Cannot start read thread: %s\n", strerror(errno));
        icsc_serial_close(newicsc->uartFD);
        free(newicsc);
        return NULL;
    }

    pthread_detach(newicsc->readThread);

    pthread_attr_destroy(&attr);

    icsc_debug("Read thread started OK\n");

    return newicsc;
}

icsc_ptr icsc_init(const char *uart, unsigned long baud, uint8_t station) {
    return icsc_init_de(uart, baud, station, -1);
}

int icsc_send_array(icsc_ptr icsc, uint8_t station, char command, uint8_t len, const char *data) {
    return icsc_send_raw(icsc, icsc->station, station, command, len, data);
}

int icsc_send_string(icsc_ptr icsc, uint8_t station, char command, const char *str) {
    return icsc_send_raw(icsc, icsc->station, station, command, (uint8_t)strlen(str), str);
}

// In ICSC a "long" is a signed 32-bit value. Must be little-endian.
int icsc_send_long(icsc_ptr icsc, uint8_t station, char command, int32_t data) {
    int32_t le = htole32(data);
    return icsc_send_raw(icsc, icsc->station, station, command, 4, (const char *)&le); 
}

// In ICSC an "int" is a signed 16-bit value. Must be little-endian.
int icsc_send_int(icsc_ptr icsc, uint8_t station, char command, int16_t data) {
    int16_t le = htole16(data);
    return icsc_send_raw(icsc, icsc->station, station, command, 2, (const char *)&le); 
}

int icsc_send_char(icsc_ptr icsc, uint8_t station, char command, int8_t data) {
    return icsc_send_raw(icsc, icsc->station, station, command, 1, (const char *)&data); 
}


int icsc_broadcast_array(icsc_ptr icsc, char command, uint8_t len, const char *data) {
    return icsc_send_raw(icsc, icsc->station, ICSC_BROADCAST, command, len, data);
}

int icsc_broadcast_string(icsc_ptr icsc, char command, const char *str) {
    return icsc_send_raw(icsc, icsc->station, ICSC_BROADCAST, command, (uint8_t)strlen(str), str);
}

// In ICSC a "long" is a signed 32-bit value. Must be little-endian.
int icsc_broadcast_long(icsc_ptr icsc, char command, int32_t data) {
    int32_t le = htole32(data);
    return icsc_send_raw(icsc, icsc->station, ICSC_BROADCAST, command, 4, (const char *)&le); 
}

// In ICSC an "int" is a signed 16-bit value. Must be little-endian.
int icsc_broadcast_int(icsc_ptr icsc, char command, int16_t data) {
    int16_t le = htole16(data);
    return icsc_send_raw(icsc, icsc->station, ICSC_BROADCAST, command, 2, (const char *)&le); 
}

int icsc_broadcast_char(icsc_ptr icsc, char command, int8_t data) {
    return icsc_send_raw(icsc, icsc->station, ICSC_BROADCAST, command, 1, (const char *)&data); 
}

int icsc_close(icsc_ptr icsc) {
    void *res;
    if (icsc == NULL) {
        return -1;
    }

    icsc_debug("Closing ICSC channel\n");

    icsc->readThreadRunning = 0;
    pthread_join(icsc->readThread, &res);

    icsc_debug("Read thread joined\n");

    if (icsc->commandList != NULL) {
        command_ptr scan;
        command_ptr tmp;

        scan = icsc->commandList;
        while (scan != NULL) {
            tmp = scan->next;
            free(scan);
            scan = tmp;
        }

    }

    free(icsc);
    icsc_debug("Memory freed up\n");
    return 0;
}

int icsc_reset(icsc_ptr icsc) {
    if (icsc->buffer != NULL) {
        free(icsc->buffer);
        icsc->buffer = NULL;
    }
    icsc->recPhase = 0;
    icsc->recPos = 0;
    icsc->recLen = 0;
    icsc->recCommand = 0;
    icsc->recCS = 0;
    icsc->recCalcCS = 0;
    pthread_mutex_unlock(&icsc->uartMutex);
    return 0;
}

