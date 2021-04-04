#pragma once

#include <cstdint>
#include <vector>
#include <array>

class LumaPredictor
{
private:
    std::array<int, 6> diffs2x2;
    std::array<int, 4> diffs4;
    
protected:
    enum class PredMode
    {
        AllSame = 1,
        Y11IsDifferent,
        Y12IsDifferent,
        Y21IsDifferent,
        Y22IsDifferent,
        VertSplit,
        HorzSplit,
        AsIs = 15
    };

    PredMode GuessModeFromEncoded(const uint16_t y11_10, const uint16_t y12_10, const uint16_t y21_10, const uint16_t y22_10, const bool expandthresh = false, const int thresh = 512);
    void PredictHigh(const uint16_t y11_10, const uint16_t y12_10, const uint16_t y21_10, const uint16_t y22_10,
                        const uint16_t c1, const uint16_t c2,
                        uint16_t &y11_h, uint16_t &y12_h, uint16_t &y21_h, uint16_t &y22_h, const int thresh);

    LumaPredictor() {};
    ~LumaPredictor() {};
};
