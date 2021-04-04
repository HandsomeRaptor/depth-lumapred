#pragma once

#include <predictor.h>
#include <vector>

class DepthMixer : private LumaPredictor
{
private:
    const int width = -1;
    const int height = -1;
    const int adaptThresh = true;

    const uint16_t err_offset = 16U;
    const int blocksize = 128;

    int SAD4x4(const int &a11, const int &a12, const int &a21, const int &a22,
                const int &b11, const int &b12, const int &b21, const int &b22);
    void FoldLow10(const std::vector<uint16_t> &input, std::vector<uint16_t> &output);
    void Encode4x4Block(const uint16_t &y11_10, const uint16_t &y12_10, const uint16_t &y21_10, const uint16_t &y22_10,
                        const uint16_t &y11_12, const uint16_t &y12_12, const uint16_t &y21_12, const uint16_t &y22_12,
                        uint16_t &c1, uint16_t &c2, const int thresh, const bool forceExplicit = false);
    void Encode4x4BlockBest(const uint16_t &y11_10, const uint16_t &y12_10, const uint16_t &y21_10, const uint16_t &y22_10,
                            const uint16_t &y11_12, const uint16_t &y12_12, const uint16_t &y21_12, const uint16_t &y22_12,
                            uint16_t &c1, uint16_t &c2, const int thresh);
    void FindThresholds(const std::vector<uint16_t> &input12, const std::vector<uint16_t> &input10low, std::vector<int> &outputth);
    void EncodeHigh2ToChroma(const std::vector<uint16_t> &input12, const std::vector<uint16_t> &input10low, const std::vector<int> &thresholds, std::vector<uint16_t> &output10high1, std::vector<uint16_t> &output10high2);

public:
    DepthMixer(int width, int height, bool adaptive = true) : width(width), height(height), adaptThresh(adaptive) {};
    ~DepthMixer() {};

    void ProcessFrame(const std::vector<uint16_t> &input, std::vector<uint16_t> &outY, std::vector<uint16_t> &outCb, std::vector<uint16_t> &outCr, std::vector<int> &outThresholds);
};
