/** @file icsc.h
 *  @brief C libray for ICSC communication
 */

/** \mainpage
 * \copyright
 * Copyright (c) 2016, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _ICSC_H
#define _ICSC_H

#include <stdint.h>
#include <termios.h>
#include <sys/types.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


#define ICSC_BROADCAST  0x00
#define ICSC_CMD_SYS    0x1F
#define ICSC_SYS_PING   0x05
#define ICSC_SYS_PONG   0x06
#define ICSC_SYS_QSTAT  0x07
#define ICSC_SYS_RSTAT  0x08
#define ICSC_SYS_RELAY  0x09

//When this is used during registerCommand all message will pushed
//to the callback function

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

    pthread_t readThread;
    int readThreadRunning;
    pthread_mutex_t uartMutex;
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

/** \defgroup gpio
 *  \brief Helper functions to make working with GPIO easier.
 * @{
 */

#define ICSC_GPIO_OUTPUT 0
#define ICSC_GPIO_INPUT 1

/*! \brief Open a GPIO device in either input or output mode. Exports the GPIO node.
 *  \param num GPIO number
 *  \param mode Either ICSC_GPIO_INPUT or ICSC_GPIO_OUTPUT
 *  \return 0 on success or -1 on error.
 */
extern int icsc_gpio_open(int num, int mode);

/*! \brief Read the value of a GPIO
 *  \param num GPIO number
 *  \return 1 if the GPIO reads high, 0 if it reads low, or -1 on an error.
 */
extern int icsc_gpio_read(int num);

/*! \brief Set a GPIO to high or low.
 *  \param num GPIO number
 *  \param level 1 for logic high or 0 for logic low
 *  \return 0 on success or -1 on error.
 */
extern int icsc_gpio_write(int num, int level);

/*! \brief Close a GPIO. Unexports the GPIO node.
 *  \param num GPIO number
 *  \return 0 on success or -1 on error.
 */
extern int icsc_gpio_close(int num);

/** @} */

/* serial.c */
/** \defgroup serial
 *  \brief Helper functions to make working with serial devices easier.
 * @{
 */

/*! \brief Open a serial device at a specific baud rate
 *  \param path Path to the serial device (e.g., /dev/ttyAMA0)
 *  \param baud Symbolic baud rate for the port in the form Bxxx (e.g., B115200)
 *  \return The file descriptor for the newly opened port or -1 on an error.
 */
extern int icsc_serial_open(const char *path, unsigned long baud);

/*! \brief Wait for serial data to arrive up until the timeout expires
 *  \param fd The file descriptor of the port opened by icsc_serial_open()
 *  \param timeout The maximum number of microseconds to wait for data to arrive
 *  \return 1 if data is available, 0 if it timed out, or -1 on an error.
 */
extern int icsc_serial_wait_available(int fd, unsigned long timeout);

/*! \brief Look to see if serial data is available
 *  \param fd The file descriptor of the port opened by icsc_serial_open()
 *  \return 1 if data is available, 0 if there is none, or -1 on an error.
 */
extern int icsc_serial_available(int fd);

/*! \brief Read a byte from the serial port
 *  \param fd The file descriptor of the port opened by icsc_serial_open()
 *  \return A byte from the serial buffer, or -1 if no bytes are available.
 */
extern int icsc_serial_read(int fd);
/*! \brief Wait until all data sent to the serial port has been delivered to the wire
 *  \param fd The file descriptor of the port opened by icsc_serial_open()
 *  \return nothing
 */
extern void icsc_serial_flush(int fd);

/*! \brief Write a byte to a serial port
 *  \param fd The file descriptor of the port opened by icsc_serial_open()
 *  \param c The byte to write to the port
 *  \return 0 if the byte was written, -1 on error.
 */
extern int icsc_serial_write(int fd, uint8_t c);

/*! \brief Close the serial port
 *  \param fd The file descriptor of the port opened by icsc_serial_open()
 *  \return nothing
 */
extern void icsc_serial_close(int fd);
/** @} */

/* icsc.c */

/** \defgroup callbacks
 *  \brief Functions for dealing with commands and callback functions
 * @{
 */
#define ICSC_CATCH_ALL    0xFF

/*! \brief Register a new command callback
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param command The character to use for the command
 *  \param func The callback function to call when the command is received
 *  \return 0 if the command was registered successfully, otherwise -1 on an error.
 */
extern int icsc_register_command(icsc_ptr icsc, char command, callbackFunction func);

/*! \brief Unregister an old command character.
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param command The command character to unregister
 *  \return 0 if the command was unregistered successfully, otherwise -1 on an error.
 */
extern int icsc_unregister_command(icsc_ptr icsc, char command);

/** @}*/

/** \defgroup Initialization
 *  \brief Functions used for initializing and finisging with an ICSC instance
 *  @{
 */

/*! \brief Create a new ICSC context, initialize the hardware, and start listening
 *         for messages.
 *  \param uart The path name of the UART device to communicate with (e.g., /dev/ttyAMA0)
 *  \param baud The baud rate symbolic name in the form Bxxxx (e.g., B115200)
 *  \param station The station number of this device
 *  \param de The GPIO number to use for the RS-485 DE pin.
 *  \return The pointer to the newly created context.
 */
extern icsc_ptr icsc_init_de(const char *uart, unsigned long baud, uint8_t station, int de);

/*! \brief Create a new ICSC context, initialize the hardware, and start listening
 *         for messages.
 *  \param uart The path name of the UART device to communicate with (e.g., /dev/ttyAMA0)
 *  \param baud The baud rate symbolic name in the form Bxxxx (e.g., B115200)
 *  \param station The station number of this device
 *  \return The pointer to the newly created context.
 */
extern icsc_ptr icsc_init(const char *uart, unsigned long baud, uint8_t station);

/*! \brief Close an ICSC instance freeing the memory. Terminates all communication.
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \return 0 on success or -1 on error.
 */
extern int icsc_close(icsc_ptr icsc);

/** @} */


/** \defgroup sending
 *  \brief Functions used for sending data to a remote station
 *  @{
 */

/*! \brief Send an array of data (or struct as if it were an array) to a remote station
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param station Destination station to send to
 *  \param command Command character to trigger at the remote station
 *  \param len The length of the array or size of the struct
 *  \param data The data to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_send_array(icsc_ptr icsc, uint8_t station, char command, uint8_t len, const char *data);

/*! \brief Send a text string to a remote station
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param station Destination station to send to
 *  \param command Command character to trigger at the remote station
 *  \param str The string to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_send_string(icsc_ptr icsc, uint8_t station, char command, const char *str);

/*! \brief Send a 32-bit integer to a remote station
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param station Destination station to send to
 *  \param command Command character to trigger at the remote station
 *  \param data The integer to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_send_long(icsc_ptr icsc, uint8_t station, char command, int32_t data);

/*! \brief Send a 16-bit integer to a remote station
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param station Destination station to send to
 *  \param command Command character to trigger at the remote station
 *  \param data The integer to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_send_int(icsc_ptr icsc, uint8_t station, char command, int16_t data);

/*! \brief Send an 8-bit integer to a remote station
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param station Destination station to send to
 *  \param command Command character to trigger at the remote station
 *  \param data The integer to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_send_char(icsc_ptr icsc, uint8_t station, char command, int8_t data);

/** @} */



/** \defgroup broadcast
 *  \brief Functions used for broadcasting data to all remote stations
 *  @{
 */

/*! \brief Send an array of data (or struct as if it were an array) to all stations
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param command Command character to trigger at the remote station
 *  \param len The length of the array or size of the struct
 *  \param data The data to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_broadcast_array(icsc_ptr icsc, char command, uint8_t len, const char *data);

/*! \brief Send a text string to all remote stations
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param command Command character to trigger at the remote station
 *  \param str The string to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_broadcast_string(icsc_ptr icsc, char command, const char *str);

/*! \brief Send a 32-bit integer to all remote stations
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param command Command character to trigger at the remote station
 *  \param data The integer to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_broadcast_long(icsc_ptr icsc, char command, int32_t data);

/*! \brief Send a 16-bit integer to all remote stations
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param command Command character to trigger at the remote station
 *  \param data The integer to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_broadcast_int(icsc_ptr icsc, char command, int16_t data);

/*! \brief Send an 8-bit integer to all remote stations
 *  \param icsc Pointer to an icsc context created using icsc_init() or icsc_init_de()
 *  \param command Command character to trigger at the remote station
 *  \param data The integer to send
 *  \return 0 on success, -1 on error.
 */
extern int icsc_broadcast_char(icsc_ptr icsc, char command, int8_t data);
/** @} */


/** \defgroup debugging
 *  \brief Functions used for debugging and error reporting
 *  @{
 */

/*! \brief Enable debug messages
 *  
 *  * Caution - this gets very noisy
 *  
 *  \param none
 *  \return nothing
 */
extern void icsc_enable_debug();

/*! \brief Disable debug messages
 *  
 *  \param none
 *  \return nothing
 */
extern void icsc_disable_debug();

/*! \brief Display a debug message to stderr
 *
 *  The string "ICSC DEBUG: " is prepended to the message. Parameters
 *  and formatting follow the same rules as printf().
 *
 *  This function only output anything if debugging is first enabled with
 *  icsc_enable_debug().
 */
extern void icsc_debug(const char *fmt, ...);


/*! \brief Display an error message to stderr
 *
 *  The string "ICSC ERROR: " is prepended to the message. Parameters
 *  and formatting follow the same rules as printf().
 */
extern void icsc_error(const char *fmt, ...);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
