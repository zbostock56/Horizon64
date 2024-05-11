#pragma once

/* Reference: https://wiki.osdev.org/"8042"_PS/2_Controller */

/*
  "8042" PS/2 Controller

  In the old days, this chip was discrete. Today it is part of the
  Advanced Integrated Peripheral (AIP).
*/

/* PS/2 Controller IO Ports */
typedef enum {
  PS2_DATA_PORT         = 0x60, /* Access: read/write */
  PS2_STATUS_REGISTER   = 0x64, /* Access: read */
  PS2_COMMAND_REGISTER  = 0x64, /* Access: write */
} PS2_CONTROLLER_IO_PORTS;

/*
  The PS/2 Controller accepts commands and performs them.
  These commands should not be confused with bytes sent to a PS/2 device
  (e.g. keyboard, mouse).

  To send a command to the controller, write the command byte to IO port 0x64.
  If there is a "next byte" (the command is 2 bytes) then the next byte needs
  to be written to IO Port 0x60 after making sure that the controller is ready
  for it (by making sure bit 1 of the Status Register is clear).
  If there is a response byte, then the response byte needs to be read from
  IO Port 0x60 after making sure it has arrived (by making sure bit 0 of the
  Status Register is set).

  NOTE: When a controller only supports one PS2 port, the commands which
        are defined for the second PS2 port should be treated as chipset
        specific or unknown and should not be issued.
*/
typedef enum {
  /* Response: Controller Bonfiguration Byte */
  COMMAND_READ_BYTE_INTERNAL_RAM        = 0x20,
  /* Response: None */
  COMMAND_WRITE_BYTE_INTERNAL_RAM       = 0x60,
  /*
    Response: None
    Note: Only funtions if 2 PS2 ports are supported
  */
  COMMAND_DISABLE_SECOND_PS2_PORT       = 0xA7,
  /*
    Response: None
    Note: Only funtions if 2 PS2 ports are supported
  */
  COMMAND_ENABLE_SECOND_PS2_PORT        = 0xA8,
  /*
    Reponse:
    - 0x00 : test passed
    - 0x01 : clock line stuck low
    - 0x02 : clock line stuck high
    - 0x03 : data line stuck low
    - 0x04 : data line stuck high
    Note: Only functions if 2 PS2 ports are supported
  */
  COMMAND_TEST_SECOND_PS2_PORT          = 0xA9,
  /*
    Response:
    - 0x55 : test passed
    - 0xFC : test failed
  */
  COMMAND_TEST_PS2_CONTROLLER           = 0xAA,
  /*
    Reponse:
    - 0x00 : test passed
    - 0x01 : clock line stuck low
    - 0x02 : clock line stuck high
    - 0x03 : data line stuck low
    - 0x04 : data line stuck high
  */
  COMMAND_TEST_FIRST_PS2_PORT           = 0xAB,
  /* Reponse: unknown */
  COMMAND_DIAGNOSTIC_DUMP               = 0xAC,
  /* Response: None */
  COMMAND_DISABLE_FIRST_PS2_PORT        = 0xAD,
  /* Response: None */
  COMMAND_ENABLE_FIRST_PS2_PORT         = 0xAE,
  /* Respone: unknown (none of these bits have a standard) */
  COMMAND_READ_CONTROLLER_INPUT         = 0xC0,
  /*
    Response: None
    Description:
      Copies bits 0 through 3 of the input port to status bits 4 through 7
  */
  COMMAND_CP_0_3_INPUT_STATUS_4_7       = 0xC1,
  /*
    Response: None
    Description:
      Copies bits 4 through 7 of the input port to status bits 4 through 7
  */
  COMMAND_CP_4_7_INPUT_STATUS_4_7       = 0xC2,
  /*
    Response: bitstring from controller (refer to controller output port)
  */
  COMMAND_READ_CONTROLLER_OUTPUT        = 0xD0,
  /*
    Response: None
    Note: Check to make sure the output buffer is empty first
  */
  COMMAND_WRITE_NEXT_BYTE_CONTR_OUT     = 0xD1,
  /*
    Respone: None
    Note: Only functions if 2 PS2 ports are supported
    Description: writes byte to port 1 output
  */
  COMMAND_WRITE_NEXT_BYTE_PS2_P1_OUT    = 0xD2,
  /*
    Respone: None
    Note: Only functions if 2 PS2 ports are supported
    Description: writes byte to port 2 output
  */
  COMMAND_WRITE_NEXT_BYTE_PS2_P2_OUT    = 0xD3,
  /*
    Respone: None
    Note: Only functions if 2 PS2 ports are supported
    Description: writes byte to port 2 input
  */
  COMMAND_WRITE_NEXT_BYTE_PS2_P2_IN     = 0xD4,
} PS2_CONTROLLER_COMMANDS;

/* PS2 Controller Configuration Byte */
/*
  Commands 0x20 and 0x60 let you read and write from PS2 controller
  configuration byte. This defines the output:

  Bit 0 : First PS2 port interrupt (1 = enabled, 0 = disabled)
  Bit 1 : Second PS2 port interrupt (1 = enabled, 0 = disabled, only if 2
          PS2 ports are supported)
  Bit 2 : System Flag (1 = system passed POST, 0 = OS should not be running)
  Bit 3 : Should be zero
  Bit 4 : First PS2 port clock (1 = disabled, 0 = enabled)
  Bit 5 : Second PS2 port clock (1 = disabled, 0 = enabled, only if 2 PS2 ports
          are supported)
  Bit 6 : First PS2 port translation (1 = enabled, 0 = disabled)
  Bit 7 : Must be zero
*/
#define PS2_CONTROLLER_CONFIG_BYTE_N (input, n) (input & (0x1 << n))

/* PS2 Controller Output Port */
/*
  Commands 0xD0 and 0xD1 let you read and write the PS2 controller output port.
  This defines the input:

  Bit 0 : System Reset (output). One must pulse the reset line (e.g. sending
          command 0xFE), and setting this bit to '0' can lock up the computer
  Bit 1 : A20 gate (output)
  Bit 2 : Second PS2 port clock (output, only if 2 PS2 ports supported)
  Bit 3 : Second PS2 port data (output, only if 2 PS2 ports supported)
  Bit 4 : Output buffer full with byte from first PS2 port (connected to IRQ 1)
  Bit 5 : Output buffer full with byte from second PS2 port (connected to IRQ 12
          only if two PS2 ports are supported)
  Bit 6 : First PS2 port clock (output)
  Bit 7 : First PS2 port data (output)
*/
#define PS2_CONTROLLER_OUTPUT_PORT_BYTE_N (input, n) (input & (0x1 << n))

/* Status Register Flags */
/*
  The status register contains flags which show the state of the PS2 controller.
  The output is defined as follows:

  Bit 0 : Output buffer status (0 = empty, 1 = full) (must be SET before
          attempting to read data from IO port 0x60)
  Bit 1 : Input buffer status (0 = empty, 1 = full) (must be CLEAR before
          attempting to write data to IO port 0x60 or IO port 0x64)
  Bit 2 : System flag (meant to be cleared on reset and set by firmware (via.
          PS2 Controller Configuration Byte) if the system POSTs)
  Bit 3 : Command/data (0 = data written to input buffer is data for PS2 device,
          1 = data written to input buffer is data for PS2 controller command)
  Bit 4 : Unknown
  Bit 5 : Unkown
  Bit 6 : Time-out error (0 = no error, 1 = time-out error)
  Bit 7 : Parity error (0 = no error, 1 = parity error)
*/
#define PS2_STATUS_REGISTER_BYTE_N (input, n) (input & (0x1 << n))


/* Use to get at different bytes in the controllers memory (no response byte) */
#define READ_BYTE_INTERNAL_RAM (offset) (COMMAND_READ_BYTE_INTERNAL_RAM + offset)
/* Use to get write to different bytes in contorllers memory (no response) */
#define WRITE_BYTE_INTERNAL_RAM (offset) { \
  COMMAND_WRITE_BYTE_INTERNAL_RAM + offset \
}
