#pragma once

#include <predictor.h>
#include <vector>

class DepthDemixer : private LumaPredictor
{
private:
    const int width = -1;
    const int height = -1;
    const int adaptThresh = true;

    const int blocksize = 128;

    void UnfoldLow10(const std::vector<uint16_t> &input, std::vector<uint16_t> &output);
    void DecodeHigh2FromChroma(const std::vector<uint16_t> &input10high1, 
                                const std::vector<uint16_t> &input10high2,
                                const std::vector<int> &thresholds,
                                const std::vector<uint16_t> &input10low,
                                std::vector<uint16_t> &output12);

public:
    DepthDemixer(int width, int height, bool adaptive = true) : width(width), height(height), adaptThresh(adaptive) {};
    ~DepthDemixer() {};

    void ProcessFrame(const std::vector<uint16_t> &inputY, const std::vector<uint16_t> &inputCb, const std::vector<uint16_t> &inputCr, const std::vector<int> &inputThresholds, std::vector<uint16_t> &outDepth);
};