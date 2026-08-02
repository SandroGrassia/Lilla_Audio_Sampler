/**************************************************************************
 * Derived from FRAM_MB85RC_I2C by SOSAndroid.fr (E. Ha.).
 * Adapted for LILLA Audio Sampler to manage four MB85RC256V devices as a
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

/*
 * Driver for a linear bank of four MB85RC256V FRAM devices on Wire2.
 *
 * Device addresses: 0x50, 0x51, 0x52, 0x53
 * Linear address:   0x00000 .. 0x1FFFF (128 KiB)
 */
class LillaFRAM
{
private:
    static constexpr uint8_t MASTER_CODE = 0xF8;
    static constexpr uint16_t FUJITSU_MANUFACTURER_ID = 0x00A;
    static constexpr uint8_t MB85RC256V_DENSITY_CODE = 0x05;


    enum class BankCheckError : uint8_t
    {
        None,
        ChipNotPresent,
        DeviceIdReadFailed,
        WrongManufacturer,
        WrongCapacity
    };

    struct BankCheckResult
    {
        BankCheckError error;
        uint8_t chip;
        uint8_t i2cAddress;
        uint16_t manufacturer;
        uint16_t product;
        uint16_t densityCode;

        bool ok() const
        {
            return error == BankCheckError::None;
        }
    };

    boolean _framInitialised;
    uint16_t manufacturer;
    uint16_t productid;
    uint16_t densitycode;
    uint16_t density;

    static bool validRange(uint32_t framAddr, uint32_t items);
    static uint8_t chipAddress(uint32_t framAddr);
    static uint16_t localAddress(uint32_t framAddr);
    byte getDeviceID(uint8_t address, uint16_t &manufacturerId, uint16_t &productId, uint16_t &densityCode);
    byte beginAddressTransmission(uint32_t framAddr);

public:
    static constexpr uint8_t CHIP_COUNT = 4;
    static constexpr uint8_t FIRST_I2C_ADDRESS = 0x50;
    static constexpr uint32_t CHIP_SIZE = 32768UL;
    static constexpr uint32_t TOTAL_SIZE = CHIP_COUNT * CHIP_SIZE;
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

    LillaFRAM();

    void begin();
    bool Destructive_Fram_Test(const uint8_t writevalue);
    byte checkDevice();
    byte readBit(uint32_t framAddr, uint8_t bitNb, byte *bit);
    byte setOneBit(uint32_t framAddr, uint8_t bitNb);
    byte clearOneBit(uint32_t framAddr, uint8_t bitNb);
    byte toggleBit(uint32_t framAddr, uint8_t bitNb);
    byte readArray(uint32_t framAddr, byte items, uint8_t values[]);
    byte writeArray(uint32_t framAddr, byte items, uint8_t values[]);
    byte readByte(uint32_t framAddr, uint8_t *value);
    byte writeByte(uint32_t framAddr, uint8_t value);
    byte copyByte(uint32_t origAddr, uint32_t destAddr);
    byte readWord(uint32_t framAddr, uint16_t *value);
    byte writeWord(uint32_t framAddr, uint16_t value);
    byte readLong(uint32_t framAddr, uint32_t *value);
    byte writeLong(uint32_t framAddr, uint32_t value);
    byte getOneDeviceID(uint8_t idType, uint16_t *id);
    boolean isReady() const;
    byte eraseDevice();
    BankCheckResult checkConfiguredBank();
};
