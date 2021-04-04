#include <predictor.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <assert.h>

using namespace std;

LumaPredictor::PredMode LumaPredictor::GuessModeFromEncoded(const uint16_t y11_10, const uint16_t y12_10, const uint16_t y21_10, const uint16_t y22_10, const bool expandthresh/*  = false */, const int thresh/*  = 512 */)
{
    const int thresh_p = thresh - (expandthresh ? 64 : 0);
    const int thresh_n = thresh + (expandthresh ? 64 : 0);
    // diffs2x2 = {abs(y11_10 - y12_10), abs(y11_10 - y21_10), abs(y11_10 - y22_10), 
    //             abs(y12_10 - y21_10), abs(y12_10 - y22_10),
    //             abs(y21_10 - y22_10) };
    diffs2x2[0] = abs(y11_10 - y12_10);
    diffs2x2[1] = abs(y11_10 - y21_10);
    diffs2x2[2] = abs(y11_10 - y22_10);
    diffs2x2[3] = abs(y12_10 - y21_10);
    diffs2x2[4] = abs(y12_10 - y22_10);
    diffs2x2[5] = abs(y21_10 - y22_10);

    // mode 1
    if (*(max_element(diffs2x2.begin(), diffs2x2.end())) < thresh_p)
        return PredMode::AllSame;

    // mode 2, y11 is the outlier
    if ((diffs2x2[3] < thresh_p) && (diffs2x2[4] < thresh_p) && (diffs2x2[5] < thresh_p)
        && ((diffs2x2[0] >= thresh_n) || (diffs2x2[1] >= thresh_n) || diffs2x2[2] >= thresh_n))
        return PredMode::Y11IsDifferent;

    // mode 2, y12 is the outlier
    if ((diffs2x2[1] < thresh_p) && (diffs2x2[2] < thresh_p) && (diffs2x2[5] < thresh_p)
        && ((diffs2x2[0] >= thresh_n) || (diffs2x2[3] >= thresh_n) || diffs2x2[4] >= thresh_n))
        return PredMode::Y12IsDifferent;

    // mode 2, y21 is the outlier
    if ((diffs2x2[0] < thresh_p) && (diffs2x2[2] < thresh_p) && (diffs2x2[4] < thresh_p)
        && ((diffs2x2[1] >= thresh_n) || (diffs2x2[3] >= thresh_n) || diffs2x2[5] >= thresh_n))
        return PredMode::Y21IsDifferent;

    // mode 2, y22 is the outlier
    if ((diffs2x2[0] < thresh_p) && (diffs2x2[1] < thresh_p) && (diffs2x2[3] < thresh_p)
        && ((diffs2x2[2] >= thresh_n) || (diffs2x2[4] >= thresh_n) || diffs2x2[5] >= thresh_n))
        return PredMode::Y22IsDifferent;

    // mode 2, two vertical slices
    diffs4 = { diffs2x2[0], diffs2x2[2], diffs2x2[3], diffs2x2[5] };
    if ( (diffs2x2[1] < thresh_p) && (diffs2x2[4] < thresh_p) && (*(min_element(diffs4.begin(), diffs4.end())) >= thresh_n))
        return PredMode::VertSplit;

    // mode 2, two horizontal slices
    diffs4 = { diffs2x2[1], diffs2x2[2], diffs2x2[3], diffs2x2[4] };
    if ( (diffs2x2[0] < thresh_p) && (diffs2x2[5] < thresh_p) && (*(min_element(diffs4.begin(), diffs4.end())) >= thresh_n))
        return PredMode::HorzSplit;

    // TODO: ADD THE CRISS-CROSS BS

    // explicit coding
    return PredMode::AsIs;
}

void LumaPredictor::PredictHigh(const uint16_t y11_10, const uint16_t y12_10, const uint16_t y21_10, const uint16_t y22_10,
                                const uint16_t c1, const uint16_t c2,
                                uint16_t &y11_h, uint16_t &y12_h, uint16_t &y21_h, uint16_t &y22_h, const int thresh)
{
    assert(y11_10 < 1024 && y12_10 < 1024 && y21_10 < 1024 && y22_10 < 1024 && c1 < 1024 && c2 < 1024);
    if (c2 & (1 << 9)) // explicit mode
    {
        uint16_t cp1, cp2, cp3, cp4;
        cp1 = ((c1 & 96U) >> 5) << 10;
        cp2 = ((c1 & 384U) >> 7) << 10;
        cp3 = ((c2 & 96U) >> 5) << 10;
        cp4 = ((c2 & 384U) >> 7) << 10;
        y11_h |= cp1;
        y12_h |= cp2;
        y21_h |= cp3;
        y22_h |= cp4;
    }
    else
    {
        uint16_t cp1, cp2;
        cp1 = ((c1 & 96U) >> 5) << 10;
        cp2 = ((c1 & 384U) >> 7) << 10;
        PredMode mode = /* (c1 & (1 << 9)) ? PredMode::AllSame :  */GuessModeFromEncoded(y11_10, y12_10, y21_10, y22_10, false, thresh);
        switch (mode)
        {
        case PredMode::AllSame:
            y11_h |= cp1;
            y12_h |= cp1;
            y21_h |= cp1;
            y22_h |= cp1;
            break;
        case PredMode::Y11IsDifferent:
            y11_h |= cp2;
            y12_h |= cp1;
            y21_h |= cp1;
            y22_h |= cp1;
            break;
        case PredMode::Y12IsDifferent:
            y11_h |= cp1;
            y12_h |= cp2;
            y21_h |= cp1;
            y22_h |= cp1;
            break;
        case PredMode::Y21IsDifferent:
            y11_h |= cp1;
            y12_h |= cp1;
            y21_h |= cp2;
            y22_h |= cp1;
            break;
        case PredMode::Y22IsDifferent:
            y11_h |= cp1;
            y12_h |= cp1;
            y21_h |= cp1;
            y22_h |= cp2;
            break;
        case PredMode::VertSplit:
            y11_h |= cp1;
            y12_h |= cp2;
            y21_h |= cp1;
            y22_h |= cp2;
            break;
        case PredMode::HorzSplit:
            y11_h |= cp1;
            y12_h |= cp1;
            y21_h |= cp2;
            y22_h |= cp2;
            break;
        default:
            assert(false);
            y11_h = 0;
            y12_h = 0;
            y21_h = 0;
            y22_h = 0;
            break;
        }
    }
    assert(y11_h < 4096 && y12_h < 4096 && y21_h < 4096 && y22_h < 4096);
}
