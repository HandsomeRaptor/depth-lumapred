# 12-bit Lossless Depth Map Compression using 10-bit HEVC

This project is a POC of using 10-bit intra-only HEVC with some pre-processing for encoding (and respective post-processing after decoding) applied to achieve efficient encoding of 12-bit depth maps.

### But why?

Depth maps usually require sufficient depth resolution to avoid perceptual quality loss (after multi-view reconstruction), which requires depth maps to have high bit depth. However, for multi-view video good decoding performance is difficult to achieve on just the CPU due to having to decode multiple high-def streams at the same time, and hardware decoding is a must. But hardware decoding of 12-bit HEVC streams is practically not present on any consumer-obtainable devices, and 10 bits is almost always not enough for viewing objects at a reasonably close distance.

This is where the need for this algorithm comes in: being able to use the much more prevalent 10-bit HEVC decoders, with an added cost of some additional CPU time dedicated to post-proc can be crucial for multi-view applications on the current market.

### How it works

The preprocessing is very simple in principle:

* Take the lower 10 bits of the 12-bit depth map. Wrap the values with a triangle mapping function to remove discontinuites in regions where values pass the 1024*n threshold. This transformed image is put into the Y channel of the 4:2:0 to-be-encoded 10-bit image. Note that this step introduces ambiguities in the previously mentioned regions.

* Take the upper 2 bits of the 12-bit depth map. Then, for every 2x2 block (corresponding to 4 luma samples and 2 chroma samples in the to-be-encoded image) try to apply the following predictor:

    * If all four samples in the lower bit plane are similar, assume that there is no border in the upper bit plane as well, and write just the two bits in the higher bits (5-6) of the Cb channel.

    * If the four samples in the lower bit plane form two distinct groups, encode the two bits for each of the group in the higher bits (5-8) of the Cb channel.

* If using this predictor results in a non-zero decoding error for this 2x2 block, encode all 8 bits of the higher bit plane in the higher bits (5-8) of the Cb and the Cr channels.

The resulting Cb and Cr channels localize high magnitude changes to regions where the original depth map crosses the 1024*n threshold, while having the rest of the coefficients locally constant. Since the actual values in the chroma channels are located in the higher bits, QP for chroma planes can be increased, increasing the resulting coding gain.


### Some results

Using this algorithm to encode depth maps of synthetically generated 3D models provided a reduction in BD-rate of 26.6% compared to using 10-bit HEVC, and an increase of 9.8% compared to Monochrome 12 HEVC. However, a more important point is the performance: on a Raspberry Pi 4 Model B, FFmpeg decoded 1 frame of 12-bit HEVC in about 243ms, whereas a naive implementation of this method runs the post-processing in 26ms (both tests were conducted with 2 decoding threads, where post-proc for this algorithm was parallelized by processing the top/bottom halves of the frame separately). This proves that even with minor to no optimization effort, using post-processing actually allows to decode 12-bit depth maps on regular embedded hardware in real time. The decoding performance can be further improved by implementing the post-proc with SIMD, or possibly using a fragment shader.
