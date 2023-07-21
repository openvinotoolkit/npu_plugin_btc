//
// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache 2.0
//

//

#pragma once
#include <cstdint>
#include <cstring>
#include <istream>
#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cassert>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <functional>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "utils/utils.h"
#include "utils/logger.h"

namespace btc27
{
#define BTC27_NUMALGO                 8
#define BTC27_NUM4KALGO               2

#define BTC27_MAX_DECOMPRESS_FACTOR   5
#define BTC27_MAX_COMPRESS_FACTOR     2

#define BTC27_FILENAME_SIZE           256

class BitCompactor
{
public:

    // Compress Wrap Arguments
    typedef struct btcmpctr_compress_wrap_args_s
    {
        int verbosity{0};       // Verbosity level
        int mixedBlkSize{0};    // Enable mixed block size. 0 -> disabled, 1 -> enabled
        int proc_bin_en{0};     // Enable binning pre-processing mode. 0 -> disabled, 1 -> enabled
        int proc_btmap_en{0};   // Enable bitmap pre-processing mode. 0 -> disabled, 1 -> enabled
        int align{1};           // Align mode. 0 -> byte align, 1 -> 32B align, 2 -> 64B align.
        int dual_encode_en{1};  // Enable dual encoding mode. 0 -> disabled, 1 -> enabled
        int bypass_en{0};       // When set to 1, the compressor will treat all blocks as bypass.
        int minFixedBitLn{3};   // Set minimum fixed-length symbol size in bits (0..7, default 3)
    } btcmpctr_compress_wrap_args_t;

    BitCompactor();
    ~BitCompactor();

    BitCompactor(const BitCompactor &) = delete;
    BitCompactor& operator= (const BitCompactor &) = delete;

    //      #     ######   ###
    //     # #    #     #   #
    //    #   #   #     #   #
    //   #     #  ######    #
    //   #######  #         #
    //   #     #  #         #
    //   #     #  #        ###

    // BTC decoding/decompression
    //  return: 1 - decompression success;
    //          0 - decompression fail;
    int  DecompressWrap(const unsigned char*        src,            // input compressed buffer data
                        unsigned int                srcLen,         // input compressed buffer size
                        unsigned char*              dst,            // output decompressed buffer data
                        unsigned int&               dstLen,         // input decompressed buffer size bound (Max BTC27_MAX_DECOMPRESS_LEN)
                                                                    // output decompressed buffer size result
                        const btcmpctr_compress_wrap_args_t& args   // input decompression configuration args
                        );

    // BTC encoding/compression
    //  return: 1 - compression success;
    //          0 - compression fail;
    int  CompressWrap(  const unsigned char*        src,            // input decompressed buffer data
                        unsigned int                srcLen,         // input decompressed buffer size
                        unsigned char*              dst,            // output compressed buffer data
                        unsigned int&               dstLen,         // input compressed buffer size bound (see GetCompressedSizeBound)
                                                                    // output compressed size result
                        const btcmpctr_compress_wrap_args_t& args   // input compression configuration args
                        );

    // BTC compressed size bound calculation. Call before compression.
    //  return: Worst case compressed buffer size for given decompressed buffer size
    unsigned int GetCompressedSizeBound(unsigned int bufSize        // input decompressed buffer size
                                        ) const;

    // This is a SWIG/numpy integration friendly interface for the decompression function.
    //  return: decompressed buffer size result
    unsigned int DecompressArray(   const unsigned char* const           src,       // input compressed buffer data
                                    unsigned int                         srcLen,    // input compressed buffer size
                                    unsigned char*                       dst,       // output decompressed buffer data
                                    unsigned int                         dstLen,    // input decompressed buffer size bound (Max BTC27_MAX_DECOMPRESS_LEN)
                                    const btcmpctr_compress_wrap_args_t& args       // input decompression configuration args
                               );

    // This is a SWIG/numpy integration friendly interface for the compression function.
    //  return: compressed buffer size result
    unsigned int CompressArray( const unsigned char* const           src,       // input decompressed buffer data
                                unsigned int                         srcLen,    // input decompressed buffer size
                                unsigned char*                       dst,       // output compressed buffer data
                                unsigned int                         dstLen,    // input compressed buffer size bound (see GetCompressedSizeBound)
                                const btcmpctr_compress_wrap_args_t& args       // input compression configuration args
                              );

private:

    typedef unsigned char elem_type ;

    // Struct defining the return type of an algorithm
    //
    typedef struct btcmpctr_algo_args_s
    {
        const unsigned char* inAry;   // Input Buffer to work On.
        unsigned char*       minimum; // Array of bytes to Add or bins
        unsigned char*       bitln;   // Bit Length after the algo is run.
        unsigned char*       residual; // Pre-Processed data to be inserted into the output stream.
                  int        blkSize;  // Current Block Size to work with.
                  int        minFixedBitLn;
                  int*       numSyms;  // Number of Symbols binned.
        unsigned char*       bitmap;   // bit map when replacing highest frequency symbol.
                  int*       numBytes; // Number of bytes in residual that is valid, when bitmap compressed.
    } btcmpctr_algo_args_t;

    // Typedef of the Algo Function pointer.
    typedef void (BitCompactor::*Algo)(btcmpctr_algo_args_t* algoArg);

    // Struct defining the chosen Algorithm and its compressed size
    typedef struct btcmpctr_algo_choice_s
    {
       // Pointer to the chosen Algo function
       Algo chosenAlgo;
       int  cmprsdSize;
       int  algoType; // 0 64B, 1 4K
       int  none; // None chosen, hence uncompressed.
       int  algoHeader;
       int  workingBlkSize;
       int  dual_encode;
    } btcmpctr_algo_choice_t;


    // CompressWrap
    Algo AlgoAry[BTC27_NUMALGO];
    Algo AlgoAry4K[BTC27_NUM4KALGO];

    // Declare Array of Algorithms
    int AlgoAryHeaderOverhead[BTC27_NUMALGO];
    int AlgoAryHeaderOverhead4K[BTC27_NUM4KALGO];

    // Returns ceil(log2(i)), i = [0..256]
    const uint8_t mCeilLog2LUT[257] {
        0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8};

    // Debug support
    int mVerbosityLevel;
    std::stringstream mDebugStr;

    unsigned char btcmpctr_insrt_byte( unsigned char  byte,
                                       unsigned char  bitln,
                                       unsigned int*  outBufLen,
                                       unsigned char* outBuf,
                                       unsigned char  state,
                                       unsigned int*  accum,
                                                int   flush
                                     );

    unsigned char btcmpctr_insrt_hdr(int chosenAlgo,
                                     unsigned char  bitln,
                                     unsigned int*  outBufLen,
                                     unsigned char* outBuf,
                                     unsigned char  state,
                                     unsigned int*  accum,
                                     unsigned char  eofr,
                                     int            workingBlkSize,
                                     int            mixedBlkSize,
                                     int            align
                                    );

    void btcmpctr_calc_bitln(const unsigned char*   residual,
                             unsigned char*         bitln,
                             int                    blkSize,
                             int                    minFixedBitLn
                            );
    void btcmpctr_calc_dual_bitln(const unsigned char* residual,
                                        unsigned char* bitln,
                                                 int   blkSize,
                                        unsigned char* bitmap,
                                                 int*  compressedSize
                                 );
    void btcmpctr_tounsigned(const signed char* inAry,
                             unsigned char*     residual,
                             int                blkSize
                            );
    void btcmpctr_minprdct(
                            btcmpctr_algo_args_t* algoArg
                          );
    void btcmpctr_minSprdct(
                             btcmpctr_algo_args_t* algoArg
                           );
    void btcmpctr_muprdct(
                            btcmpctr_algo_args_t* algoArg
                          );
    void btcmpctr_medprdct(
                            btcmpctr_algo_args_t* algoArg
                          );
    void btcmpctr_noprdct(
                            btcmpctr_algo_args_t* algoArg
                         );
    void btcmpctr_noSprdct(
                            btcmpctr_algo_args_t* algoArg
                         );
    void btcmpctr_binCmpctprdct(
                                btcmpctr_algo_args_t* algoArg
                               );
    void btcmpctr_btMapprdct(
                                btcmpctr_algo_args_t* algoArg
                            );
    void btcmpctr_dummyprdct(
                                btcmpctr_algo_args_t* algoArg
                               );
    void btcmpctr_xtrct_bits(
                       const unsigned char* inBuf,
                             unsigned  int* inBufLen,
                             unsigned char* state,
                             unsigned char* outByte,
                             unsigned char  numBits
                            );
    unsigned char btcmpctr_xtrct_hdr(
                               const unsigned char* inAry,
                                     unsigned  int* inAryPtr,
                                     unsigned char  state,
                                     unsigned char* cmp,
                                     unsigned char* eofr,
                                     unsigned char* algo,
                                     unsigned char* bitln,
                                              int*  blkSize,
                                     unsigned char* bytes_to_add, // 1 or 2 bytes
                                     unsigned char* numSyms,
                                     unsigned int*  numBytes,
                                     unsigned char* bitmap,
                                              int   mixedBlkSize,
                                              int   dual_encode_en,
                                     unsigned char*  dual_encode
                                    );
    unsigned char btcmpctr_xtrct_bytes_wbitmap(
                                         const unsigned char* inAry,
                                               unsigned  int* inAryPtr,
                                               unsigned char  state,
                                               unsigned char  bitln,
                                               unsigned char* outBuf, // Assume OutBuf is BLKSIZE
                                                         int  blkSize,
                                               unsigned char* bitmap
                                             );
    unsigned char btcmpctr_xtrct_bytes(
                                  const unsigned char* inAry,
                                        unsigned  int* inAryPtr,
                                        unsigned char  state,
                                        unsigned char  bitln,
                                        unsigned char* outBuf, // Assume OutBuf is BLKSIZE
                                                  int  blkSize,
                                        unsigned char  mode16
                                      );
    void btcmpctr_tosigned(const unsigned char* inAry,
                                 unsigned char* outBuf
                          );
    void btcmpctr_addByte(
                    const unsigned char* data_to_add,
                          unsigned char  mode16,
                          unsigned char* outBuf
                         );


    void btcmpctr_initAlgosAry(const btcmpctr_compress_wrap_args_t& args);

    unsigned char btcmpctr_getAlgofrmIdx(int idx);

    unsigned char btcmpctr_get4KAlgofrmIdx(int idx);

    btcmpctr_algo_choice_t btcmpctr_ChooseAlgo64B(btcmpctr_algo_args_t* algoArg,
                                                                   int mixedBlkSize,
                                                                   int dual_encode_en
                                                );

    btcmpctr_algo_choice_t btcmpctr_ChooseAlgo4K(btcmpctr_algo_args_t* algoArg,
                                                                  int mixedBlkSize
                                               );

    elem_type getMedianNaive(elem_type arr[], unsigned int blkSize);
};
} // namespace btc27
