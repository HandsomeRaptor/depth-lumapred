#include <demixer.h>
#include <assert.h>

using namespace std;

void DepthDemixer::UnfoldLow10(const vector<uint16_t> &input, vector<uint16_t> &output)
{
    uint16_t low10;
    for (int i = 0; i < input.size(); i++)
    {
        assert((input[i] & 1023) == input[i]);
        low10 = input[i];
        if (output[i] & 1024)/* ((output[i] >> 10) & 1) */
            low10 = 1023U - low10;
        output[i] |= low10;
    }
}

void DepthDemixer::DecodeHigh2FromChroma(const vector<uint16_t> &input10high1, const vector<uint16_t> &input10high2, const vector<int> &thresholds, const vector<uint16_t> &input10low, vector<uint16_t> &output12)
{
    output12.resize(width * height);

    const int blocksizec = blocksize / 2; // in chroma samples
    const int blocksyc = (height / 2) / blocksizec;
    const int blocksxc = (width / 2) / blocksizec;

    for (int ic = 0; ic < input10high1.size(); ic++) // by chroma plane
    {
        const int yc = ic / (width / 2);
        const int xc = ic % (width / 2);
        const int iy = width * (yc * 2) + (xc * 2); // luma plane index
        const int ib = blocksxc * (yc / blocksizec) + (xc / blocksizec); // threshold averaging block index
        assert(iy + width + 1 < input10low.size());

        PredictHigh(input10low[iy], input10low[iy + 1], input10low[iy + width], input10low[iy + width + 1],
                    input10high1[ic], input10high2[ic],
                    output12[iy], output12[iy + 1], output12[iy + width], output12[iy + width + 1], adaptThresh ? thresholds[ib] : 512);
    }
}

void DepthDemixer::ProcessFrame(const vector<uint16_t> &inputY, const vector<uint16_t> &inputCb, const vector<uint16_t> &inputCr, const vector<int> &inputThresholds, vector<uint16_t> &outDepth)
{
    std::fill(outDepth.begin(), outDepth.end(), 0);
    DecodeHigh2FromChroma(inputCb, inputCr, inputThresholds, inputY, outDepth);
    UnfoldLow10(inputY, outDepth);
}