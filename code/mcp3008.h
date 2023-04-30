#pragma once

#include "pico/types.h"
#include <array>

namespace mcp3008
{
    enum channels : uint8_t
    {
        CH0 = 0,
        CH1 = 1,
        CH2 = 2,
        CH3 = 3,
        CH4 = 4,
        CH5 = 5,
        CH6 = 6,
        CH7 = 7,
    };

    static constexpr std::array<uint8_t, 3> encode(channels channel)
    {
        std::array<uint8_t, 3> data{};
        uint8_t mode = 0;
        mode = channel | 0b1000;
        mode = mode << 4;
        data[0] = 0x01;
        data[1] = mode;
        data[2] = 0x00;
        return data;
    }

    static constexpr uint32_t decode(std::array<uint8_t, 3> data)
    {
        uint32_t result = 0;
        // Combine two bytes
        result = data[1] << 8 | data[2];
        // Look at only 10 bits
        result = result & 0x3FF;
        return result;
    }

}
