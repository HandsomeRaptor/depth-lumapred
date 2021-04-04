#include <mixer.h>
#include <demixer.h>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////
static const string  yuv_in_filepath = "/home/ivan/Videos/luyu97/deep_12bit_y400_2048x2048.yuv";
static const int           yuv_width = 2048;
static const int          yuv_height = 2048;
static const int          yuv_frames = 97;
static const bool   use_adaptive_thr = true;

static const string     yuv_out_filepath = "deep10bit_2048x2048_y420_mixed.yuv";
static const string yuv_out_enc_filepath = "deep10bit_2048x2048_y420_mixed_enc.h265";
static const string yuv_out_dec_filepath = "deep10bit_2048x2048_y420_mixed_dec.yuv";
static const string yuv_out_flt_filepath = "deep12bit_2048x2048_y400_decoded.yuv";
//////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char const *argv[])
{
    const int yuv_samples = yuv_width * yuv_height;
    const int yuv_bytes16 = yuv_samples * sizeof(uint16_t);

    vector<vector<int>> thresholds;

    {
        ifstream yuv_file_in;
        ofstream yuv_file_out;

        yuv_file_in.open(yuv_in_filepath, ios::in | ios::binary);
        if (!yuv_file_in.is_open())
        {
            cout << "Failed: could not open input yuv file" << endl;
            return EXIT_FAILURE;
        }

        yuv_file_out.open(yuv_out_filepath, ios::out | ios::binary);
        if (!yuv_file_in.is_open())
        {
            cout << "Failed: could not open output yuv file" << endl;
            return EXIT_FAILURE;
        }

        vector<uint16_t> yuv_in_frame(yuv_samples);
        vector<uint16_t> yuv_outY(yuv_samples);
        vector<uint16_t> yuv_outCb(yuv_samples / 4);
        vector<uint16_t> yuv_outCr(yuv_samples / 4);
        std::vector<int> thresh;

        auto mixer = DepthMixer(yuv_width, yuv_height, use_adaptive_thr);

        for (int frame = 0; frame < yuv_frames; frame++)
        {
            cout << "mixing frame " << frame << endl;
            yuv_file_in.read(reinterpret_cast<char *>(yuv_in_frame.data()), yuv_bytes16);

            mixer.ProcessFrame(yuv_in_frame, yuv_outY, yuv_outCb, yuv_outCr, thresh);

            yuv_file_out.write(reinterpret_cast<const char *>(yuv_outY.data()), yuv_outY.size() * sizeof(uint16_t));
            yuv_file_out.write(reinterpret_cast<const char *>(yuv_outCb.data()), yuv_outCb.size() * sizeof(uint16_t));
            yuv_file_out.write(reinterpret_cast<const char *>(yuv_outCr.data()), yuv_outCr.size() * sizeof(uint16_t));
            thresholds.push_back(thresh);
        }
    }

    string h265params = "tskip=1:cbqpoffs=-1:crqpoffs=-1:qp=16:idr-recovery-sei=0:aud=1:single_sei=1:slices=1:b-adapt=0:no-sao=1:single_sei=1:no-info=1:no-strong-intra-smoothing=1:deblock=-1:psy-rd=0:intra-refresh=1:keyint=1:ref=1:no-open-gop=1:bframes=0:b-intra=0:rd-refine=1:no-psy-rdoq=1";
    string ffmpeg_enc_cmdline = "ffmpeg -nostdin -hide_banner -y -framerate 1 -pix_fmt yuv420p10le -f rawvideo -s:v " + to_string(yuv_width) + "x" + to_string(yuv_height) + " -r 1 -i " + yuv_out_filepath + " -frames:v " + to_string(yuv_frames) + " -c:v libx265 -threads 4 -tune fastdecode -preset veryslow -an -tag:v hvc1 -flags2:v +local_header -color_primaries 1 -color_trc 1 -colorspace 1 -pix_fmt yuv420p10le -x265-params " + h265params + " -dst_range 1 " + yuv_out_enc_filepath;
    string ffmpeg_dec_cmdline = "ffmpeg -nostdin -hide_banner -y -f hevc -i " + yuv_out_enc_filepath + " -f rawvideo " + yuv_out_dec_filepath;
    system(ffmpeg_enc_cmdline.c_str());
    system(ffmpeg_dec_cmdline.c_str());

    {
        ifstream yuv_file_in;
        ofstream yuv_file_out;

        yuv_file_in.open(yuv_out_dec_filepath, ios::in | ios::binary);
        if (!yuv_file_in.is_open())
        {
            cout << "Failed: could not open input yuv file" << endl;
            return EXIT_FAILURE;
        }

        yuv_file_out.open(yuv_out_flt_filepath, ios::out | ios::binary);
        if (!yuv_file_in.is_open())
        {
            cout << "Failed: could not open output yuv file" << endl;
            return EXIT_FAILURE;
        }

        vector<uint16_t> yuv_in_frameY(yuv_samples), yuv_in_frameCb(yuv_samples / 4), yuv_in_frameCr(yuv_samples / 4);
        vector<uint16_t> yuv_out_frame(yuv_samples);

        auto demixer = DepthDemixer(yuv_width, yuv_height, use_adaptive_thr);

        for (int frame = 0; frame < yuv_frames; frame++)
        {
            cout << "demixing frame " << frame << endl;
            yuv_file_in.read(reinterpret_cast<char *>(yuv_in_frameY.data()), yuv_in_frameY.size() * sizeof(uint16_t));
            yuv_file_in.read(reinterpret_cast<char *>(yuv_in_frameCb.data()), yuv_in_frameCb.size() * sizeof(uint16_t));
            yuv_file_in.read(reinterpret_cast<char *>(yuv_in_frameCr.data()), yuv_in_frameCr.size() * sizeof(uint16_t));

            demixer.ProcessFrame(yuv_in_frameY, yuv_in_frameCb, yuv_in_frameCr, thresholds[frame], yuv_out_frame);

            yuv_file_out.write(reinterpret_cast<const char *>(yuv_out_frame.data()), yuv_out_frame.size() * sizeof(uint16_t));
        }
    }

    return EXIT_SUCCESS;
}