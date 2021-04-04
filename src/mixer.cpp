#include <mixer.h>
#include <cmath>
#include <algorithm>
#include <assert.h>

using namespace std;

int inline DepthMixer::SAD4x4(const int &a11, const int &a12, const int &a21, const int &a22,
                                const int &b11, const int &b12, const int &b21, const int &b22)
{
    return abs(a11 - b11) + abs(a12 - b12) + abs(a21 - b21) + abs(a22 - b22);
}

void DepthMixer::FoldLow10(const vector<uint16_t> &input, vector<uint16_t> &output)
{
    uint16_t low10;
    for (int i = 0; i < input.size(); i++)
    {
        low10 = input[i] & 1023;
        if ((input[i] >> 10) & 1)
            low10 = 1023U - low10;
        output[i] = low10;
    }
}

void DepthMixer::Encode4x4Block(const uint16_t &y11_10, const uint16_t &y12_10, const uint16_t &y21_10, const uint16_t &y22_10,
                                        const uint16_t &y11_12, const uint16_t &y12_12, const uint16_t &y21_12, const uint16_t &y22_12,
                                        uint16_t &c1, uint16_t &c2, const int thresh, const bool forceExplicit)
{
    PredMode mode = forceExplicit ? PredMode::AsIs : GuessModeFromEncoded(y11_10, y12_10, y21_10, y22_10, true, thresh);
    switch (mode)
    {
    case PredMode::AllSame: // mode 1
        c1 = ((y11_12 >> 10) << 5) /* | 512U */;
        c2 = 0;
        break;
    case PredMode::Y11IsDifferent: // mode 2
        c1 = ((y12_12 >> 10) << 5) | ((y11_12 >> 10) << 7);
        c2 = 0;
        break;
    case PredMode::Y12IsDifferent:
        c1 = ((y11_12 >> 10) << 5) | ((y12_12 >> 10) << 7);
        c2 = 0;
        break;
    case PredMode::Y21IsDifferent:
        c1 = ((y11_12 >> 10) << 5) | ((y21_12 >> 10) << 7);
        c2 = 0;
        break;
    case PredMode::Y22IsDifferent:
        c1 = ((y11_12 >> 10) << 5) | ((y22_12 >> 10) << 7);
        c2 = 0;
        break;
    case PredMode::VertSplit:
        c1 = ((y11_12 >> 10) << 5) | ((y12_12 >> 10) << 7);
        c2 = 0;
        break;
    case PredMode::HorzSplit:
        c1 = ((y11_12 >> 10) << 5) | ((y21_12 >> 10) << 7);
        c2 = 0;
        break;
    case PredMode::AsIs: // mode O
        c1 = ((y11_12 >> 10) << 5) | ((y12_12 >> 10) << 7);
        c2 = ((y21_12 >> 10) << 5) | ((y22_12 >> 10) << 7) | 512U;
        break;
    default:
        assert(false);
        break;
    }
    c1 += err_offset;
    c2 += err_offset;
}

void DepthMixer::Encode4x4BlockBest(const uint16_t &y11_10, const uint16_t &y12_10, const uint16_t &y21_10, const uint16_t &y22_10,
                                            const uint16_t &y11_12, const uint16_t &y12_12, const uint16_t &y21_12, const uint16_t &y22_12,
                                            uint16_t &c1, uint16_t &c2, const int thresh)
{
    uint16_t y11e = y11_12 & 1023, y12e = y12_12 & 1023, y21e = y21_12 & 1023, y22e = y22_12 & 1023;
    uint16_t c1e, c2e;
    Encode4x4Block(y11_10, y12_10, y21_10, y22_10, y11_12, y12_12, y21_12, y22_12, c1e, c2e, thresh, false); // get the auto prediction
    PredictHigh(y11_10, y12_10, y21_10, y22_10, c1e, c2e, y11e, y12e, y21e, y22e, thresh); // decode using the implicit pred
    int implicitDist = SAD4x4(y11_12, y12_12, y21_12, y22_12, y11e, y12e, y21e, y22e);
    if (implicitDist != 0)
    {
        y11e = y11_12 & 1023; y12e = y12_12 & 1023; y21e = y21_12 & 1023; y22e = y22_12 & 1023;
        Encode4x4Block(y11_10, y12_10, y21_10, y22_10, y11_12, y12_12, y21_12, y22_12, c1e, c2e, thresh, true);
        PredictHigh(y11_10, y12_10, y21_10, y22_10, c1e, c2e, y11e, y12e, y21e, y22e, thresh);
        int explicitDist = SAD4x4(y11_12, y12_12, y21_12, y22_12, y11e, y12e, y21e, y22e);
        assert(explicitDist == 0);
    }

    c1 = c1e; c2 = c2e;
}

void DepthMixer::FindThresholds(const vector<uint16_t> &input12, const vector<uint16_t> &input10low, vector<int> &outputth)
{
    const int blocksizec = blocksize / 2; // in chroma samples
    const int blocksyc = (height / 2) / blocksizec;
    const int blocksxc = (width / 2) / blocksizec;

    outputth.resize(blocksxc * blocksyc);

    if (!adaptThresh)
    {
        std::fill(outputth.begin(), outputth.end(), 512);
        return;
    }

    vector<int> distr_hsame(blocksizec * blocksizec), distr_hdiff(blocksizec * blocksizec);

    for (int bi = 0; bi < blocksxc * blocksyc; bi++)
    {
        const int ystart = (bi * blocksizec) / (width / 2) * blocksizec;
        const int xstart = (bi * blocksizec) % (width / 2);

        std::fill(distr_hdiff.begin(), distr_hdiff.end(), 0);
        std::fill(distr_hsame.begin(), distr_hsame.end(), 0);

        int ibc = 0;
        for (int yc = ystart; yc < ystart + blocksizec; yc++)
        {
            for (int xc = xstart; xc < xstart + blocksizec; xc++)
            {
                const int iy = width * (yc * 2) + (xc * 2); // luma plane index
                assert(iy + width + 1 < input12.size());
                const int ic = (width / 2) * yc + xc; // chroma plane

                const uint16_t &y11_10 = input10low[iy];
                const uint16_t &y12_10 = input10low[iy + 1];
                const uint16_t &y21_10 = input10low[iy + width];
                const uint16_t &y22_10 = input10low[iy + width + 1];

                const uint16_t &y11_12 = input12[iy];
                const uint16_t &y12_12 = input12[iy + 1];
                const uint16_t &y21_12 = input12[iy + width];
                const uint16_t &y22_12 = input12[iy + width + 1];

                const bool hasHighStep = !(((y11_12 & 3072) == (y12_12 & 3072)) && ((y12_12 & 3072) == (y21_12 & 3072)) && ((y21_12 & 3072) == (y22_12 & 3072)));
                vector<int> diffs2x2 {  abs(y11_10 - y12_10), abs(y11_10 - y21_10), abs(y11_10 - y22_10), 
                            abs(y12_10 - y21_10), abs(y12_10 - y22_10),
                            abs(y21_10 - y22_10) };

                int maxdiff = *(max_element(diffs2x2.begin(), diffs2x2.end()));
                if (hasHighStep) distr_hdiff[ibc++] = maxdiff; else distr_hsame[ibc++] = maxdiff;
            }
        }

        int min_diff = 1023;
        int max_same = 0;
        for (auto e : distr_hdiff)
            if (e > 128 && e < min_diff) min_diff = e;
        for (auto e : distr_hsame)
            if (e > max_same) max_same = e;

        int th = (min_diff - 32) > 0 ? min_diff - 32 : 0;

        outputth[bi] = th;
    }
}

void DepthMixer::EncodeHigh2ToChroma(const std::vector<uint16_t> &input12, const std::vector<uint16_t> &input10low, const std::vector<int> &thresholds, std::vector<uint16_t> &output10high1, std::vector<uint16_t> &output10high2)
{
    uint16_t c1, c2;
    output10high1.resize(width * height / 4);
    output10high2.resize(width * height / 4);

    const int blocksizec = blocksize / 2; // in chroma samples
    const int blocksyc = (height / 2) / blocksizec;
    const int blocksxc = (width / 2) / blocksizec;

    for (int ic = 0; ic < output10high1.size(); ic++) // by chroma plane
    {
        const int yc = ic / (width / 2);
        const int xc = ic % (width / 2);
        const int iy = width * (yc * 2) + (xc * 2); // luma plane index
        const int ib = blocksxc * (yc / blocksizec) + (xc / blocksizec); // threshold averaging block index
        assert(iy + width + 1 < input10low.size());

        Encode4x4BlockBest(input10low[iy], input10low[iy + 1], input10low[iy + width], input10low[iy + width + 1],
                            input12[iy], input12[iy + 1], input12[iy + width], input12[iy + width + 1],
                            c1, c2, thresholds[ib]);
        output10high1[ic] = c1;
        output10high2[ic] = c2;
    }
}

void DepthMixer::ProcessFrame(const vector<uint16_t> &input, vector<uint16_t> &outY, vector<uint16_t> &outCb, vector<uint16_t> &outCr, vector<int> &outThresholds)
{
    outY.resize(width * height);
    FoldLow10(input, outY);
    FindThresholds(input, outY, outThresholds);
    EncodeHigh2ToChroma(input, outY, outThresholds, outCb, outCr);
}
