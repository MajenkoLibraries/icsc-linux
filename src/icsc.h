#ifndef _ICSC_H
#define _ICSC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ICSC_GPIO_OUTPUT 0
#define ICSC_GPIO_INPUT 1

#define ICSC_BROADCAST  0x00

#define ICSC_CMD_SYS    0x1F

// Shouldn't use anything that would conflict with the header characters.
#define ICSC_SYS_PING   0x05
#define ICSC_SYS_PONG   0x06
#define ICSC_SYS_QSTAT  0x07
#define ICSC_SYS_RSTAT  0x08
//Used when message is relayed to other station via a other station
#define ICSC_SYS_RELAY  0x09

//When this is used during registerCommand all message will pushed
//to the callback function
#define ICSC_CATCH_ALL    0xFF

// Packet wrapping characters, defined in standard ASCII table
#define SOH 1
#define STX 2
#define ETX 3
#define EOT 4

//The number of SOH to start a message
//some device like Raspberry was missing the first SOH
//Increase or decrease the number to your needs
#define ICSC_SOH_START_COUNT 1

struct icsc_command;

typedef struct icsc_command command_t;
typedef struct icsc_command *command_ptr;

typedef struct {
    int uartFD;
    int dePin;
    command_ptr commandList;
    uint8_t station;

    char header[5];

    char *buffer;

    uint8_t recPhase;
    uint8_t recPos;
    uint8_t recCommand;
    uint8_t recLen;
    uint8_t recStation;
    uint8_t recSender;
    uint8_t recCS;
    uint8_t recCalcCS;
} icsc_t, *icsc_ptr;

// Format of command callback functions
typedef void(*callbackFunction)(icsc_ptr, unsigned char, char, unsigned char, char *);

// Structure to store command code / function pairs as a linked list
struct icsc_command {
    char commandCode;
    callbackFunction callback;
    struct icsc_command *next;
};


/* gpio.c */
extern int icsc_gpio_open(int num, int mode);
extern int icsc_gpio_read(int num);
extern int icsc_gpio_write(int num, int level);
extern int icsc_gpio_close(int num);

/* serial.c */
extern int icsc_serial_open(const char *path, unsigned long baud);
extern int icsc_serial_wait_available(int fd, unsigned long timeout);
extern int icsc_serial_available(int fd);
extern int icsc_serial_read(int fd);
extern void icsc_flush(int fd);
extern size_t icsc_serial_write(int fd, uint8_t c);

#ifdef __cplusplus
}
#endif

#endif
