/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedVFS.h"
#include <cstring>

const char *Get_packet_name(uint16_t packet_id, char (&name)[NAME_PACKET_SIZE])
{
    char digits[5];
    uint8_t length = 0;

    do
    {
        digits[length++] = '0' + packet_id % 10;
        packet_id /= 10;
    } while (packet_id != 0);

    constexpr size_t prefix_length = sizeof(RECORDING_PACKET_PREFIX) - 1;
    memcpy(name, RECORDING_PACKET_PREFIX, prefix_length);

    for (uint8_t index = 0; index < length; ++index)
    {
        name[index + prefix_length] = digits[length - index - 1];
    }

    memcpy(name + length + prefix_length, ".raw", 5);
    return name;
}

// VFS/Virtual File System

int VFS_FAT_table[VFS_PACKETS_DS] = {0};
int VFS_packets = 0; // number of Packets on FLASH (maximum is VFS_PACKETS_MAX)
int VFS_packets_max = 0;

VFS_Recording Recording[RECORDINGS];

int VFS_Get_packets_free(void)
{
    int value = 0;

    for (auto i = DS_First_packet; i < DS_VFS_packets; ++i)
    {
        if (VFS_FAT_table[i] == -1)
        {
            ++value;
        }
    }

    return value;
}
