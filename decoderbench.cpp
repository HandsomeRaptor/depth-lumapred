#include <demixer.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////
static const int           yuv_width = 2048;
static const int          yuv_height = 2048;
static const int          yuv_frames = 10; // TO PRELOAD INTO RAM

static const string yuv_out_dec_filepath = "deep10bit_2048x2048_y420_mixed_dec.yuv";
//////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    const int yuv_samples = yuv_width * yuv_height;

    {
        ifstream yuv_file_in;

        yuv_file_in.open(yuv_out_dec_filepath, ios::in | ios::binary);
        if (!yuv_file_in.is_open())
        {
            cout << "Failed: could not open input yuv file" << endl;
            return EXIT_FAILURE;
        }

        vector<vector<uint16_t>> yuv_in_framesY, yuv_in_framesCb, yuv_in_framesCr;
        vector<uint16_t> yuv_out_frame(yuv_samples);
        vector<int> thresholds; // dummy if thresholds are not adaptive

        auto demixer = DepthDemixer(yuv_width, yuv_height, false);

        for (int frame = 0; frame < yuv_frames; frame++)
        {
            cout << "preloading frame " << frame << endl;

            yuv_in_framesY.push_back(vector<uint16_t>(yuv_samples));
            yuv_in_framesCb.push_back(vector<uint16_t>(yuv_samples / 4));
            yuv_in_framesCr.push_back(vector<uint16_t>(yuv_samples / 4));

            yuv_file_in.read(reinterpret_cast<char *>(yuv_in_framesY[frame].data()), yuv_in_framesY[frame].size() * sizeof(uint16_t));
            yuv_file_in.read(reinterpret_cast<char *>(yuv_in_framesCb[frame].data()), yuv_in_framesCb[frame].size() * sizeof(uint16_t));
            yuv_file_in.read(reinterpret_cast<char *>(yuv_in_framesCr[frame].data()), yuv_in_framesCr[frame].size() * sizeof(uint16_t));
        }

        auto start = chrono::high_resolution_clock::now();
        auto stop = chrono::high_resolution_clock::now(); 
        for (int frame = 0; true; frame = (frame + 1) % yuv_frames)
        {
            start = chrono::high_resolution_clock::now(); 
            demixer.ProcessFrame(yuv_in_framesY[frame], yuv_in_framesCb[frame], yuv_in_framesCr[frame], thresholds, yuv_out_frame);
            stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
            cout << "Demixed in " << duration.count() << " microseconds\n";
        }
    }

    return EXIT_SUCCESS;
}