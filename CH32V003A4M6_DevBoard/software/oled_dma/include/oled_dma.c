// ===================================================================================
// SSD1306 128x64 Pixels I2C OLED Continuous DMA Refresh Functions            * v0.2 *
// ===================================================================================
// 2024 by Stefan Wagner:   https://github.com/wagiminator

#include "oled_dma.h"

// ===================================================================================
// I2C DMA Functions
// ===================================================================================

// Init I2C
void I2C_init(void) {
  // Setup GPIO pins
  #if I2C_MAP == 0
    // Set pin PC1 (SDA) and PC2 (SCL) to output, open-drain, 10MHz, multiplex
    RCC->APB2PCENR |= RCC_AFIOEN | RCC_IOPCEN;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~(((uint32_t)0b1111<<(1<<2)) | ((uint32_t)0b1111<<(2<<2))))
                                 |  (((uint32_t)0b1101<<(1<<2)) | ((uint32_t)0b1101<<(2<<2)));
  #elif I2C_MAP == 1
    // Set pin PD0 (SDA) and PD1 (SCL) to output, open-drain, 10MHz, multiplex
    RCC->APB2PCENR |= RCC_AFIOEN | RCC_IOPDEN;
    AFIO->PCFR1    |= 1<<1;
    GPIOD->CFGLR = (GPIOD->CFGLR & ~(((uint32_t)0b1111<<(0<<2)) | ((uint32_t)0b1111<<(1<<2))))
                                 |  (((uint32_t)0b1101<<(0<<2)) | ((uint32_t)0b1101<<(1<<2)));
  #elif I2C_MAP == 2
    // Set pin PC6 (SDA) and PC5 (SCL) to output, open-drain, 10MHz, multiplex
    RCC->APB2PCENR |= RCC_AFIOEN | RCC_IOPCEN;
    AFIO->PCFR1    |= 1<<22;
    GPIOC->CFGLR = (GPIOC->CFGLR & ~(((uint32_t)0b1111<<(6<<2)) | ((uint32_t)0b1111<<(5<<2))))
                                 |  (((uint32_t)0b1101<<(6<<2)) | ((uint32_t)0b1101<<(5<<2)));
  #else
    #warning Wrong I2C REMAP
  #endif

  // Setup and enable I2C
  RCC->APB1PCENR |= RCC_I2C1EN;                   // enable I2C module clock
  I2C1->CTLR2     = 4;                            // set input clock rate
  I2C1->CKCFGR    = (F_CPU / (3 * I2C_CLKRATE))   // set clock division factor 1:2
                  | I2C_CKCFGR_FS;                // enable fast mode (400kHz)
  I2C1->CTLR1     = I2C_CTLR1_PE;                 // enable I2C

  // Setup DMA Channel 5
  RCC->AHBPCENR |= RCC_DMA1EN;                    // enable DMA module clock
  DMA1_Channel6->PADDR = (uint32_t)&I2C1->DATAR;  // peripheral address
  DMA1_Channel6->CFGR  = DMA_CFG6_MINC            // increment memory address
                       | DMA_CFG6_CIRC            // circular mode
                       | DMA_CFG6_DIR;            // memory to I2C
}

// Start I2C transmission (addr must contain R/W bit)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
void I2C_start(uint8_t addr) {
  while(I2C1->STAR2 & I2C_STAR2_BUSY);            // wait until bus ready
  I2C1->CTLR1 |= I2C_CTLR1_START                  // set START condition
               | I2C_CTLR1_ACK;                   // set ACK
  while(!(I2C1->STAR1 & I2C_STAR1_SB));           // wait for START generated
  I2C1->DATAR = addr;                             // send slave address + R/W bit
  while(!(I2C1->STAR1 & I2C_STAR1_ADDR));         // wait for address transmitted
  uint16_t reg = I2C1->STAR2;                     // clear flags
}
#pragma GCC diagnostic pop

// Send data byte via I2C bus
void I2C_write(uint8_t data) {
  while(!(I2C1->STAR1 & I2C_STAR1_TXE));          // wait for last byte transmitted
  I2C1->DATAR = data;                             // send data byte
}

// Stop I2C transmission
void I2C_stop(void) {
  while(!(I2C1->STAR1 & I2C_STAR1_BTF));          // wait for last byte transmitted
  I2C1->CTLR1 |= I2C_CTLR1_STOP;                  // set STOP condition
}

// Start sending data buffer continuously via I2C bus using DMA
void I2C_writeBuffer(uint8_t* buf, uint16_t len) {
  DMA1_Channel6->CNTR  = len;                     // number of bytes to be transfered
  DMA1_Channel6->MADDR = (uint32_t)buf;           // memory address
  DMA1_Channel6->CFGR |= DMA_CFG6_EN;             // enable DMA channel
  I2C1->CTLR2         |= I2C_CTLR2_DMAEN;         // enable DMA request
}

// ===================================================================================
// Standard ASCII 5x8 font (chars 32 - 127)
// ===================================================================================
const uint8_t OLED_FONT[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
  0x14, 0x7F, 0x14, 0x7F, 0x14, 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62,
  0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x00,
  0x00, 0x41, 0x22, 0x1C, 0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, 0x08, 0x08, 0x3E, 0x08, 0x08,
  0x00, 0x80, 0x60, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x60, 0x60, 0x00, 0x00,
  0x20, 0x10, 0x08, 0x04, 0x02, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x44, 0x42, 0x7F, 0x40, 0x40,
  0x42, 0x61, 0x51, 0x49, 0x46, 0x22, 0x41, 0x49, 0x49, 0x36, 0x18, 0x14, 0x12, 0x7F, 0x10,
  0x2F, 0x49, 0x49, 0x49, 0x31, 0x3E, 0x49, 0x49, 0x49, 0x32, 0x03, 0x01, 0x71, 0x09, 0x07,
  0x36, 0x49, 0x49, 0x49, 0x36, 0x26, 0x49, 0x49, 0x49, 0x3E, 0x00, 0x36, 0x36, 0x00, 0x00,
  0x00, 0x80, 0x68, 0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14,
  0x00, 0x22, 0x14, 0x08, 0x00, 0x02, 0x01, 0x51, 0x09, 0x06, 0x3E, 0x41, 0x5D, 0x55, 0x5E,
  0x7C, 0x12, 0x11, 0x12, 0x7C, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22,
  0x7F, 0x41, 0x41, 0x22, 0x1C, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01,
  0x3E, 0x41, 0x49, 0x49, 0x3A, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x41, 0x41, 0x7F, 0x41, 0x41,
  0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40, 0x40,
  0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
  0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x41, 0xC1, 0xBE, 0x7F, 0x09, 0x19, 0x29, 0x46,
  0x26, 0x49, 0x49, 0x49, 0x32, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x3F, 0x40, 0x40, 0x40, 0x3F,
  0x1F, 0x20, 0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x63, 0x14, 0x08, 0x14, 0x63,
  0x07, 0x08, 0x70, 0x08, 0x07, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x7F, 0x41, 0x41, 0x00,
  0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41, 0x7F, 0x00, 0x08, 0x04, 0x02, 0x04, 0x08,
  0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x03, 0x04, 0x00, 0x20, 0x54, 0x54, 0x54, 0x78,
  0x7F, 0x44, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x28, 0x38, 0x44, 0x44, 0x44, 0x7F,
  0x38, 0x54, 0x54, 0x54, 0x18, 0x08, 0xFE, 0x09, 0x01, 0x02, 0x18, 0xA4, 0xA4, 0xA4, 0x78,
  0x7F, 0x04, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, 0x80, 0x84, 0x7D, 0x00,
  0x41, 0x7F, 0x10, 0x28, 0x44, 0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x7C, 0x04, 0x78,
  0x7C, 0x04, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38, 0xFC, 0x24, 0x24, 0x24, 0x18,
  0x18, 0x24, 0x24, 0x24, 0xFC, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x08, 0x54, 0x54, 0x54, 0x20,
  0x04, 0x3F, 0x44, 0x40, 0x20, 0x3C, 0x40, 0x40, 0x40, 0x3C, 0x1C, 0x20, 0x40, 0x20, 0x1C,
  0x3C, 0x40, 0x30, 0x40, 0x3C, 0x44, 0x28, 0x10, 0x28, 0x44, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C,
  0x44, 0x64, 0x54, 0x4C, 0x44, 0x08, 0x08, 0x36, 0x41, 0x41, 0x00, 0x00, 0xFF, 0x00, 0x00,
  0x41, 0x41, 0x36, 0x08, 0x08, 0x08, 0x04, 0x08, 0x10, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// ===================================================================================
// SSD1306 128x64 Pixels OLED Functions
// ===================================================================================

uint8_t OLED_buffer[OLED_WIDTH * OLED_HEIGHT / 8];// screen buffer

// OLED initialisation sequence
const uint8_t OLED_INIT_CMD[] = {
  0xA8, 0x3F,                                     // set multiplex ratio  
  0x8D, 0x14,                                     // set DC-DC enable  
  0x20, 0x00,                                     // set horizontal addressing mode
  0x21, 0x00, 0x7F,                               // set start and end column
  0x22, 0x00, 0x3F,                               // set start and end page
  0xDA, 0x12,                                     // set com pins
  0xA1, 0xC8,                                     // flip screen
  0xAF                                            // display on
};

// Init OLED
void OLED_init(void) {
  I2C_init();                                     // initialize I2C first
  I2C_start(OLED_ADDR);                           // start transmission to OLED
  I2C_write(OLED_CMD_MODE);                       // set command mode
  for(uint8_t i=0; i<sizeof(OLED_INIT_CMD); i++)
    I2C_write(OLED_INIT_CMD[i]);                  // send the command bytes
  I2C_stop();                                     // stop transmission

  I2C_start(OLED_ADDR);                           // start transmission to OLED
  I2C_write(OLED_DAT_MODE);                       // set command mode
  I2C_writeBuffer(OLED_buffer, sizeof(OLED_buffer)); // send screen buffer using DMA
}

// ===================================================================================
// Graphics Functions
// ===================================================================================

void OLED_clear(void) {
  uint32_t* ptr = (uint32_t*)&OLED_buffer;
  uint32_t  cnt = sizeof(OLED_buffer) >> 2;
  while(cnt--) *ptr++ = (uint32_t)0;
}

uint8_t OLED_getPixel(int16_t xpos, int16_t ypos) {
  if((xpos < 0) || (xpos >= OLED_WIDTH) || (ypos < 0) || (ypos >= OLED_HEIGHT)) return 0;
  return((OLED_buffer[((uint16_t)ypos >> 3) * 128 + xpos] >> (ypos & 7)) & 1);
}

void OLED_setPixel(int16_t xpos, int16_t ypos, uint8_t color) {
  if((xpos < 0) || (xpos >= OLED_WIDTH) || (ypos < 0) || (ypos >= OLED_HEIGHT)) return;
  if(color) OLED_buffer[((uint16_t)ypos >> 3) * 128 + xpos] |=  ((uint8_t)1 << (ypos & 7));
  else      OLED_buffer[((uint16_t)ypos >> 3) * 128 + xpos] &= ~((uint8_t)1 << (ypos & 7));
}

void OLED_drawVLine(int16_t x, int16_t y, int16_t h, uint8_t color) {
  for(int16_t i=y; i<y+h; i++) OLED_setPixel(x, i, color);
}

void OLED_drawHLine(int16_t x, int16_t y, int16_t w, uint8_t color) {
  for(int16_t i=x; i<x+w; i++) OLED_setPixel(i, y, color);
}

void OLED_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
  uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
  if(steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if(x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);
  int16_t err = dx >> 1;
  int16_t ystep;

  if(y0 < y1) ystep = 1;
  else ystep = -1;

  while(x0 <= x1) {
    if(steep) OLED_setPixel(y0, x0, color);
    else      OLED_setPixel(x0, y0, color);
    err -= dy;
    if(err < 0) {
      y0  += ystep;
      err += dx;
    }
    x0++;
  }
}

void OLED_drawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -(r << 1);
  int16_t x = 0;
  int16_t y = r;

  while(x <= y) {
    OLED_setPixel(x0 + x, y0 + y, color);
    OLED_setPixel(x0 - x, y0 + y, color);
    OLED_setPixel(x0 + x, y0 - y, color);
    OLED_setPixel(x0 - x, y0 - y, color);
    OLED_setPixel(x0 + y, y0 + x, color);
    OLED_setPixel(x0 - y, y0 + x, color);
    OLED_setPixel(x0 + y, y0 - x, color);
    OLED_setPixel(x0 - y, y0 - x, color);

    if(f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    ddF_x += 2;
    f += ddF_x;
    x++;
  }
}

void OLED_fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -(r << 1);
  int16_t x = 0;
  int16_t y = r;

  while(x <= y) {
    OLED_drawVLine(x0 - x, y0 - y, (y << 1) + 1, color);
    OLED_drawVLine(x0 + x, y0 - y, (y << 1) + 1, color);
    OLED_drawVLine(x0 - y, y0 - x, (x << 1) + 1, color);
    OLED_drawVLine(x0 + y, y0 - x, (x << 1) + 1, color);

    if(f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  }
}

void OLED_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  OLED_drawHLine(x    , y,     w, color);
  OLED_drawHLine(x    , y+h-1, w, color);
  OLED_drawVLine(x    , y,     h, color);
  OLED_drawVLine(x+w-1, y,     h, color);
}

void OLED_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  for(int16_t i=x; i<x+w; i++) OLED_drawVLine(i, y, h, color);
}

void OLED_drawChar(int16_t x, int16_t y, char c, uint8_t color, uint8_t size) {
  uint16_t ptr = c - 32;
  ptr += ptr << 2;
  for(uint8_t i=6; i; i--) {
    uint8_t line, col;
    int16_t y1 = y;
    if(i == 1) line = 0;
    else line = OLED_FONT[ptr++];
    for(uint8_t j=0; j<8; j++, line>>=1) {
      if(line & 1) col = color;
      else col = !color;
      if(size == 1) OLED_setPixel(x, y1++, col);
      else {
        OLED_fillRect(x, y1, size, size, col);
        y1 += size;
      }
    }
    x += size;
  }
}

void OLED_print(int16_t x, int16_t y, char* str, uint8_t color, uint8_t size) {
  while(*str) {
    OLED_drawChar(x, y, *str++, color, size);
    x += ((size << 2) + (size << 1));
  }
}
