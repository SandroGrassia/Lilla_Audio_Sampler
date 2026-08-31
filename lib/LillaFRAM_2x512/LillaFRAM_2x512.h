/**************************************************************************
 * Derived from FRAM_MB85RC_I2C by SOSAndroid.fr (E. Ha.).
 * Adapted for LILLA Audio Sampler to manage n.2 MB85RC512V devices as a
 * single linear address space.
 *
 * Software License Agreement (BSD License)
 *
 * Copyright (c) 2013, SOSAndroid.fr (E. Ha.)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <config.h>

/*

Driver for a linear bank of n.2 chips LillaFRAM_2x512 (64KByte) on I2C number 2.
 
chip addresses: 0x50, 0x51 (not used 0x52, 0x53)

byte addresses:
FRAM 0:      0  –  65.535
FRAM 1: 65.536  –  131.071

not used:
FRAM 2:
FRAM 3:

This class implements a "global address" strategy:

| FRAM | I²C    | Indirizzo globale | Indirizzo locale |
| 0    | `0x50` | `0x00000–0x0FFFF` | `0x0000–0xFFFF`  |
| 1    | `0x51` | `0x10000–0x1FFFF` | `0x0000–0xFFFF`  |

 */

class LillaFRAM_2x512
{
private:
    // n2 chip FRAM chips Fujitsu MB85RC512V on I2C_2:
    // SCL2 pin 24
    // SDA2 pin 25
    static constexpr uint8_t FRAM_CHIPS = 2;
    static constexpr uint32_t FRAM_CHIP_SIZE = 65536UL;
    static constexpr uint8_t FRAM_DEVICE_ID_RESERVED_SLAVE_ID = 0xF8;
    static constexpr uint16_t FRAM_MANUFACTURER_ID = 0x00A;
    static constexpr uint8_t FRAM_DENSITY_CODE = 0x06; // 512Kbit

    static bool validRange(uint32_t framAddr, uint32_t items);
    static uint8_t chipAddress(uint32_t framAddr);
    static uint16_t localAddress(uint32_t framAddr);
    byte verifyDeviceID(uint8_t address, uint16_t &manufacturerId, uint16_t &productId, uint16_t &densityCode);
    byte beginAddressTransmission(uint32_t framAddr);
    byte checkDevice();

public:
    static constexpr uint8_t FIRST_I2C_ADDRESS = 0x50;
    static constexpr uint32_t TOTAL_SIZE = FRAM_CHIPS * FRAM_CHIP_SIZE;
    static constexpr uint32_t MAX_ADDRESS = TOTAL_SIZE - 1;

    enum Error : byte
    {
        ERROR_0 = 0,
        ERROR_1 = 1,
        ERROR_2 = 2,
        ERROR_3 = 3,
        ERROR_4 = 4,
        ERROR_5 = 5,
        ERROR_6 = 6,
        ERROR_7 = 7,
        ERROR_8 = 8,
        ERROR_9 = 9,
        ERROR_10 = 10,
        ERROR_11 = 11
    };

    LillaFRAM_2x512() {};

    byte begin();
    void Destructive_Fram_Test(const uint8_t writevalue);
    

    byte readBit(uint32_t framAddr, uint8_t bitNb, byte *bit);
    byte setOneBit(uint32_t framAddr, uint8_t bitNb);
    byte clearOneBit(uint32_t framAddr, uint8_t bitNb);
    byte toggleBit(uint32_t framAddr, uint8_t bitNb);
    byte copyByte(uint32_t origAddr, uint32_t destAddr);

    byte readByte(uint32_t framAddr, uint8_t *value);
    byte writeByte(uint32_t framAddr, uint8_t value);

    byte readWord(uint32_t framAddr, uint16_t *value);
    byte writeWord(uint32_t framAddr, uint16_t value);

    byte readLong(uint32_t framAddr, uint32_t *value);
    byte writeLong(uint32_t framAddr, uint32_t value);

    byte readArray(uint32_t framAddr, uint32_t items, uint8_t values[]);
    byte writeArray(uint32_t framAddr, uint32_t items, const uint8_t values[]);

    byte eraseDevice();
};
