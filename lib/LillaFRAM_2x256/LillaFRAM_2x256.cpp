/**************************************************************************
 * Derived from FRAM_MB85RC_I2C by SOSAndroid.fr (E. Ha.).
 * Adapted for LILLA Audio Sampler to manage n.2 MB85RC256V devices as a
 * single linear address space.
 *
 * Distributed under the BSD License reproduced in LillaFRAM.h.
 * Copyright (c) 2013, SOSAndroid.fr (E. Ha.). All rights reserved.
 **************************************************************************/

#include "LillaFRAM_2x256.h"

#include <string.h>

LillaFRAM_2x256::LillaFRAM_2x256() : _framInitialised(false), manufacturer(0), productid(0), densitycode(0), density(0)
{
}

byte LillaFRAM_2x256::begin()
{
    return checkDevice();
}

byte LillaFRAM_2x256::checkDevice(void)
{
    _framInitialised = false;

    for (uint8_t chip = 0; chip < FRAM_CHIPS; ++chip)
    {
        uint16_t currentManufacturer = 0;
        uint16_t currentProduct = 0;
        uint16_t currentDensityCode = 0;
        const byte result = getDeviceID(FIRST_I2C_ADDRESS + chip, currentManufacturer, currentProduct, currentDensityCode);
        if (result != ERROR_0 || currentManufacturer != FRAM_MANUFACTURER_ID || currentDensityCode != FRAM_DENSITY_CODE)
        {
            return ERROR_7;
        }

        if (chip == 0)
        {
            manufacturer = currentManufacturer;
            productid = currentProduct;
            densitycode = currentDensityCode;
        }
    }

    density = 256;
    _framInitialised = true;
    return ERROR_0;
}

bool LillaFRAM_2x256::validRange(uint32_t framAddr, uint32_t items)
{
    return items != 0 && framAddr < TOTAL_SIZE && items <= (TOTAL_SIZE - framAddr);
}

uint8_t LillaFRAM_2x256::chipAddress(uint32_t framAddr)
{
    return FIRST_I2C_ADDRESS + static_cast<uint8_t>(framAddr / FRAM_CHIP_SIZE);
}

uint16_t LillaFRAM_2x256::localAddress(uint32_t framAddr)
{
    return static_cast<uint16_t>(framAddr % FRAM_CHIP_SIZE);
}

byte LillaFRAM_2x256::beginAddressTransmission(uint32_t framAddr)
{
    const uint16_t address = localAddress(framAddr);
    Wire2.beginTransmission(chipAddress(framAddr));
    Wire2.write(static_cast<uint8_t>(address >> 8));
    Wire2.write(static_cast<uint8_t>(address & 0xFF));
    return ERROR_0;
}

byte LillaFRAM_2x256::writeArray(uint32_t framAddr, byte items, uint8_t values[])
{
    if (items == 0)
    {
        return ERROR_8;
    }
    if (!validRange(framAddr, items))
    {
        return ERROR_11;
    }

    uint32_t address = framAddr;
    uint16_t remaining = items;
    uint16_t offset = 0;

    while (remaining != 0)
    {
        const uint16_t local = localAddress(address);
        const uint16_t bytesInChip = static_cast<uint16_t>(FRAM_CHIP_SIZE - local);
        const uint16_t chunk = remaining < bytesInChip ? remaining : bytesInChip;

        beginAddressTransmission(address);
        for (uint16_t i = 0; i < chunk; ++i)
        {
            Wire2.write(values[offset + i]);
        }

        const byte result = Wire2.endTransmission();
        if (result != ERROR_0)
        {
            return result;
        }

        address += chunk;
        offset += chunk;
        remaining -= chunk;
    }

    return ERROR_0;
}

byte LillaFRAM_2x256::readArray(uint32_t framAddr, byte items, uint8_t values[])
{
    if (items == 0)
    {
        return ERROR_8;
    }

    if (!validRange(framAddr, items))
    {
        return ERROR_11;
    }

    uint32_t address = framAddr;
    uint16_t remaining = items;
    uint16_t offset = 0;

    while (remaining != 0)
    {
        const uint16_t local = localAddress(address);
        const uint16_t bytesInChip = static_cast<uint16_t>(FRAM_CHIP_SIZE - local);
        const uint16_t chunk = remaining < bytesInChip ? remaining : bytesInChip;

        beginAddressTransmission(address);
        byte result = Wire2.endTransmission();

        if (result != ERROR_0)
        {
            return result;
        }

        const uint8_t received = Wire2.requestFrom(chipAddress(address), static_cast<uint8_t>(chunk));

        if (received != chunk)
        {
            return ERROR_4;
        }

        for (uint16_t i = 0; i < chunk; ++i)
        {
            values[offset + i] = static_cast<uint8_t>(Wire2.read());
        }

        address += chunk;
        offset += chunk;
        remaining -= chunk;
    }

    return ERROR_0;
}

byte LillaFRAM_2x256::writeByte(uint32_t framAddr, uint8_t value)
{
    return writeArray(framAddr, 1, &value);
}

byte LillaFRAM_2x256::readByte(uint32_t framAddr, uint8_t *value)
{
    return readArray(framAddr, 1, value);
}

byte LillaFRAM_2x256::copyByte(uint32_t origAddr, uint32_t destAddr)
{
    uint8_t value = 0;
    byte result = readByte(origAddr, &value);
    return result == ERROR_0 ? writeByte(destAddr, value) : result;
}

byte LillaFRAM_2x256::readBit(uint32_t framAddr, uint8_t bitNb, byte *bit)
{
    if (bitNb > 7)
    {
        return ERROR_9;
    }
    uint8_t value = 0;
    const byte result = readByte(framAddr, &value);

    if (result == ERROR_0)
    {
        *bit = bitRead(value, bitNb);
    }

    return result;
}

byte LillaFRAM_2x256::setOneBit(uint32_t framAddr, uint8_t bitNb)
{
    if (bitNb > 7)
    {
        return ERROR_9;
    }
    uint8_t value = 0;
    byte result = readByte(framAddr, &value);

    if (result != ERROR_0)
    {
        return result;
    }

    bitSet(value, bitNb);
    return writeByte(framAddr, value);
}

byte LillaFRAM_2x256::clearOneBit(uint32_t framAddr, uint8_t bitNb)
{
    if (bitNb > 7)
    {
        return ERROR_9;
    }
    uint8_t value = 0;
    byte result = readByte(framAddr, &value);
    if (result != ERROR_0)
    {
        return result;
    }
    bitClear(value, bitNb);
    return writeByte(framAddr, value);
}

byte LillaFRAM_2x256::toggleBit(uint32_t framAddr, uint8_t bitNb)
{
    if (bitNb > 7)
    {
        return ERROR_9;
    }
    uint8_t value = 0;
    byte result = readByte(framAddr, &value);
    if (result != ERROR_0)
    {
        return result;
    }
    value ^= static_cast<uint8_t>(1U << bitNb);
    return writeByte(framAddr, value);
}

byte LillaFRAM_2x256::readWord(uint32_t framAddr, uint16_t *value)
{
    uint8_t buffer[sizeof(*value)];
    const byte result = readArray(framAddr, sizeof(buffer), buffer);
    if (result == ERROR_0)
    {
        memcpy(value, buffer, sizeof(*value));
    }
    return result;
}

byte LillaFRAM_2x256::writeWord(uint32_t framAddr, uint16_t value)
{
    return writeArray(framAddr, sizeof(value), reinterpret_cast<uint8_t *>(&value));
}

byte LillaFRAM_2x256::readLong(uint32_t framAddr, uint32_t *value)
{
    uint8_t buffer[sizeof(*value)];
    const byte result = readArray(framAddr, sizeof(buffer), buffer);
    if (result == ERROR_0)
    {
        memcpy(value, buffer, sizeof(*value));
    }
    return result;
}

byte LillaFRAM_2x256::writeLong(uint32_t framAddr, uint32_t value)
{
    return writeArray(framAddr, sizeof(value), reinterpret_cast<uint8_t *>(&value));
}

byte LillaFRAM_2x256::getOneDeviceID(uint8_t idType, uint16_t *id)
{
    switch (idType)
    {
    case 1:
        *id = manufacturer;
        return ERROR_0;
    case 2:
        *id = productid;
        return ERROR_0;
    case 3:
        *id = densitycode;
        return ERROR_0;
    case 4:
        *id = density;
        return ERROR_0;
    default:
        *id = 0;
        return ERROR_5;
    }
}

boolean LillaFRAM_2x256::isReady() const
{
    return _framInitialised;
}

byte LillaFRAM_2x256::eraseDevice()
{
    for (uint32_t address = 0; address < TOTAL_SIZE; ++address)
    {
        const byte result = writeByte(address, 0x00);
        if (result != ERROR_0)
        {
            return result;
        }
    }
    return ERROR_0;
}

byte LillaFRAM_2x256::getDeviceID(uint8_t address, uint16_t &manufacturerId, uint16_t &productId, uint16_t &densityCode)
{
    uint8_t buffer[3] = {0, 0, 0};

    Wire2.beginTransmission(FRAM_DEVICE_ID_RESERVED_SLAVE_ID >> 1);
    Wire2.write(static_cast<byte>(address << 1));
    byte result = Wire2.endTransmission(false);
    if (result != ERROR_0)
        return result;

    const uint8_t received = Wire2.requestFrom(FRAM_DEVICE_ID_RESERVED_SLAVE_ID >> 1, 3);
    if (received != 3)
    {
        return ERROR_4;
    }

    for (uint8_t i = 0; i < 3; ++i)
    {
        buffer[i] = static_cast<uint8_t>(Wire2.read());
    }

    manufacturerId = static_cast<uint16_t>((buffer[0] << 4) | (buffer[1] >> 4));
    densityCode = static_cast<uint16_t>(buffer[1] & 0x0F);
    productId = static_cast<uint16_t>(((buffer[1] & 0x0F) << 8) | buffer[2]);
    return ERROR_0;
}

void LillaFRAM_2x256::Destructive_Fram_Test(const uint8_t writevalue)
{
    Serial.println("ArchivingManager::Destructive_Fram_Test(void) - start - first 1000 byte of each chip are written.");

    bool chipPresent[FRAM_CHIPS] = {false};

    // Verifica la presenza di ciascun chip
    for (uint8_t chip = 0; chip < FRAM_CHIPS; ++chip)
    {
        const uint8_t i2cAddress = LillaFRAM_2x256::FIRST_I2C_ADDRESS + chip;

        Wire2.beginTransmission(i2cAddress);
        chipPresent[chip] = (Wire2.endTransmission() == 0);

        Serial.print(F("FRAM "));
        Serial.print(chip);
        Serial.print(F(" at I2C address 0x"));
        Serial.print(i2cAddress, HEX);
        Serial.println(chipPresent[chip] ? F(": PRESENT") : F(": NOT PRESENT"));
    }

    // Test dei primi 1000 indirizzi di ciascun chip
    for (uint8_t chip = 0; chip < FRAM_CHIPS; ++chip)
    {
        if (!chipPresent[chip])
        {
            continue;
        }

        const uint32_t chipBaseAddress = static_cast<uint32_t>(chip) * FRAM_CHIP_SIZE;

        for (uint16_t localAddress = 0; localAddress < 1000; ++localAddress)
        {
            const uint32_t globalAddress = chipBaseAddress + localAddress;
            const byte writeResult = writeByte(globalAddress, writevalue);

            if (writeResult != LillaFRAM_2x256::ERROR_0)
            {
                Serial.print(F("WRITE error on FRAM "));
                Serial.print(chip);
                Serial.print(F(", global location: "));
                Serial.println(globalAddress);
                continue;
            }

            uint8_t readvalue = 0;
            const byte readResult = readByte(globalAddress, &readvalue);

            if (readResult != LillaFRAM_2x256::ERROR_0)
            {
                Serial.print(F("READ error on FRAM "));
                Serial.print(chip);
                Serial.print(F(", global location: "));
                Serial.println(globalAddress);
                continue;
            }

            if (writevalue != readvalue)
            {
                Serial.print(F("NOT corresponding value on FRAM "));
                Serial.print(chip);
                Serial.print(F(", global location: "));
                Serial.println(globalAddress);
            }
        }

        Serial.print("Chip n.");
        Serial.print(chip);
        Serial.println("- Test done.");
    }
}
