//
// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache 2.0
//

//

// C++ class built around the C methods in the original BitCompactor model
//

#include "bitCompactor.h"

namespace btc27
{
// Logger macros
#define BTC_REPORT_INFO(verbosity,lvl,message)  if (lvl <= verbosity) { REPORT_INFO("BitCompactor", message, Logger::Verbosity::HIGH); }
#define BTC_REPORT_ERROR(message) { REPORT_ERROR("BitCompactor", message); }

// Block Size
#define BLKSIZE 64
#define BIGBLKSIZE 4096
// Dual Length Mode
// Indicates if Bit length of the compressed data
// should be included in the header
#define DL_INC_BL
// ALGO
#define BITC_ALG_NONE 16
#define LFTSHFTPROC 5
#define BINEXPPROC 4
#define BTEXPPROC 6
#define ADDPROC 3
#define SIGNSHFTADDPROC 2
#define NOPROC 0
#define SIGNSHFTPROC 1

#define EOFR 0
#define CMPRSD 3
#define UNCMPRSD 2
#define LASTBLK 1
// Header bit positions index
#define BITLN 5
#define ALGO 2

#define INCR_STATE_UC(state,len)  (*state)++; if(*state == 8) { *state = 0; (*len)++; }
#define ELEM_SWAP(a,b) { register elem_type t=(a);(a)=(b);(b)=t; }

//-----------------------------------------------------
//64B block size related constants
//-----------------------------------------------------
// Count defined by BTC27_NUMALGO
#define MINPRDCT_IDX 0
#define MINSPRDCT_IDX 1
#define MUPRDCT_IDX 2
#define NOPRDCT_IDX 3
#define NOSPRDCT_IDX 4
#define MEDPRDCT_IDX 5
#define BINCMPCT_IDX 6
#define BTMAP_IDX 7

#define MAXSYMS 16
#define NUMSYMSBL 4
//-----------------------------------------------------
// 4K Block Size related constants
//-----------------------------------------------------
// Count defined by NUM4KALGO
#define BINCMPCT4K_IDX 0
#define BTMAP4K_IDX 1

#define MAXSYMS4K 64
#define NUMSYMSBL4K 6

//-----------------------------------------------------

// Alignment
#define DEFAULT_ALIGN 1
// 1 --> 32B Alignment
// 2 --> 64B Alignment
// 0 --> No  Alignment

// Function Declarations

BitCompactor::BitCompactor() :
        mVerbosityLevel(0)
{
}

BitCompactor::~BitCompactor()
{
}

unsigned char BitCompactor::btcmpctr_insrt_byte( unsigned char  byte,
                                   unsigned char  bitln,
                                   unsigned int*  outBufLen,
                                   unsigned char* outBuf,
                                   unsigned char  state,
                                   unsigned int*  accum,
                                            int   flush
                                 )
{
    unsigned char mask, thisBit, rem;
    // Use 32bit operations to insert a byte into the accumulator
    // Once 32bits are accumulated, push 4bytes to the outBuf
    // and increment the outBufLen.
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "State = "<< std::to_string((int)state) << ", BitLen = "<< std::to_string(bitln) <<", Byte = "<< std::to_string(byte) <<", outByfLen = " << std::to_string(*outBufLen);
    BTC_REPORT_INFO(mVerbosityLevel,11,mDebugStr.str().c_str());
    #endif
    if (flush) {
        // Depending on state write the correct number of bytes to
        // outBuf and increment outBufLen
        if (state != 0) {
            double numbytesf = state/8.0;
            int numBytes = ceil(numbytesf);
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Flushing numBytes = " << std::to_string(numBytes);
            BTC_REPORT_INFO(mVerbosityLevel,11,mDebugStr.str().c_str());
            #endif
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Accum = " << std::to_string(*accum);
            BTC_REPORT_INFO(mVerbosityLevel,11,mDebugStr.str().c_str());
            #endif
            for(int i = 0; i < numBytes; i++) {
                #ifdef __BTCMPCTR__EN_DBG__
                mDebugStr.str(""); mDebugStr << "Flushing Byte = " << std::to_string((unsigned char)(*accum));
                BTC_REPORT_INFO(mVerbosityLevel,11,mDebugStr.str().c_str());
                #endif
                *(outBuf + *outBufLen + i) = (unsigned char)(*accum);
                (*accum) >>= 8;
            }
                (*outBufLen) += numBytes;
            state = 0;
        }

    } else {
        // State indicates the number of bits valid in accum.
        // First see if based on state and bitln we will cross
        // over 32bits.
        if( (state + bitln) > 32) {
            // Need to first insert 32-state bits first from byte and
            // then insert the rest.
            rem = 32 - state;
            mask = ~(0xFF << rem);
            thisBit = byte & mask;
            *accum  |= (thisBit << state);
            *((unsigned int *)(outBuf+ *outBufLen)) = *accum;
            (*outBufLen) += 4;
            state = 0;
            *accum = 0;
            // Remaining bits
            byte = byte >> rem;
            rem = bitln - rem;
            mask = ~(0xFF << rem);
            thisBit = byte & mask;
            *accum  |= (thisBit << state);
            state += rem;
        } else {
            // Can insert bitln number of bits into accum
            mask    = ~(0xFF << bitln); // This should give us correct bit values from byte.
            thisBit = byte & mask;
            // Leftshift this by state
            *accum  |= (thisBit << state);
            state += bitln;
            if(state == 32) {
                *((unsigned int*)(outBuf + *outBufLen)) = *accum;
                (*outBufLen) += 4;
                state = 0;
                *accum = 0;
            }
        }
    }

    //for(int j = 0; j < 8; j++) {
    //    if(j < bitln) {
    //        mask    = 1 << j;
    //        thisBit = byte & mask;
    //        thisBit = thisBit >> j;
    //        mask    = thisBit << state;
    //        outBuf[*outBufLen] |= mask;
    //        INCR_STATE(state,outBufLen,outBuf)
    //        mDebugStr.str(""); mDebugStr << "State = " << std::to_string((int)state) << ", Len = " << std::to_string(*outBufLen) << ", Bit = " << std::to_string(thisBit);
    //        BTC_REPORT_INFO(mVerbosityLevel,6,mDebugStr.str().c_str())
    //    }
    //}
    return state;
}


// Insert header into the output Buffer.
unsigned char BitCompactor::btcmpctr_insrt_hdr(int chosenAlgo,
                                 unsigned char  bitln,
                                 unsigned int*  outBufLen,
                                 unsigned char* outBuf,
                                 unsigned char  state,
                                 unsigned int*  accum,
                                 unsigned char  eofr,
                                 int            workingBlkSize,
                                 int            mixedBlkSize,
                                 int            align
                                )
{
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "Inserting Header, outBuf Length = " << std::to_string(*outBufLen);
    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
    #endif
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "Chosen Algo = " << std::to_string(chosenAlgo);
    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
    #endif
    // Header includes the following fields
    // If eofr, then 2 bit is inserted witha value of '10'
    // else if no compression is done, then 1 bits are inserted '0'
    // else 8 bits are inserted '<3 bits bitln><3 bits algo><01>' --> [7:0] if algo != 16bit modes
    bool is16bitmode = 0;
    bool is4K        = (workingBlkSize == BIGBLKSIZE);
    bool isLastBlk    = (workingBlkSize != BLKSIZE) & !is4K;

    if (eofr) {
        //just insert the no compression bit '0' and return.
        state = btcmpctr_insrt_byte(EOFR,2,outBufLen,outBuf,state,accum,0);
        // Once the SKIP header is inserted, check the alignment requirement.
        if ( (align == 1) || (align == 2) ) {
            // 32B alignment
            // OutBufLen and State combined together will tell the current alignment.
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Aligning to = " << std::to_string(align);
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif
            double numbytesf = state/8.0;
            unsigned int bytesinBuf = (*outBufLen + (unsigned int)ceil(numbytesf));
            unsigned int alignB = ((align == 1) ? 32 : 64);
            unsigned int numBytesToInsert = ((bytesinBuf % alignB) == 0) ? 0 : alignB - (bytesinBuf % alignB);
            unsigned int numBitsToInsert = ((state % 8) == 0) ? 0 : 8-(state %8);
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "numBits = " << std::to_string(numBitsToInsert) << ", numBytes = " << std::to_string(numBytesToInsert) << ", outBufLen = " << std::to_string(*outBufLen) << ", state = " << std::to_string(state);
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif
            state = btcmpctr_insrt_byte(0,numBitsToInsert,outBufLen,outBuf,state,accum,0);
            for(unsigned int i = 0; i < numBytesToInsert; i++) {
                state = btcmpctr_insrt_byte(0,8,outBufLen,outBuf,state,accum,0);
            }
        }
        // Below will ensure byte alignment.
        state = btcmpctr_insrt_byte(EOFR,2,outBufLen,outBuf,state,accum,1); // Flush
        return state;
    } else  if (chosenAlgo == BITC_ALG_NONE) {
        if (isLastBlk) {
            state = btcmpctr_insrt_byte(LASTBLK,2,outBufLen,outBuf,state,accum,0);
            // Insert 6 bits of block Size in bytes.
            state = btcmpctr_insrt_byte(workingBlkSize,6,outBufLen,outBuf,state,accum,0);
        } else {
            state = btcmpctr_insrt_byte(UNCMPRSD,2,outBufLen,outBuf,state,accum,0);
            if(mixedBlkSize) {
                if (is4K) {
                    state = btcmpctr_insrt_byte(1,2,outBufLen,outBuf,state,accum,0);
                } else {
                    state = btcmpctr_insrt_byte(0,2,outBufLen,outBuf,state,accum,0);
                }
            }
        }
        return state;
    } else {
        // We are guarantteed to come in here when workingBlkSize != 64
        // First form the header byte and then use a loop to insert it into
        // the output buffer.
        //Algo is 3 bits, hence mask off the rest of the bits from chosen Algo.
        state = btcmpctr_insrt_byte(CMPRSD,2,outBufLen,outBuf,state,accum,0);
        if(mixedBlkSize) {
            if (is4K) {
                state = btcmpctr_insrt_byte(1,2,outBufLen,outBuf,state,accum,0);
            } else {
                state = btcmpctr_insrt_byte(0,2,outBufLen,outBuf,state,accum,0);
            }
        }
        state = btcmpctr_insrt_byte(chosenAlgo,3,outBufLen,outBuf,state,accum,0);
        //Bit Lenght is in the range of 1 - 8. it will get encoded to 0 - 7, with 0 indicating 8 bits.
        if ((bitln == 8) && !is16bitmode) { bitln = 0; } // Only if in 8 bit mode, in 16 bit mode this chack should change to == 16. TODO
        if ((bitln == 16) && is16bitmode) { bitln = 0; } // Only if in 8 bit mode, in 16 bit mode this chack should change to == 16. TODO
        state = btcmpctr_insrt_byte(bitln,3,outBufLen,outBuf,state,accum,0);

        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Inserting Header, outBuf Length = " << std::to_string(*outBufLen);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        return state;
    }
}

// Calculate minimum number of bits needed to represent all the numbers
void BitCompactor::btcmpctr_calc_bitln( const unsigned char* residual,
                                        unsigned char* bitln,
                                        int            blkSize,
                                        int            minFixedBitLn
                                    )
{
    uint16_t maximum = 0;
    for(int i = 0; i < blkSize; i++) { // TODO : Get residual Length
        if(residual[i] > maximum)
            maximum = residual[i];
    }
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In calc bitln, maximum is " << std::to_string(maximum);
    BTC_REPORT_INFO(mVerbosityLevel,8,mDebugStr.str().c_str());
    #endif
    // Find number of bits needed to encode the maximu.
    // Use (maximum + 1) in the log2. This helps when maximum is power of 2.
    *bitln = (maximum == 0) ? 1 : BitCompactor::mCeilLog2LUT[(maximum+1)];
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In calc bitln, bitln is " << std::to_string(*bitln) << ", minFixedBitLn is " << std::to_string(*minFixedBitLn);
    BTC_REPORT_INFO(mVerbosityLevel,8,mDebugStr.str().c_str());
    #endif
    if ( *bitln < minFixedBitLn )
    {
        *bitln = minFixedBitLn;
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "In calc bitln, bitln is " << std::to_string(*bitln) << ", limited by minFixedBitLn " << std::to_string(*minFixedBitLn);
        BTC_REPORT_INFO(mVerbosityLevel,8,mDebugStr.str().c_str());
        #endif
    }
}

// A function that takes a buffer (residue) and calculates the dual length encoding.
// One of the lengths could be < 8 and is indicated in a 0 in the bitmap. A 1 in the
// bitmap indicates 8bit symbols.
void BitCompactor::btcmpctr_calc_dual_bitln( const unsigned char*   residual,
                                             unsigned char*         bitln,
                                             int                    blkSize,
                                             unsigned char*         bitmap,
                                             int*                   compressedSize
                             )
{
    // First calculate/bin the bitlengths
    unsigned char bin[9]; // need to store 1 - 8 bitln
    unsigned char symbolBitln[BLKSIZE]; // Assume this is used only for 64B blocks.
    unsigned char sBitln;
    int cSize[9];
    unsigned char cumSumL,cumSumH;

    for(int i = 1; i < 9; i++) {
        bin[i] = 0;
    }
    for(int i = 0; i < blkSize; i++) {
        // Calculate the number of bits needed to encode the symbol
        sBitln = (*(residual+i) == 0) ? 1 : BitCompactor::mCeilLog2LUT[(*(residual+i)+1)];
        // Increment the bin for that bitlength.
        bin[sBitln]++;
        // Store the bitlenght for the i th symbol.
        symbolBitln[i] = sBitln;
    }
    #ifdef __BTCMPCTR__EN_DBG__
    for(int i = 1; i < 9; i++) {
        mDebugStr.str(""); mDebugStr << "Num symbols with length " << std::to_string(i)<< " is " << std::to_string(bin[i]);
        BTC_REPORT_INFO(mVerbosityLevel,8,mDebugStr.str().c_str());
    }
    #endif
    // For each of the bins, calculate the compressed Size.
    // And find the bitln that results in the minimum compressed Size.
    for (int i = 1; i < 9; i++) {
        cumSumL = 0;
        for(int j = 1; j <= i; j++) {
            cumSumL += bin[j];
        }
        cumSumH = 0;
        for(int j = i+1; j < 9; j++) {
            cumSumH += bin[j];
        }
        cSize[i] = cumSumL*i + cumSumH*8;
        // Find the minimum compressed Size.
        if (i == 1) {
            *compressedSize = cSize[i];
            *bitln          = i;
        } else {
            if (cSize[i] < *compressedSize) {
                *compressedSize = cSize[i];
                *bitln          = i;
            }
        }
    }
    // *bitln contains the chosen bitln < 8.
    // Calculate the bitmap
    uint32_t shortSymbolCount = 0;
    uint32_t longSymbolCount = 0;
    for(int i = 0; i < blkSize; i++) {
       if (symbolBitln[i] <= *bitln) {
           bitmap[i] = 0;
           shortSymbolCount++;
       } else {
           bitmap[i] = 1;
           longSymbolCount++;
       }
    }

    // Dual-length encoding only makes sense if both symbol lengths are actually needed
    // In the case where we find we can use the short length for all symbols, we could
    // end up with a block made up of 64 1-bit symbols. This might be the unwanted result.
    // 
    if ( longSymbolCount == 0 )
    {
        // Remedy situation with compressed blocks made up entirely of 'short' symbols
        // by forcing at least one long (8-bit) symbol at start of bitmap.
        // Make adjustment to compressedSize and to bitmap[0].
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "calc_dual_bitln: 8-bit Symbols: 1 (forced); " << std::to_string(*bitln) << "-bit Symbols: " << shortSymbolCount-1;
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif

        // account for the fact that using the 8-bit length for the first symbol will require more bits
        *compressedSize = *compressedSize + (8-(*bitln));
        // set element zero in bitmap to 1 (i.e. force long/8-bit symbol length)
        bitmap[0] = 1;
    }
    #ifdef __BTCMPCTR__EN_DBG__
    else
    {
        // We have at least one 'long' symbol in this block, which is fine
        mDebugStr.str(""); mDebugStr << "calc_dual_bitln: 8-bit Symbols: " << std::to_string(longSymbolCount) << "; "<< std::to_string(*bitln) << "-bit Symbols: " << std::to_string(shortSymbolCount);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());

    }
    #endif
}

// Convert a signed number to unsigned by storing the sign bit in LSB
//
void BitCompactor::btcmpctr_tounsigned(const signed char* inAry,
                                        unsigned char*    residual,
                                        int               blkSize
                                       )
{
    //if number is <0 then lose the MSB and shift a 1 to LSB.
    for(int i = 0; i < blkSize; i++) { // TODO : inAry size
        if(inAry[i] < 0) {
            residual[i] = (unsigned char) (~inAry[i] << 1) | 0x01;
        } else {
            residual[i] = (unsigned char) (inAry[i] << 1);
        }
    }
}
// Do Min Predict algo on a buffer, return minimum number and the bitln. These are pointers given to the function.
void BitCompactor::btcmpctr_minprdct(
                        btcmpctr_algo_args_t* algoArg
                      )
{
    *(algoArg->minimum) = 255;

    for(int i = 0; i < algoArg->blkSize; i++) {// TODO : inAry size
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "In minprdct, inAry["<< std::to_string(i) << "] is "<< std::to_string(algoArg->inAry[i]);
        BTC_REPORT_INFO(mVerbosityLevel,12,mDebugStr.str().c_str());
        #endif
        if(algoArg->inAry[i] < *algoArg->minimum) {
            *algoArg->minimum = algoArg->inAry[i];
        }
    }
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In minprdct, minimum is " << std::to_string(*algoArg->minimum);
    BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
    #endif
    // Subtract the minimum from the array and make a copy of the array.
    for(int i = 0; i < algoArg->blkSize; i++) {
        algoArg->residual[i] = algoArg->inAry[i] - *algoArg->minimum;
    }
    // Find Bit Length
    btcmpctr_calc_bitln(algoArg->residual,algoArg->bitln,algoArg->blkSize,algoArg->minFixedBitLn);
}

// Do Min Signed Predict algo on a buffer, return minimum number and the bitln. These are pointers given to the function.
void BitCompactor::btcmpctr_minSprdct(
                         btcmpctr_algo_args_t* algoArg
                       )
{
    signed char  residualS[BLKSIZE], inSAry[BLKSIZE];
    signed char minS;
    for(int i = 0; i < algoArg->blkSize; i++) {// TODO : inAry size
        inSAry[i] = (signed char )algoArg->inAry[i];
    }

    *algoArg->minimum = 255;
    minS = 127;

    for(int i = 0; i < algoArg->blkSize; i++) {// TODO : inAry size
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "In minprdct, inSAry[" << std::to_string(i) <<"] is "<< std::to_string(inSAry[i]);
        BTC_REPORT_INFO(mVerbosityLevel,12,mDebugStr.str().c_str());
        #endif
        if(inSAry[i] < minS) {
            minS = inSAry[i];
        }
    }
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In minSprdct, minimum is " << std::to_string(minS);
    BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
    #endif
    *algoArg->minimum = (unsigned char)minS;
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In minSprdct, minimum is " << std::to_string(*algoArg->minimum);
    BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
    #endif

    // Subtract the minimum from the array and make a copy of the array.
    for(int i = 0; i < algoArg->blkSize; i++) {
        residualS[i] = inSAry[i] - minS;
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "In minSprdct, residual[" << std::to_string(i) << "] = " << std::to_string(residualS[i]);
        BTC_REPORT_INFO(mVerbosityLevel,12,mDebugStr.str().c_str());
        #endif
    }
    //Convert to Unsigned
    btcmpctr_tounsigned(residualS, algoArg->residual,algoArg->blkSize);
    // Find Bit Length
    btcmpctr_calc_bitln(algoArg->residual,algoArg->bitln,algoArg->blkSize,algoArg->minFixedBitLn);
}
// Do Mean Signed Predict algo on a buffer, return minimum number and the bitln. These are pointers given to the function.
void BitCompactor::btcmpctr_muprdct(
                        btcmpctr_algo_args_t* algoArg
                      )
{
    signed char* inSAry = (signed char *)algoArg->inAry;
    signed char  residualS[BLKSIZE];
    signed char muS;
    double      sum,mud;

    *algoArg->minimum = 0;
    muS = 0;
    sum = 0;
    for(int i = 0; i < algoArg->blkSize; i++) {
            sum += inSAry[i];
    }
    mud = sum/algoArg->blkSize;
    muS = (signed char)(round(mud));

    // Subtract the minimum from the array and make a copy of the array.
    for(int i = 0; i < algoArg->blkSize; i++) {
        residualS[i] = inSAry[i] - muS;
    }
    *algoArg->minimum = (unsigned char)muS;
    //Convert to Unsigned
    btcmpctr_tounsigned(residualS, algoArg->residual,algoArg->blkSize);
    // Find Bit Length
    btcmpctr_calc_bitln(algoArg->residual,algoArg->bitln,algoArg->blkSize,algoArg->minFixedBitLn);
}
// Do Median Signed Predict algo on a buffer, return minimum number and the bitln. These are pointers given to the function.
// http://ndevilla.free.fr/median/median/src/quickselect.c

void BitCompactor::btcmpctr_medprdct(
                        btcmpctr_algo_args_t* algoArg
                      )
{
    signed char* inSAry = (signed char *)algoArg->inAry;
    signed char  residualS[BLKSIZE];
    signed char medianS;
    for(int i = 0; i < algoArg->blkSize; i++) {
        algoArg->residual[i] = algoArg->inAry[i];
    }
    // Sort the array, Even length => median = arr[blkSize/2 -1], Odd length => median = arr[blkSize/2]
    medianS = (signed char) getMedianNaive((unsigned char *)algoArg->residual,algoArg->blkSize);

    // Subtract the minimum from the array and make a copy of the array.
    for(int i = 0; i < algoArg->blkSize; i++) {
        residualS[i] = inSAry[i] - medianS;
    }
    *algoArg->minimum = (unsigned char)medianS;
    //Convert to Unsigned
    btcmpctr_tounsigned(residualS, algoArg->residual,algoArg->blkSize);
    // Find Bit Length
    btcmpctr_calc_bitln(algoArg->residual,algoArg->bitln,algoArg->blkSize,algoArg->minFixedBitLn);
}
// No Predict, just look at the maximum in the array
void BitCompactor::btcmpctr_noprdct(
                        btcmpctr_algo_args_t* algoArg
                     )
{
    // Copy inAry to residual
    for(int i = 0; i< algoArg->blkSize; i++) {
        algoArg->residual[i] = algoArg->inAry[i];
    }
    btcmpctr_calc_bitln(algoArg->inAry,algoArg->bitln,algoArg->blkSize,algoArg->minFixedBitLn);
}
// No Sign Predict. Store
void BitCompactor::btcmpctr_noSprdct(
                        btcmpctr_algo_args_t* algoArg
                     )
{
    // First convert to unsigned representation by storing MSB in LSB.
    btcmpctr_tounsigned((signed char*) algoArg->inAry, algoArg->residual,algoArg->blkSize);
    btcmpctr_calc_bitln(algoArg->residual,algoArg->bitln,algoArg->blkSize,algoArg->minFixedBitLn);
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In NoSPrdct, bitln is " << std::to_string(*algoArg->bitln);
    BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
    #endif
}

// BIN the bytes in the block and check if there are <= 16 unique symbols
// If yes, return the bitln and residual. Residual is the bin number of the
// byte in the block.
// Also need to return the array of bytes to be inserted after the header. Can be stored in *minimum.
// Need a way to return the number of symbols valid. So that the correct number of bytes can be
// inserted.
void BitCompactor::btcmpctr_binCmpctprdct(
                            btcmpctr_algo_args_t* algoArg
                           )
{
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In binCmpctprdct";
    BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
    #endif
    int inBins   = 0;
    int allinBin = 1;
    int maxsyms;
    maxsyms = (algoArg->blkSize > 64) ? MAXSYMS4K : MAXSYMS;

    *algoArg->numSyms     = 1;
    algoArg->minimum[0]   = algoArg->inAry[0];
    algoArg->residual[0]  = 0;
    for(int i = 1; i < algoArg->blkSize; i++) {
        inBins = 0;
        for(int k = 0; k < *algoArg->numSyms; k++) {
            if(algoArg->inAry[i] == algoArg->minimum[k]) {
                inBins = 1;
                algoArg->residual[i] = k;
                break;
            }
        }
        if (inBins == 0) {
            if(*algoArg->numSyms == maxsyms) {
                allinBin = 0;
                break;
            }
            algoArg->minimum[*algoArg->numSyms] = algoArg->inAry[i];
            algoArg->residual[i] = *algoArg->numSyms;
           (*algoArg->numSyms)++;
        }
    }
    // Check if all symbols are in < MAXSYMS bins
    if(allinBin == 0) {
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "More than MAXSYMS symbols, returning";
        BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
        #endif
        *algoArg->bitln = 8;
        for(int i = 0; i < algoArg->blkSize; i++) {
            algoArg->residual[i] = algoArg->inAry[i];
        }
    } else {
        // Resudaul should already have the correct bin
        // index in them.
        btcmpctr_calc_bitln(algoArg->residual,algoArg->bitln,algoArg->blkSize,algoArg->minFixedBitLn);
    }
}
// Bitmap Proc
// Bit map preproccing, find the top bin symbol and removes that symbol
// while preserving a bitmap showing the location of the removed symbol.
void BitCompactor::btcmpctr_btMapprdct(
                            btcmpctr_algo_args_t* algoArg
                        )
{
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In btMapprdct";
    BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
    #endif
    int maxIdx     = 0;
    int maxSymFreq = 0;
    int symbFreq[256];
    for(int i = 0; i < 256; i++) {
        symbFreq[i] = 0;
    }

    // Bin the Block and find the maximum frequency symbol.
    for(int i = 0; i < algoArg->blkSize; i++) {
        symbFreq[algoArg->inAry[i]]++;
    }
    // Find Max Index.
    for(int i = 0; i < 256; i++) {
        if(symbFreq[i] > maxSymFreq) {
            maxIdx = i;
            maxSymFreq = symbFreq[i];
        }
    }
    // Max Frequency Symbol is maxIdx.
    // Find this maxIdx in the inAry and create a bitmap.
    int cnt = 0;
    for(int i =0; i < algoArg->blkSize; i++) {
        if(algoArg->inAry[i] == maxIdx) {
            algoArg->bitmap[i] = 0;
        } else {
            algoArg->bitmap[i] = 1;
            algoArg->residual[cnt++] = algoArg->inAry[i];
        }
    }
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "In btMapprdct maxFreqSymb = " << std::hex << std::to_string(maxIdx) <<", numBytes = " << std::dec << std::to_string(cnt);
    BTC_REPORT_INFO(mVerbosityLevel,7,mDebugStr.str().c_str());
    #endif
    *algoArg->minimum  = maxIdx;
    *algoArg->bitln    = 1;
    *algoArg->numBytes = cnt;

}
// Dummy Proc
void BitCompactor::btcmpctr_dummyprdct(
                            btcmpctr_algo_args_t* algoArg
                           )
{
    // Choose results which show worse performance.
    *(algoArg->bitln) = 8;
    *algoArg->numBytes = algoArg->blkSize;
    for(int i = 0; i < algoArg->blkSize; i++) {
        algoArg->residual[i] = algoArg->inAry[i];
    }
}
// Uncompression function
// Given an inBuf, Current inBuf pointer, and a state, extract specified number of bytes in outByte.
void BitCompactor::btcmpctr_xtrct_bits(
                   const unsigned char* inBuf,
                         unsigned  int* inBufLen,
                         unsigned char* state,
                         unsigned char* outByte,
                         unsigned char  numBits
                        )
{
    unsigned char mask, thisBit;

    *outByte = 0;
    for(int i = 0; i< numBits; i++) {
        mask    = 1 << *state;
        thisBit = (inBuf[*inBufLen] & mask) >> *state; // Needed bit is in LSB of thisBit now.
           *outByte |= (thisBit << i);
        INCR_STATE_UC(state,inBufLen)
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Extracting bits, state =" << std::to_string((int)*state)<< ", srcPtr = " << std::to_string(*inBufLen)<<", Bit = " << std::to_string(thisBit);
        BTC_REPORT_INFO(mVerbosityLevel,6,mDebugStr.str().c_str());
        #endif
    }
}

// Extract header and give out, cmp, algo, bitln, eof, 8 or 16 bit to add.
unsigned char BitCompactor::btcmpctr_xtrct_hdr(
                           const unsigned char* inAry,
                                 unsigned int*  inAryPtr,
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
                                )
{
    unsigned char header = 0;
    // Assign default
    *eofr = 0;
    *cmp  = 0;
    *algo = BITC_ALG_NONE;
    *bitln = 8;
    *bytes_to_add = 0;
    *blkSize = 0;
    *numSyms = 0;
    *dual_encode = 0;
    unsigned char blk;
    // First extract 2bits
    btcmpctr_xtrct_bits(inAry,inAryPtr,&state,&header,2);
    if(header == EOFR) {
        // EOFR
        *eofr = 1;
        return state;
    } else if (header == CMPRSD) {
        // Compressed block
        // More header bits to extract.
        *cmp = 1;
        if(mixedBlkSize) {
            // First extract 2bits block Size
            blk = 0;
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,&blk,2);
            if(blk == 1) {
                *blkSize = BIGBLKSIZE;
            } else {
                *blkSize = BLKSIZE;
            }
        } else {
            *blkSize = BLKSIZE;
        }
    } else if (header == UNCMPRSD) {
        // Uncompressed block
        if(mixedBlkSize) {
            // First extract 2bits block Size
            blk = 0;
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,&blk,2);
            if(blk == 1) {
                *blkSize = BIGBLKSIZE;
            } else {
                *blkSize = BLKSIZE;
            }
        } else {
            *blkSize = BLKSIZE;
        }
        return state;
    } else {
        // Last block with bitstream length specified
        // Extract the next 6 bits, which holds the block Size in bytes.
        btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(unsigned char *)blkSize,6);
        return state;
    }
    // Next extract Algo.
    btcmpctr_xtrct_bits(inAry,inAryPtr,&state,algo,3); // TODO Make Algo bits scalable.
    // Extract 3 bits of bitln
    btcmpctr_xtrct_bits(inAry,inAryPtr,&state,bitln,3);
    // If dual encode is enabled extract 2 bits to decode dual_encode.
    if(dual_encode_en) {
        btcmpctr_xtrct_bits(inAry,inAryPtr,&state,dual_encode,2);
        #ifdef DL_INC_BL
        if(*dual_encode) {
            // Extract 10bits of total compressed bits.
            // Keeping it 10 to make it even.
            unsigned char dummy_cmprsd_bits;
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,&dummy_cmprsd_bits,8);
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,&dummy_cmprsd_bits,2);
        }
        #endif
    }
    // Next extract 1 bytes of data_to_add
    if( (*algo == ADDPROC) || (*algo == SIGNSHFTADDPROC) ) {
        btcmpctr_xtrct_bits(inAry,inAryPtr,&state,bytes_to_add,8);
    }
    if( (*algo == BINEXPPROC) ) {
        // Number of symbols is not known during decompress.
        // Needs to be in the header. 5 additional bits after the header.
        int numSymsLen = (*blkSize == BIGBLKSIZE) ? NUMSYMSBL4K : NUMSYMSBL;
        btcmpctr_xtrct_bits(inAry,inAryPtr,&state,numSyms,numSymsLen);
        if ( (*numSyms == 0) && (numSymsLen == 4) ) { *numSyms = 16;}
        if ( (*numSyms == 0) && (numSymsLen == 6) ) { *numSyms = 64;}
        for(int i = 0; i< *numSyms; i++) {
            *(bytes_to_add+i) = 0;
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(bytes_to_add+i),8);
        }
    }
    if ( (*algo == BTEXPPROC) ) {
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Extracted Algo is Bit Map Expansion";
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        // Extract high freq symbol 8 bits.
        btcmpctr_xtrct_bits(inAry,inAryPtr,&state,bytes_to_add,8);
        // Extract numBytes, 8 or 14 bits
        *numBytes = 0;
        unsigned char lB = 0;
        btcmpctr_xtrct_bits(inAry,inAryPtr,&state,&lB,8);
        *numBytes = lB;
        if((*blkSize == BIGBLKSIZE)) {
            // Extract 6 more bits.
            lB = 0;
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,&lB,6);
            (*numBytes) |= (lB << 8);
        }
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "NumBytes is " << std::to_string(*numBytes);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        // Extract the bitmap.
        for(int i = 0 ; i < *blkSize; i++) {
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(bitmap + i),1);
        }
    }
    if(*dual_encode) {
        // Extract Bitmap
        for(int i = 0 ; i < *blkSize; i++) {
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(bitmap + i),1);
        }
    }
    return state;
}

// Extract bytes with a bitmap
unsigned char BitCompactor::btcmpctr_xtrct_bytes_wbitmap(
                                     const unsigned char* inAry,
                                           unsigned  int* inAryPtr,
                                           unsigned char  state,
                                           unsigned char  bitln,
                                           unsigned char* outBuf, // Assume OutBuf is BLKSIZE
                                                     int  blkSize,
                                           unsigned char* bitmap
                                         )
{
    int cnt = 0;
    unsigned char lbitln = bitln;
    while (cnt < blkSize) {
        if (bitln == 0) { lbitln = 8; }
        if(bitmap[cnt]) {
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Extracting Byte " << std::to_string(cnt);
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(outBuf+(cnt++)),8);
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Extracted Byte "<< std::to_string((cnt-1)) <<" is "<< std::to_string(*(outBuf+cnt-1));
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif
        } else {
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Extracting Byte "<< std::to_string(cnt);
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(outBuf+(cnt++)),lbitln);
        }
    }
    return state;

}

// Expand bits to byte, given an input buffer pointing to the exact bit, and the number of bits per symbol. produce an output byte array.
unsigned char BitCompactor::btcmpctr_xtrct_bytes(
                              const unsigned char* inAry,
                                    unsigned  int* inAryPtr,
                                    unsigned char  state,
                                    unsigned char  bitln,
                                    unsigned char* outBuf, // Assume OutBuf is BLKSIZE
                                              int  blkSize,
                                    unsigned char  mode16
                                  )
{
    // This will work for 1byte symbol extract.
    int cnt = 0;
    unsigned char lbitln = bitln;
    while (cnt < blkSize) {
        if(mode16) {
            //construct 16bits output data
            if (bitln == 0) { lbitln = 16; }
            if(lbitln > 8) {
                btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(outBuf+(cnt++)),8);
                btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(outBuf+(cnt++)),(lbitln-8));
            } else {
                btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(outBuf+(cnt++)),lbitln);
            }
        } else {
            if (bitln == 0) { lbitln = 8; }
            btcmpctr_xtrct_bits(inAry,inAryPtr,&state,(outBuf+(cnt++)),lbitln);
        }
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Extracting Bytes, cnt =" << std::to_string(cnt);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
    }
    return state;

}

// tosigned - oppposite of tounsigned.
void BitCompactor::btcmpctr_tosigned(const unsigned char* inAry,
                                           unsigned char* outBuf
                      )
{
    // if LSB == 1, then it was a signed number.
    for(int i = 0; i < BLKSIZE; i++) {
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Before Sign Conversion = " << std::to_string(inAry[i]);
        BTC_REPORT_INFO(mVerbosityLevel,12,mDebugStr.str().c_str());
        #endif
        if((inAry[i]&1) == 1 ) {
            outBuf[i] = (unsigned char) (~inAry[i] >> 1) | 0x80;
        } else {
            outBuf[i] = (unsigned char) (inAry[i] >> 1);
        }
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "After Sign Conversion = "<< std::to_string(outBuf[i]);
        BTC_REPORT_INFO(mVerbosityLevel,12,mDebugStr.str().c_str());
        #endif
    }
}
// 16 and 8 bit adder into array.
void BitCompactor::btcmpctr_addByte(
             const unsigned char* data_to_add,
                      unsigned char  mode16,
                      unsigned char* outBuf
                     )
{
    int cnt = 0;
    while(cnt < BLKSIZE) {
        if(mode16) {
        } else {
            outBuf[cnt] += *data_to_add;
            cnt++;
        }
    }
}

void BitCompactor::btcmpctr_initAlgosAry(const btcmpctr_compress_wrap_args_t& args)
{
    AlgoAry[MINPRDCT_IDX]       = &BitCompactor::btcmpctr_minprdct;
    AlgoAry[MINSPRDCT_IDX]      = &BitCompactor::btcmpctr_minSprdct;
    AlgoAry[MUPRDCT_IDX]        = &BitCompactor::btcmpctr_muprdct;
    AlgoAry[MEDPRDCT_IDX]       = &BitCompactor::btcmpctr_medprdct;
    AlgoAry[NOPRDCT_IDX]        = &BitCompactor::btcmpctr_noprdct;
    AlgoAry[NOSPRDCT_IDX]       = &BitCompactor::btcmpctr_noSprdct;
    if(args.proc_bin_en) {
        AlgoAry[BINCMPCT_IDX]       = &BitCompactor::btcmpctr_binCmpctprdct;
    } else {
        AlgoAry[BINCMPCT_IDX]       = &BitCompactor::btcmpctr_dummyprdct;
    }
    if(args.proc_btmap_en) {
        AlgoAry[BTMAP_IDX]       = &BitCompactor::btcmpctr_btMapprdct;
    } else {
        AlgoAry[BTMAP_IDX]       = &BitCompactor::btcmpctr_dummyprdct;
    }
    //4K Algo
    if(args.proc_btmap_en) {
        AlgoAry4K[BTMAP4K_IDX]       = &BitCompactor::btcmpctr_btMapprdct;
    } else {
        AlgoAry4K[BTMAP4K_IDX]       = &BitCompactor::btcmpctr_dummyprdct;
    }
    if(args.proc_bin_en) {
        AlgoAry4K[BINCMPCT4K_IDX]       = &BitCompactor::btcmpctr_binCmpctprdct;
    } else {
        AlgoAry4K[BINCMPCT4K_IDX]       = &BitCompactor::btcmpctr_dummyprdct;
    }
    //Initialize the Header overhead.
    AlgoAryHeaderOverhead[MINPRDCT_IDX]       = 16 + (2*args.mixedBlkSize) + (2*args.dual_encode_en); // 8bit header + 8 bit byte_to_add (minimum)
    AlgoAryHeaderOverhead[MINSPRDCT_IDX]      = 16 + (2*args.mixedBlkSize) + (2*args.dual_encode_en); // 8bit header + 8 bit byte_to_add (minimum)
    AlgoAryHeaderOverhead[MUPRDCT_IDX]        = 16 + (2*args.mixedBlkSize) + (2*args.dual_encode_en); // 8bit header + 8 bit byte_to_add (mu)
    AlgoAryHeaderOverhead[MEDPRDCT_IDX]       = 16 + (2*args.mixedBlkSize) + (2*args.dual_encode_en); // 8bit header + 8 bit byte_to_add (mu)
    AlgoAryHeaderOverhead[NOPRDCT_IDX]        = 8 + (2*args.mixedBlkSize) + (2*args.dual_encode_en);  // 8 bit header
    AlgoAryHeaderOverhead[NOSPRDCT_IDX]       = 8 + (2*args.mixedBlkSize) + (2*args.dual_encode_en);  // 8 bit header
    AlgoAryHeaderOverhead[BINCMPCT_IDX]       = 12 + (2*args.mixedBlkSize) + (2*args.dual_encode_en);  // 12 bit header. There is additional overhead based on the number of symbols which is dynamic
    AlgoAryHeaderOverhead[BTMAP_IDX]          = 8+8+8+64 + (2*args.mixedBlkSize) + (2*args.dual_encode_en);  // 8 Header, 8 topBinByte,8 ByteLength,64 Bitmap
    AlgoAryHeaderOverhead4K[BINCMPCT4K_IDX]   = 14 + (2*args.mixedBlkSize);  // 12 bit header. There is additional overhead based on the number of symbols which is dynamic
    AlgoAryHeaderOverhead4K[BTMAP4K_IDX]      = 8+8+14+4096 + (2*args.mixedBlkSize);  // 8 Header, 8 topBinByte,14 ByteLength,4096 Bitmap

}

unsigned char BitCompactor::btcmpctr_getAlgofrmIdx(int idx)
{
    if( (idx == NOPRDCT_IDX) ) {
       return NOPROC;
    } else if ( (idx == MINPRDCT_IDX) ) {
       return ADDPROC;
    } else if ( (idx == MINSPRDCT_IDX) || (idx == MUPRDCT_IDX) || (idx == MEDPRDCT_IDX) ) {
       return SIGNSHFTADDPROC;
    } else if ( (idx == NOSPRDCT_IDX) ) {
       return SIGNSHFTPROC;
    } else if ( (idx == BINCMPCT_IDX) ) {
       return BINEXPPROC;
    } else if ( (idx == BTMAP_IDX) ) {
       return BTEXPPROC;
    } else {
       return BITC_ALG_NONE;
    }
}

unsigned char BitCompactor::btcmpctr_get4KAlgofrmIdx(int idx)
{
    if ( (idx == BINCMPCT4K_IDX) ) {
       return BINEXPPROC;
    } else if ( (idx == BTMAP4K_IDX) ) {
       return BTEXPPROC;
    } else {
       return BITC_ALG_NONE;
    }
}

BitCompactor::btcmpctr_algo_choice_t BitCompactor::btcmpctr_ChooseAlgo64B(btcmpctr_algo_args_t* algoArg,
                                                               int mixedBlkSize,
                                                               int dual_encode_en
                                            )
{
    btcmpctr_algo_choice_t algoChoice;
    int minSize, minSizeDual;
    int chosenAlgo, chosenAlgoDual(0);
    int workingBlkSize = (algoArg->blkSize);
    int cmprsdSize, cmprsdSizeDual(0), dualCpSize;
    unsigned char dualBitln;
    unsigned char bitmap[BLKSIZE];

    minSize        = (workingBlkSize*8) + (mixedBlkSize ? 4 : 2);
    minSizeDual    = (workingBlkSize*8) + (mixedBlkSize ? 4 : 2);
    chosenAlgo     = BITC_ALG_NONE;
    chosenAlgoDual = BITC_ALG_NONE;
    *(algoArg->bitln)       = 8;
    // Run Through the 64B Algo's, except BINCMPCT_IDX and BTMAP_IDX (disabled)
    for(int i = 0; i< (BTC27_NUMALGO - 2); i++) {
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Calling Algo "<< std::to_string(i);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        (this->*AlgoAry[i])(algoArg);
        // If BTMAP_IDX then multiple is numBytes
        int numBytes = workingBlkSize;
        if(i == BTMAP_IDX) {
            numBytes = *(algoArg->numBytes);
        }
        cmprsdSize = AlgoAryHeaderOverhead[i] + (numBytes * (*(algoArg->bitln))) ;
        if((dual_encode_en) & (i != BTMAP4K_IDX) ) {
            btcmpctr_calc_dual_bitln(algoArg->residual,&dualBitln,workingBlkSize,(unsigned char*)bitmap,&dualCpSize);
            #ifdef DL_INC_BL
            cmprsdSizeDual = AlgoAryHeaderOverhead[i] + dualCpSize + 64 + 10;
            #else
            cmprsdSizeDual = AlgoAryHeaderOverhead[i] + dualCpSize + 64 ;
            #endif
        }
        if(i == BINCMPCT_IDX) {
            cmprsdSize += ((*(algoArg->numSyms))*8);
            if(dual_encode_en) {
                cmprsdSizeDual += ((*(algoArg->numSyms))*8);
            }
        }
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Compressed Size in bits is  "<< std::to_string(cmprsdSize);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        if(cmprsdSize < minSize) {
            minSize = cmprsdSize;
            chosenAlgo = i;
        }
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Dual Compressed Size in bits is "<< std::to_string(cmprsdSizeDual);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        if((cmprsdSizeDual < minSizeDual) && dual_encode_en && (i != BTMAP4K_IDX)) {
            minSizeDual    = cmprsdSizeDual;
            chosenAlgoDual = i;
        }
    }
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "Chosen Algo is "<< std::to_string(chosenAlgo) << ",";
    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
    #endif
    if((minSizeDual < minSize) && dual_encode_en) {
        // Choose Dual Mode encoding
        algoChoice.dual_encode = 1;
    } else {
        algoChoice.dual_encode = 0;
    }
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "Dual Encode Mode is  "<< std::to_string(algoChoice.dual_encode) <<",";
    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
    #endif
    algoChoice.none       = algoChoice.dual_encode ? (chosenAlgoDual == BITC_ALG_NONE) : (chosenAlgo == BITC_ALG_NONE);
    algoChoice.algoHeader = btcmpctr_getAlgofrmIdx(algoChoice.dual_encode ? chosenAlgoDual : chosenAlgo);
    algoChoice.chosenAlgo = algoChoice.none ? &BitCompactor::btcmpctr_dummyprdct : (AlgoAry[algoChoice.dual_encode ? chosenAlgoDual : chosenAlgo]);
    algoChoice.cmprsdSize = algoChoice.dual_encode ? minSizeDual : minSize;
    algoChoice.algoType   = 0;
    algoChoice.workingBlkSize = workingBlkSize;
    return algoChoice;

}

BitCompactor::btcmpctr_algo_choice_t BitCompactor::btcmpctr_ChooseAlgo4K(btcmpctr_algo_args_t* algoArg,
                                                              int mixedBlkSize
                                           )
{
    btcmpctr_algo_choice_t algoChoice = {};
    int minSize;
    int chosenAlgo;
    int workingBlkSize = (algoArg->blkSize);
    int cmprsdSize;

    minSize     = (workingBlkSize*8) + (mixedBlkSize ? 4 : 2);
    chosenAlgo  = BITC_ALG_NONE;
    *(algoArg->bitln)       = 8;
    // Run Through the 64B Algo's
    for(int i = 0; i< BTC27_NUM4KALGO; i++) {
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Calling Algo "<< std::to_string(i);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        (this->*AlgoAry4K[i])(algoArg);
        int numBytes = workingBlkSize;
        if(i == BTMAP4K_IDX) {
            numBytes = *(algoArg->numBytes);
        }
        cmprsdSize = AlgoAryHeaderOverhead4K[i] + (numBytes * (*(algoArg->bitln)));
        #ifdef __BTCMPCTR__EN_DBG__
        mDebugStr.str(""); mDebugStr << "Compressed Size in bits is "<< std::to_string(cmprsdSize);
        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
        #endif
        if(i == BINCMPCT4K_IDX) {
            cmprsdSize += ((*(algoArg->numSyms))*8);
        }
        if(cmprsdSize < minSize) {
            minSize = cmprsdSize;
            chosenAlgo = i;
        }
    }
    #ifdef __BTCMPCTR__EN_DBG__
    mDebugStr.str(""); mDebugStr << "Chosen Algo is "<< std::to_string(chosenAlgo) << ",";
    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
    #endif
    algoChoice.algoHeader = btcmpctr_get4KAlgofrmIdx(chosenAlgo);

    // @ajgorman: KW fix
    if (chosenAlgo == BITC_ALG_NONE) {
        algoChoice.none = 0x1;
        algoChoice.chosenAlgo = nullptr; // should never be used if 'none' is asserted
    } else {
        algoChoice.none = 0x0;
        algoChoice.chosenAlgo = AlgoAry4K[chosenAlgo]; // chosenAlgo should be < NUM4KALGO
    }

    algoChoice.cmprsdSize = minSize;
    algoChoice.algoType   = 1;
    algoChoice.workingBlkSize = workingBlkSize;

    return algoChoice;
}

//Compress Wrap
//This is a SWIG/numpy integration friendly interface for the compression function.
//
unsigned int BitCompactor::CompressArray(const unsigned char* const              src,
                                         unsigned int                            srcLen,
                                         unsigned char*                          dst,
                                         unsigned int                            dstLen,
                                         const btcmpctr_compress_wrap_args_t&    args
                                        )
{
    unsigned int resultLen = dstLen;
    int status;

    status = CompressWrap(src, srcLen, dst, resultLen, args);
    if(status) {
        return resultLen;
    } else {
        return 0;
    }
}

int BitCompactor::CompressWrap(const unsigned char*                 src,
                               unsigned int                         srcLen,
                               unsigned char*                       dst,
                               unsigned int&                        dstLen, // dstLen holds the size of the output buffer.
                               const btcmpctr_compress_wrap_args_t& args
                           )
{
    mVerbosityLevel = args.verbosity;
    if(src && dst)
    {
        // Check if output buffer is big enough for compressed data worst case
        unsigned int boundSize = this->GetCompressedSizeBound(srcLen);
        if(boundSize <= dstLen)
        {
            btcmpctr_initAlgosAry(args);
            // Keep a copy of the destination buffer size allocated
            // to be used in checks. The final destination size will
            // be returned in this pointer.

            unsigned char bitln;
            unsigned char minimum[MAXSYMS4K];
            int cmprsdSize, workingBlkSize,dualCpSize;
            unsigned int srcCnt = 0, dstCnt = 0;
            int chosenAlgo;
            int state = 0;
            int blkCnt = 0;
            int numSyms = 0;
            unsigned char residual[BIGBLKSIZE];
            unsigned char bitmap[BIGBLKSIZE];
            int numBytes;
            unsigned int accum = 0;
            btcmpctr_algo_args_t algoArg;
            btcmpctr_algo_choice_t chosenAlgos[BIGBLKSIZE/BLKSIZE];
            btcmpctr_algo_choice_t chosenAlgos4K = {};
            int smCntr = 0;
            int numSmBlks = 0;
            int bigBlkSize;

            // Choose the correct Algo to run.
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Source Length = "<< std::to_string(srcLen);
            BTC_REPORT_INFO(mVerbosityLevel,1,mDebugStr.str().c_str());
            #endif
            algoArg.minimum  = (unsigned char *)&minimum;
            algoArg.bitln    = &bitln;
            algoArg.numSyms  = &numSyms;
            algoArg.bitmap   = (unsigned char *)&bitmap;
            algoArg.numBytes = &numBytes;
            algoArg.residual = (unsigned char *)&residual;
            algoArg.minFixedBitLn = args.minFixedBitLn;

            while (srcCnt < srcLen) {
                if( (srcCnt + BIGBLKSIZE) > srcLen) {
                    bigBlkSize = (srcLen - srcCnt);
                } else {
                    bigBlkSize = BIGBLKSIZE;
                }
                // Choose 4K Algorithm
                if((bigBlkSize == BIGBLKSIZE) && args.mixedBlkSize) {
                   algoArg.inAry = (src + srcCnt);
                   algoArg.blkSize = bigBlkSize;
                   chosenAlgos4K = btcmpctr_ChooseAlgo4K(&algoArg,args.mixedBlkSize);
                }

                // Work on the 64B block size.
                cmprsdSize = 0;
                smCntr = 0;
                numSmBlks = 0;
                while(smCntr < bigBlkSize) {

                    if( (smCntr + BLKSIZE) > bigBlkSize) {
                        workingBlkSize = (bigBlkSize - smCntr);
                    } else {
                        workingBlkSize = BLKSIZE;
                    }
                    algoArg.inAry = (src + srcCnt + smCntr);
                    algoArg.blkSize = workingBlkSize;
                    // Call the Algo Choice.
                    #ifdef __BTCMPCTR__EN_DBG__
                    mDebugStr.str(""); mDebugStr << "Trying to find best algo for this blockCnt = "<< std::to_string(blkCnt);
                    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                    #endif
                    chosenAlgos[numSmBlks].workingBlkSize = workingBlkSize;
                    if (workingBlkSize == BLKSIZE) {
                        chosenAlgos[numSmBlks] = btcmpctr_ChooseAlgo64B(&algoArg,args.mixedBlkSize,args.dual_encode_en);
                        #ifdef __BTCMPCTR__EN_DBG__
                        mDebugStr.str(""); mDebugStr << " blkCnt = "<< std::to_string(blkCnt);
                        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                        #endif
                    }
                    cmprsdSize += chosenAlgos[numSmBlks].cmprsdSize;
                    numSmBlks++;
                    smCntr += workingBlkSize;
                    #ifdef __BTCMPCTR__EN_DBG__
                    mDebugStr.str(""); mDebugStr << "bigBlkSize = "<< std::to_string(bigBlkSize)<< ", smCntr = "<< std::to_string(smCntr) << ", numSmBlks = "<< std::to_string(numSmBlks);
                    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                    #endif
                }
                // IF    Compressed size of 4K block < totoal comporessed size of 64B blocks, and if this is not the last <4K block. and if mixed Block size is enabled.
                if( (((chosenAlgos4K.cmprsdSize <= cmprsdSize) && (bigBlkSize == BIGBLKSIZE)) || args.bypass_en) && args.mixedBlkSize) {
                    //---------------------------------------------------------------------------------
                    // Chosen 4K Blocks
                    //---------------------------------------------------------------------------------
                    #ifdef __BTCMPCTR__EN_DBG__
                    mDebugStr.str(""); mDebugStr << "Running chosen algo";
                    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                    #endif
                    if ( (chosenAlgos4K.workingBlkSize < BIGBLKSIZE) || args.bypass_en) {
                        // Force an Algo for the last block.
                        chosenAlgos4K.none = 1;
                    }
                    if(chosenAlgos4K.none != 1) {
                        algoArg.inAry = (src + srcCnt);
                        algoArg.blkSize = chosenAlgos4K.workingBlkSize;
                        (this->*chosenAlgos4K.chosenAlgo)(&algoArg);
                        chosenAlgo = chosenAlgos4K.algoHeader;
                     } else {
                        bitln = 8;
                        for(int i = 0; i < chosenAlgos4K.workingBlkSize; i++) {
                            residual[i] = *(src + srcCnt + i);
                        }
                        chosenAlgo = BITC_ALG_NONE;
                    }
                    // Insert Header
                    #ifdef __BTCMPCTR__EN_DBG__
                    mDebugStr.str(""); mDebugStr << "Inserting Header, chosen Algo in 4K is "<< std::to_string(chosenAlgo);
                    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                    #endif
                    state = btcmpctr_insrt_hdr(chosenAlgo, bitln, &dstCnt, dst, state, &accum,0,chosenAlgos4K.workingBlkSize,args.mixedBlkSize,0);
                    // Insert Post Header bytes
                    // Insert the symbols in case of BINEXPPROC.
                    if ( (chosenAlgo == BINEXPPROC) ) {
                       // Insert 6 bits of numSyms
                       int numSymsToInsrt = (numSyms == 64) && (NUMSYMSBL4K == 6) ? 0 : numSyms;
                       state = btcmpctr_insrt_byte(numSymsToInsrt,NUMSYMSBL4K,&dstCnt,dst,state,&accum,0);
                       // Insert the number of symbols.
                       for(int i = 0; i < numSyms; i++) {
                           #ifdef __BTCMPCTR__EN_DBG__
                           mDebugStr.str(""); mDebugStr << "Inserting Binned Header "<< std::to_string(i) << ", "<< std::to_string(minimum[i]);
                           BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                           #endif
                           state = btcmpctr_insrt_byte(minimum[i],8,&dstCnt,dst,state,&accum,0);
                       }
                    }
                    if ( (chosenAlgo == BTEXPPROC) ) {
                        // Insert 8bits of max freq symbol.
                        state = btcmpctr_insrt_byte(minimum[0],8,&dstCnt,dst,state,&accum,0);
                        // insert 14bits of byte length (14 to keep an even number of header bits)
                        state = btcmpctr_insrt_byte(numBytes,8,&dstCnt,dst,state,&accum,0);
                        state = btcmpctr_insrt_byte((numBytes>>8),6,&dstCnt,dst,state,&accum,0);
                        // Insert 4096 bits of bitmap
                        for(int i = 0; i < chosenAlgos4K.workingBlkSize; i++) {
                           state = btcmpctr_insrt_byte(bitmap[i],1,&dstCnt,dst,state,&accum,0);
                        }
                    }

                    // Insert data.
                    #ifdef __BTCMPCTR__EN_DBG__
                    mDebugStr.str(""); mDebugStr << "Inserting Data";
                    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                    #endif
                    int dataSize = (chosenAlgo == BTEXPPROC) ? numBytes : chosenAlgos4K.workingBlkSize;
                    for(int i = 0; i< dataSize; i++) {
                        #ifdef __BTCMPCTR__EN_DBG__
                        mDebugStr.str(""); mDebugStr << "Inserting Data cnt ="<< std::to_string(i)<<" Data = "<< std::to_string(residual[i])<< ", Src Data = "<< std::hex << std::to_string(*(src + srcCnt + i));
                        BTC_REPORT_INFO(mVerbosityLevel,6,mDebugStr.str().c_str());
                        #endif
                        state = btcmpctr_insrt_byte(residual[i],bitln,&dstCnt,dst,state,&accum,0);
                    }

                } else {
                    //---------------------------------------------------------------------------------
                    // Chosen 64B Blocks
                    //---------------------------------------------------------------------------------
                    // ChosenAlgo must be re-run with to correctly generate the compressed
                    smCntr = 0;
                    for(int smBlk = 0; smBlk < numSmBlks ; smBlk++) {
                        #ifdef __BTCMPCTR__EN_DBG__
                        mDebugStr.str(""); mDebugStr << "Running chosen algo for small block count = "<< std::to_string(smBlk) <<"with blockSize = "<< std::to_string(chosenAlgos[smBlk].workingBlkSize);
                        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                        #endif
                        if ( (chosenAlgos[smBlk].workingBlkSize < BLKSIZE) || args.bypass_en) {
                            // Force an Algo for the last block.
                            chosenAlgos[smBlk].none = 1;
                            chosenAlgos[smBlk].dual_encode = 0;
                        }
                        if(chosenAlgos[smBlk].none != 1) {
                            algoArg.inAry = (src + srcCnt + smCntr);
                            algoArg.blkSize = chosenAlgos[smBlk].workingBlkSize;
                            (this->*chosenAlgos[smBlk].chosenAlgo)(&algoArg);
                            chosenAlgo = chosenAlgos[smBlk].algoHeader;
                            if(chosenAlgos[smBlk].dual_encode) {
                                // Re-run dualEncode calculate length
                                btcmpctr_calc_dual_bitln((unsigned char*)&residual,&bitln,chosenAlgos[smBlk].workingBlkSize,(unsigned char*)bitmap,&dualCpSize);
                            }
                         } else {
                            bitln = 8;
                            for(int i = 0; i < chosenAlgos[smBlk].workingBlkSize; i++) {
                                residual[i] = *(src + srcCnt + smCntr + i);
                            }
                            chosenAlgo = BITC_ALG_NONE;
                        }
                        // Insert Header
                        #ifdef __BTCMPCTR__EN_DBG__
                        mDebugStr.str(""); mDebugStr << "Inserting Header";
                        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                        #endif
                        state = btcmpctr_insrt_hdr(chosenAlgo, bitln, &dstCnt, dst, state, &accum,0,chosenAlgos[smBlk].workingBlkSize,args.mixedBlkSize,0);
                        // Insert Post Header bytes
                        if(chosenAlgo != BITC_ALG_NONE) {
                            #ifdef __BTCMPCTR__EN_DBG__
                            mDebugStr.str(""); mDebugStr << "Inserting Header";
                            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                            #endif
                            if(chosenAlgos[smBlk].dual_encode) {
                                state = btcmpctr_insrt_byte(1,2,&dstCnt,dst,state,&accum,0);
                                #ifdef DL_INC_BL
                                // Insert the bit length
                                //calculae the bitlength
                                uint16_t cpBitLen = 0;
                                for(int i = 0; i < chosenAlgos[smBlk].workingBlkSize; i++) {
                                    cpBitLen = bitmap[i] ? cpBitLen+8 : cpBitLen+bitln;
                                }
                                // Insert 10 bits.
                                state = btcmpctr_insrt_byte(cpBitLen,8,&dstCnt,dst,state,&accum,0);
                                cpBitLen >>= 8;
                                state = btcmpctr_insrt_byte(cpBitLen,2,&dstCnt,dst,state,&accum,0);
                                #endif
                            } else {
                                state = btcmpctr_insrt_byte(0,2,&dstCnt,dst,state,&accum,0);
                            }
                        }
                        if( (chosenAlgo == ADDPROC) || (chosenAlgo == SIGNSHFTADDPROC) ) {
                            // Insert one more byte.
                            #ifdef __BTCMPCTR__EN_DBG__
                            mDebugStr.str(""); mDebugStr << "Inserting Header plus 1 more byte";
                            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                            #endif
                            state = btcmpctr_insrt_byte(minimum[0],8,&dstCnt,dst,state,&accum,0);
                        }
                        // Insert the symbols in case of BINEXPPROC.
                        if ( (chosenAlgo == BINEXPPROC) ) {
                           // Insert 5 bits of numSyms
                           int numSymsToInsrt = (numSyms == 16) && (NUMSYMSBL == 4) ? 0 : numSyms;
                           state = btcmpctr_insrt_byte(numSymsToInsrt,NUMSYMSBL,&dstCnt,dst,state,&accum,0);
                           // Insert the number of symbols.
                           for(int i = 0; i < numSyms; i++) {
                               #ifdef __BTCMPCTR__EN_DBG__
                               mDebugStr.str(""); mDebugStr << "Inserting Binned Header "<< std::to_string(i)<<", "<< std::to_string(minimum[i]);
                               BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                               #endif
                               state = btcmpctr_insrt_byte(minimum[i],8,&dstCnt,dst,state,&accum,0);
                           }
                        }
                        if ( (chosenAlgo == BTEXPPROC) ) {
                            // Insert 8bits of max freq symbol.
                            state = btcmpctr_insrt_byte(minimum[0],8,&dstCnt,dst,state,&accum,0);
                            // insert 8bits of byte length (8 to keep an even number of header bits)
                            state = btcmpctr_insrt_byte(numBytes,8,&dstCnt,dst,state,&accum,0);
                            // Insert 64 bits of bitmap
                            for(int i = 0; i < chosenAlgos[smBlk].workingBlkSize; i++) {
                               state = btcmpctr_insrt_byte(bitmap[i],1,&dstCnt,dst,state,&accum,0);
                            }
                        }
                        // Insert the Bitmap for dual encode
                        if(args.dual_encode_en) {
                            if(chosenAlgos[smBlk].dual_encode) {
                                for(int i = 0; i < chosenAlgos[smBlk].workingBlkSize; i++) {
                                   state = btcmpctr_insrt_byte(bitmap[i],1,&dstCnt,dst,state,&accum,0);
                                }
                            }
                        }

                        // Insert data.
                        #ifdef __BTCMPCTR__EN_DBG__
                        mDebugStr.str(""); mDebugStr << "Inserting Data";
                        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                        #endif
                        int dataSize = (chosenAlgo == BTEXPPROC) ? numBytes : chosenAlgos[smBlk].workingBlkSize;
                        for(int i = 0; i< dataSize; i++) {
                            #ifdef __BTCMPCTR__EN_DBG__
                            mDebugStr.str(""); mDebugStr << "Inserting Data cnt ="<< std::to_string(i) <<" Data = "<< std::to_string(residual[i])<<", Src Data = "<< std::hex << std::to_string(*(src + srcCnt + smCntr + i));
                            BTC_REPORT_INFO(mVerbosityLevel,6,mDebugStr.str().c_str());
                            #endif
                            if( (args.dual_encode_en) && (chosenAlgos[smBlk].dual_encode) && bitmap[i] ) {
                                state = btcmpctr_insrt_byte(residual[i],8,&dstCnt,dst,state,&accum,0);
                            } else {
                                state = btcmpctr_insrt_byte(residual[i],bitln,&dstCnt,dst,state,&accum,0);
                            }
                        }
                        //
                        #ifdef __BTCMPCTR__EN_DBG__
                        mDebugStr.str(""); mDebugStr << "Destination length is "<< std::to_string(dstCnt);
                        BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                        #endif
                        smCntr += chosenAlgos[smBlk].workingBlkSize;
                    }
                }
                srcCnt += bigBlkSize;
                blkCnt++;
            }
            // Insert end of stream bits.
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Inserting End of Stream";
            BTC_REPORT_INFO(mVerbosityLevel,6,mDebugStr.str().c_str());
            #endif
            state = btcmpctr_insrt_hdr(0,0,&dstCnt,dst,state,&accum,1,0,0,args.align);
            // Check if state is non-zero, if so,  need to increment dstLen.
            if(state != 0) {
                #ifdef __BTCMPCTR__EN_DBG__
                mDebugStr.str(""); mDebugStr << "ERROR: state != 0 at the end of compression";
                BTC_REPORT_INFO(mVerbosityLevel,0,mDebugStr.str().c_str());
                #endif
            }
            // All Done!!
            dstLen = dstCnt;
            return 1;
        }
        else {
            BTC_REPORT_ERROR("CompressWrap: ERROR! Output buffer not big enough for worst case");
            return 0;
        }
    }
    else
    {
        BTC_REPORT_ERROR("CompressWrap: ERROR! Null Pointer");
        return 0;
    }
}


// DecompressWrap
//This is a SWIG/numpy integration friendly interface for the decompression function.
//
unsigned int BitCompactor::DecompressArray(const unsigned char* const             src,
                                            unsigned int                          srcLen,
                                            unsigned char*                        dst,
                                            unsigned int                          dstLen,
                                            const btcmpctr_compress_wrap_args_t&  args
                                 )
{
    unsigned int resultLen = dstLen;
    int status;
    status = DecompressWrap(src, srcLen, dst, resultLen, args);
    if(status) {
        return resultLen;
    } else {
        return 0;
    }
}

int BitCompactor::DecompressWrap(const unsigned char*                   src,
                                 unsigned int                           srcLen,
                                 unsigned char*                         dst,
                                 unsigned int&                          dstLen,
                                 const btcmpctr_compress_wrap_args_t&   args
                            )
{
    if(src && dst)
    {
        unsigned char state = 0;
        unsigned char cmp, eofr,algo,bitln, numSyms;
        int blkSize;
        unsigned char bytes_to_add[MAXSYMS4K];
        unsigned char bitmap[BIGBLKSIZE];
        unsigned char bitmapBytes[BIGBLKSIZE];
        unsigned int numBytes = 0;
        unsigned int blkCnt = 0;
        unsigned int srcLenTrk = 0;
        mVerbosityLevel = args.verbosity;
        int mixedBlkSize = args.mixedBlkSize;
        int dual_encode_en = args.dual_encode_en;
        unsigned char dual_encode;
        unsigned int dstCnt = 0;

        while ( ( srcLenTrk < srcLen ) ) {
            // Extract Header
            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Extracting Header for blockCnt = "<< std::to_string(blkCnt);
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif
            state = btcmpctr_xtrct_hdr(src,&srcLenTrk,state,&cmp,&eofr,&algo,&bitln,&blkSize,(unsigned char*)bytes_to_add,&numSyms,&numBytes,(unsigned char *)bitmap,mixedBlkSize,dual_encode_en,&dual_encode);
            #ifdef __BTCMPCTR__EN_DBG__
            if ( srcLenTrk > ( (srcLen) - 1 ) && ( (srcLen) > 0 ) )
            {
                // no more compressed source data to process; srcLenTrk has reached the end of the array
                mDebugStr.str(""); mDebugStr << "src = 0x--, srcLen = "<< std::to_string(srcLen) <<", srcLenTrk = "<< std::to_string(srcLenTrk) <<", EOFR = "<< std::to_string(eofr) <<", Compressed = "<< std::to_string(cmp) <<", Algo = "<< std::to_string(algo) <<", Bit Length = "<< std::to_string(bitln) <<", Block Size = "<< std::to_string(blkSize) << "[reached end of source data]";
            }
            else
            {
                // still more compressed source data left to process
                mDebugStr.str(""); mDebugStr << "src = 0x" << std::hex << std::setfill('0') << std::setw(2) << std::to_string(src[srcLenTrk]) <<", srcLen = "<< std::dec << std::to_string(srcLen) <<", srcLenTrk = "<< std::to_string(srcLenTrk) <<", EOFR = "<< std::to_string(eofr) <<", Compressed = "<< std::to_string(cmp) <<", Algo = "<< std::to_string(algo) <<", Bit Length = "<< std::to_string(bitln) <<", Block Size = "<< std::to_string(blkSize);
            }
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif

            // if we're at EOFR, OR if srcLenTrk has reached the end of
            // the compressed source data, stop processing at this point
            // (the 'while' loop condition should then cause us to finish)
            if( ( srcLenTrk > ( (srcLen) - 1 ) ) || eofr )
            {
                continue;
            }
            //
            if(cmp) {
                //Compressed block. Process further based on Algo
                #ifdef __BTCMPCTR__EN_DBG__
                mDebugStr.str(""); mDebugStr << "Compressed Block, Extracting data";
                BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                #endif
                if ( algo == BTEXPPROC ) {
                    // bitln set to 0, since we always extract 8bits for the non high freq calculations.
                    state = btcmpctr_xtrct_bytes(src,&srcLenTrk,state,0,bitmapBytes,numBytes,0);
                } else {
                    if(dual_encode) {
                        state = btcmpctr_xtrct_bytes_wbitmap(src,&srcLenTrk,state,bitln,(dst+dstCnt),blkSize,(unsigned char *)bitmap);
                    } else {
                        state = btcmpctr_xtrct_bytes(src,&srcLenTrk,state,bitln,(dst+dstCnt),blkSize,0);
                    }
                }
                if ( (algo == SIGNSHFTADDPROC) || (algo == SIGNSHFTPROC) ) {
                    // Convert to signed
                    #ifdef __BTCMPCTR__EN_DBG__
                    mDebugStr.str(""); mDebugStr << "Converting to Signed form";
                    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                    #endif
                    btcmpctr_tosigned((dst+dstCnt),(dst+dstCnt));
                }
                // Add any bytes.
                if ( (algo == ADDPROC) || (algo == SIGNSHFTADDPROC) ) {
                    // add bytes_to_add
                    #ifdef __BTCMPCTR__EN_DBG__
                    mDebugStr.str(""); mDebugStr << "Adding Data";
                    BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                    #endif
                    btcmpctr_addByte((unsigned char*)bytes_to_add,0,(dst+dstCnt));
                }
                // Reconstruct bitmap by using lookup into bytes_to_add vector.
                if ( (algo == BINEXPPROC) ) {
                    for(int i = 0; i < blkSize; i++) {
                        *(dst+dstCnt+i) = bytes_to_add[*(dst+dstCnt+i)];
                    }
                }
                if ( (algo == BTEXPPROC) ) {
                    // Extract numBytes from
                    int cnt = 0;
                    for(int i = 0; i < blkSize; i ++) {
                        if(!bitmap[i]) {
                            *(dst+dstCnt+i) = bytes_to_add[0];
                        } else {
                            *(dst+dstCnt+i) = bitmapBytes[cnt++];
                        }
                    }
                    //assert cnt == numBytes;
                }
            } else {
                // Uncompressed block
                #ifdef __BTCMPCTR__EN_DBG__
                mDebugStr.str(""); mDebugStr << "Uncompressed Data block..";
                BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
                #endif
                state = btcmpctr_xtrct_bytes(src,&srcLenTrk,state,bitln,(dst+dstCnt),blkSize,0);
            }

            dstCnt += blkSize;
            if (dstCnt > dstLen) {
                BTC_REPORT_ERROR("DeompressWrap: Max expected decompress size " + std::to_string(dstLen) + " bytes exceeded!");
                return 0;
            }

            #ifdef __BTCMPCTR__EN_DBG__
            mDebugStr.str(""); mDebugStr << "Src length = "<< std::to_string(srcLenTrk);
            BTC_REPORT_INFO(mVerbosityLevel,5,mDebugStr.str().c_str());
            #endif
            blkCnt++;
        }
        // All Done!!
        dstLen = dstCnt;
        return 1;
    }
    else
    {
        BTC_REPORT_ERROR("DecompressWrap: ERROR! Null Pointer");
        return 0;
    }
}

unsigned int BitCompactor::GetCompressedSizeBound(unsigned int bufSize) const
{
    return ceil(((ceil(bufSize/BLKSIZE) * 4) + 2)/8) + bufSize + 1 + 64;
}

BitCompactor::elem_type BitCompactor::getMedianNaive(elem_type arr[], unsigned int blkSize)
{
    unsigned char medianValue = 0;

    // Sort array in ascending order
    std::sort(arr, arr + blkSize);

    // Even length (after sorting)
    if (blkSize % 2 == 0) {
        medianValue = arr[blkSize/2 -1];   // As per original quick_slect(), median = lower value of two middle values
    } else {
        // Odd length
        medianValue = arr[blkSize/2];
    }

    return medianValue;
}
} // namespace btc27
