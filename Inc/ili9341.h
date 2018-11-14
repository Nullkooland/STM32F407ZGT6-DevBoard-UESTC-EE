#pragma once
#include <stm32f4xx_hal.h>
#include "lcd.h"

/* External Functions */
extern void Delay_ms(uint16_t ms);
extern void Delay_us(uint32_t us);

static inline void ILI9341_Init(uint8_t orientation)
{
    WRITE_CMD(0x00);
    WRITE_DATA(0x01);

    Delay_ms(50); // delay 50 ms 

    WRITE_CMD(0xCF);
    WRITE_DATA(0x00);
    WRITE_DATA(0xC1);
    WRITE_DATA(0x30);
    WRITE_CMD(0xED);
    WRITE_DATA(0x64);
    WRITE_DATA(0x03);
    WRITE_DATA(0x12);
    WRITE_DATA(0x81);
    WRITE_CMD(0xE8);
    WRITE_DATA(0x85);
    WRITE_DATA(0x10);
    WRITE_DATA(0x7A);
    WRITE_CMD(0xCB);
    WRITE_DATA(0x39);
    WRITE_DATA(0x2C);
    WRITE_DATA(0x00);
    WRITE_DATA(0x34);
    WRITE_DATA(0x02);
    WRITE_CMD(0xF7);
    WRITE_DATA(0x20);
    WRITE_CMD(0xEA);
    WRITE_DATA(0x00);
    WRITE_DATA(0x00);
    WRITE_CMD(0xC0);    //Power control 
    WRITE_DATA(0x1B);   //VRH[5:0] 
    WRITE_CMD(0xC1);    //Power control 
    WRITE_DATA(0x01);   //SAP[2:0];BT[3:0] 
    WRITE_CMD(0xC5);    //VCM control 
    WRITE_DATA(0x30); 	 //3F
    WRITE_DATA(0x30); 	 //3C
    WRITE_CMD(0xC7);    //VCM control2 
    WRITE_DATA(0xB7);
    WRITE_CMD(0x36);    // Memory Access Control 
    WRITE_DATA(0x48);
    WRITE_CMD(0x3A);
    WRITE_DATA(0x55);
    WRITE_CMD(0xB1);
    WRITE_DATA(0x00);
    WRITE_DATA(0x1A);
    WRITE_CMD(0xB6);    // Display Function Control 
    WRITE_DATA(0x0A);
    WRITE_DATA(0xA2);
    WRITE_CMD(0xF2);    // 3Gamma Function Disable 
    WRITE_DATA(0x00);
    WRITE_CMD(0x26);    //Gamma curve selected 
    WRITE_DATA(0x01);
    WRITE_CMD(0xE0);    //Set Gamma 
    WRITE_DATA(0x0F);
    WRITE_DATA(0x2A);
    WRITE_DATA(0x28);
    WRITE_DATA(0x08);
    WRITE_DATA(0x0E);
    WRITE_DATA(0x08);
    WRITE_DATA(0x54);
    WRITE_DATA(0xA9);
    WRITE_DATA(0x43);
    WRITE_DATA(0x0A);
    WRITE_DATA(0x0F);
    WRITE_DATA(0x00);
    WRITE_DATA(0x00);
    WRITE_DATA(0x00);
    WRITE_DATA(0x00);
    WRITE_CMD(0xE1);    //Set Gamma 
    WRITE_DATA(0x00);
    WRITE_DATA(0x15);
    WRITE_DATA(0x17);
    WRITE_DATA(0x07);
    WRITE_DATA(0x11);
    WRITE_DATA(0x06);
    WRITE_DATA(0x2B);
    WRITE_DATA(0x56);
    WRITE_DATA(0x3C);
    WRITE_DATA(0x05);
    WRITE_DATA(0x10);
    WRITE_DATA(0x0F);
    WRITE_DATA(0x3F);
    WRITE_DATA(0x3F);
    WRITE_DATA(0x0F);
    WRITE_CMD(0x2B);
    WRITE_DATA(0x00);
    WRITE_DATA(0x00);
    WRITE_DATA(0x01);
    WRITE_DATA(0x3F);
    WRITE_CMD(0x2A);
    WRITE_DATA(0x00);
    WRITE_DATA(0x00);
    WRITE_DATA(0x00);
    WRITE_DATA(0xEF);

    WRITE_CMD(0x11); //Exit Sleep
    Delay_ms(120);
    WRITE_CMD(0x29); //display on

    if (orientation)
    {
        WRITE_CMD(0x36);
        WRITE_DATA(0x08);
    }
    else
    {
        WRITE_CMD(0x36);
        WRITE_DATA(0xA8);
    }
}