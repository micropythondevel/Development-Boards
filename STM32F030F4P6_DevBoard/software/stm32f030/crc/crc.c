// ===================================================================================
// Project:   Example for STM32F030
// Version:   v1.0
// Year:      2023
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// Send a message through UART with a newline at the end, and the CRC of the message 
// will be sent back.
//
// Compilation Instructions:
// -------------------------
// - Make sure GCC toolchain (gcc-arm-none-eabi) and Python3 with PySerial is
//   installed. If necessary, a driver for the USB-to-serial converter used must
//   be installed.
// - Connect your MCU board via USB to your PC.
// - Set the MCU to boot mode by holding down the BOOT key and then pressing and 
//   releasing the RESET key. Finally release the BOOT key.
// - Run 'make flash'.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================
#include "system.h"                 // system functions
#include "uart.h"                   // UART functions

// ===================================================================================
// Main Function
// ===================================================================================
int main (void) {
  // Setup
  UART_init();                      // init UART, 8N1, BAUD: 115200, PA9/PA10
  CRC_enable();                     // enable CRC module

  // Loop
  while(1) {
    char c = UART_read();           // read character from UART
    if(c != '\n') CRC_data8 = c;    // not newline -> add to 8-bit CRC calculation
    else {                          // newline ->
      UART_print("CRC: ");          //   
      UART_printB(CRC_data8);       //   read CRC calculation and send it via UART
      UART_newline();               //   print newline
      CRC_reset();                  //   reset CRC calculation
    }
  }
}
