/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
Single Rate Asymmetrical FIR kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#ifdef __X86SIM__
// #define _DSPLIB_FIR_TDM_HPP_DEBUG_
#endif
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "fir_tdm.hpp"
#include "kernel_api_utils.hpp"
#include "fir_tdm_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {

// Single Rate Asymmetrical FIR run-time function
// According to template parameter the input may be a window, or window and cascade input
// Similarly the output interface may be a window or a cascade output
// FIR function
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_TDM_CHANNELS,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterKernel(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    // broadcast Window buffer through cascade, when required
    windowBroadcast<TT_DATA, TP_INPUT_WINDOW_VSIZE + get_margin(), TP_API, TP_DUAL_IP, m_kKernelPosEnum>(inInterface,
                                                                                                         outInterface);

    // select architecture and run FIR iteration
    filterSelectArch(inInterface, outInterface);
};

// FIR function
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_TDM_CHANNELS,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterSelectArch(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                             T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    if
        constexpr(m_kArch == kArchInternalMargin) { filterInternalMargin(inInterface, outInterface); }
    else {
        filterBasic(inInterface, outInterface);
    }
};
// #undef _DSPLIB_FIR_TDM_HPP_DEBUG_

// ----------------------------------------------------- Basic ----------------------------------------------------- //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_TDM_CHANNELS,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterBasic(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                        T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    TT_COEFF* __restrict coeffPtr = &m_internalTaps[0];
    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    using dataRead_t = ::aie::vector<TT_DATA, kSamplesInVectAcc>;
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVectData>;
    using outDataVect_t = ::aie::vector<TT_DATA, kSamplesInVectAcc>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVectCoeff>;

    using accVect_t = ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVectAcc>;

    dataVect_t dataVect;
    dataRead_t tmp, tmp2;
    dataVect_t* __restrict inPointer;
    dataRead_t* __restrict inPointerRead;
    outDataVect_t outVect, outVect2;
    coeffVect_t* __restrict coeffVectPtr;

    coeffVect_t coeffVect;
    accVect_t acc, acc2;
    dataVect_t* frameStart = (dataVect_t*)inInterface.inWindow;
    outDataVect_t* __restrict outPtr = (outDataVect_t*)outInterface.outWindowPtr;

    // inPointer = frameStart;
    // Loop through each frame
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            // pointer to a vector of coefficients
            coeffVectPtr = (coeffVect_t*)(coeffPtr);
            for (unsigned int tdm = 0; tdm < TP_TDM_LOOP_SIZE; tdm++)
                chess_prepare_for_pipelining chess_loop_range((TP_TDM_LOOP_SIZE), (TP_TDM_LOOP_SIZE)) {
                    int j = 0;
                    int dataIncrement = (tdm + (frame * TP_TDM_LOOP_SIZE) + (columnMultiple * j * TP_TDM_LOOP_SIZE));
                    int dataIncrement2 =
                        (tdm + (frame * TP_TDM_LOOP_SIZE) + ((columnMultiple * j + 1) * TP_TDM_LOOP_SIZE));

                    if
                        constexpr(columnMultiple == 1) {
                            inPointer = ((dataVect_t*)frameStart) + dataIncrement;
                            dataVect = *inPointer;
                            tmp = *inPointer;
                            tmp2 = *inPointer;
                        }
                    if
                        constexpr(columnMultiple == 2) {
                            inPointerRead = ((dataRead_t*)inInterface.inWindow) + dataIncrement;
                            tmp = *inPointerRead;
                            dataVect.insert(0, tmp);
                            inPointerRead = ((dataRead_t*)inInterface.inWindow) + dataIncrement2;
                            tmp2 = *inPointerRead;
                            dataVect.insert(1, tmp2);
                        }

                    // Make sure data and coeff vectors are of the same sample size, i.e. both have e.g. 8 samples.
                    coeffVect = *coeffVectPtr++;

                    acc = mulTdm(acc, dataVect, coeffVect);

#pragma unroll(TP_FIR_RANGE_LEN / columnMultiple)
                    for (unsigned int j = 1; j < (TP_FIR_RANGE_LEN / columnMultiple); j++) {
                        // always load 256-bits of data.
                        int dataIncrement =
                            (tdm + (frame * TP_TDM_LOOP_SIZE) + (columnMultiple * j * TP_TDM_LOOP_SIZE));
                        int dataIncrement2 =
                            (tdm + (frame * TP_TDM_LOOP_SIZE) + ((columnMultiple * j + 1) * TP_TDM_LOOP_SIZE));

                        if
                            constexpr(columnMultiple == 1) {
                                inPointer = ((dataVect_t*)frameStart) + dataIncrement;
                                dataVect = *inPointer;
                            }
                        if
                            constexpr(columnMultiple == 2) {
                                // dataVect_t = 2 x dataRead_t. Don't read 16 consequtive samples, just 8 and 8 for next
                                // frame
                                inPointerRead = ((dataRead_t*)inInterface.inWindow) + dataIncrement;
                                tmp = *inPointerRead;
                                dataVect.insert(0, tmp);
                                inPointerRead = ((dataRead_t*)inInterface.inWindow) + dataIncrement2;
                                tmp2 = *inPointerRead;
                                dataVect.insert(1, tmp2);
                            }

                        // Make sure data and coeff vectors are of the same sample size, i.e. both have e.g. 8 samples.
                        // Each load is 256 bit? so schedule load every single or every second iteration.
                        // Alternatively, cover the difference with different loads, e.g. 256-bit load and a 128-bit
                        // load
                        // and schedule both on each iteration.
                        coeffVect = *coeffVectPtr++;
                        acc = macTdm(acc, dataVect, coeffVect);
                    }

                    // output to outVect or to outCascade
                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                            // outVect2 = acc2.template to_vector<TT_DATA>(TP_SHIFT);
                            *outPtr++ = outVect;
                            // *outPtr++ = outVect2;
                        }
                    else {
                        writeincr(outInterface.outCascade, acc);
                        // writeincr(outInterface.outCascade, acc2);
                    }
                    // Jump to data of next frame
                    inPointer -= ((TP_TDM_CHANNELS * TP_FIR_LEN) - kSamplesInVectData);
                }
        }
};

// ----------------------------------------------------- filterInternalMargin
// ----------------------------------------------------- //
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
INLINE_DECL void kernelFilterClass<TT_DATA,
                                   TT_COEFF,
                                   TP_FIR_LEN,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_INPUT_WINDOW_VSIZE,
                                   TP_TDM_CHANNELS,
                                   TP_CASC_IN,
                                   TP_CASC_OUT,
                                   TP_FIR_RANGE_LEN,
                                   TP_KERNEL_POSITION,
                                   TP_CASC_LEN,
                                   TP_USE_COEFF_RELOAD,
                                   TP_NUM_OUTPUTS,
                                   TP_DUAL_IP,
                                   TP_API,
                                   TP_MODIFY_MARGIN_OFFSET,
                                   TP_COEFF_PHASE,
                                   TP_COEFF_PHASE_OFFSET,
                                   TP_COEFF_PHASES,
                                   TP_COEFF_PHASES_LEN,
                                   TP_SAT>::filterInternalMargin(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                                 T_outputIF<TP_CASC_OUT, TT_DATA> outInterface) {
    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();
    TT_COEFF* __restrict coeffPtr = &m_internalTaps[0];
    TT_DATA* __restrict bufferPtr = &m_inputBuffer[0];
    TT_COEFF* m_internalTapsCopy = m_internalTaps;

    using dataRead_t = ::aie::vector<TT_DATA, kSamplesInVectAcc>;
    using dataVect_t = ::aie::vector<TT_DATA, kSamplesInVectData>;
    using outDataVect_t = ::aie::vector<TT_DATA, kSamplesInVectAcc>;
    using coeffVect_t = ::aie::vector<TT_COEFF, kSamplesInVectCoeff>;

    using accVect_t = ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, kSamplesInVectAcc>;

    dataVect_t dataVect;
    dataVect_t* __restrict inPointer;
    outDataVect_t outVect, outVect2;
    coeffVect_t* __restrict coeffVectPtr;

    coeffVect_t coeffVect;
    accVect_t acc, acc2;
    input_circular_buffer<TT_DATA, extents<internalBufferSize>, margin<0> > inWindowCirc(&m_inputBuffer[0],
                                                                                         internalBufferSize, 0);
    auto inWrItr = ::aie::begin_vector_random_circular<kSamplesInVectData>(m_inputBuffer, internalBufferSize);
    // auto inRdItr =  ::aie::begin_vector_random_circular<kSamplesInVectAcc>(m_inputBuffer, internalBufferSize);
    auto inRdItr = ::aie::begin_vector_random_circular<kSamplesInVectAcc>(inWindowCirc);
    dataVect_t* frameStart = (dataVect_t*)inInterface.inWindow;
    outDataVect_t* __restrict outPtr = (outDataVect_t*)outInterface.outWindowPtr;
    inWrItr += (marginFrame)*TP_TDM_CHANNELS / kSamplesInVectData;
    inRdItr += ((marginFrame + 1)) * columnMultiple * TP_TDM_CHANNELS / kSamplesInVectData;

    // Loop through each frame
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            if
                constexpr(m_kFirMargin == 0) {
                    // Embed margin handling here, as this would reduce the amount of buffer size.

                    dataVect_t* frameStart = (dataVect_t*)inInterface.inWindow;
                    for (int i = 0; i < TP_TDM_CHANNELS / kSamplesInVectData; i++) {
                        inPointer = ((dataVect_t*)frameStart) + i + frame * TP_TDM_CHANNELS / kSamplesInVectData;
                        dataVect = *inPointer;
                        *inWrItr++ = dataVect;
                    }

                    // marginFrame++;
                    // marginFrame = (marginFrame + 1) % TP_FIR_LEN;
                    marginFrame = (marginFrame == (TP_FIR_LEN - 1) ? 0 : marginFrame + 1);
                    chess_memory_fence(); // to make sure data is written before it get read immediately?
                }
            else {
                // Margin has been copied externally and is as part of the window
                marginFrame = 0;
            }

            // pointer to a vector of coefficients
            coeffVectPtr = (coeffVect_t*)(coeffPtr);
            for (unsigned int tdm = 0; tdm < TP_TDM_LOOP_SIZE; tdm++)
                chess_prepare_for_pipelining chess_loop_range((TP_TDM_LOOP_SIZE), (TP_TDM_LOOP_SIZE)) {
                    // m_internalTapsCopy = chess_copy(m_internalTapsCopy);

                    int j = 0;

                    if
                        constexpr(columnMultiple == 1) {
                            dataVect.insert(0, *inRdItr);
                            inRdItr += TP_TDM_LOOP_SIZE;
                        }
                    if
                        constexpr(columnMultiple == 2) {
                            dataVect.insert(0, *inRdItr);
                            inRdItr += TP_TDM_LOOP_SIZE;
                            dataVect.insert(1, *inRdItr);
                            inRdItr += TP_TDM_LOOP_SIZE;
                        }

                    // Make sure data and coeff vectors are of the same sample size, i.e. both have e.g. 8 samples.
                    coeffVect = *coeffVectPtr++;

                    acc = mulTdm(acc, dataVect, coeffVect);

#pragma unroll(TP_FIR_RANGE_LEN / columnMultiple)
                    for (unsigned int j = 1; j < (TP_FIR_RANGE_LEN / columnMultiple); j++) {
                        if
                            constexpr(columnMultiple == 1) {
                                dataVect.insert(0, *inRdItr);
                                inRdItr += TP_TDM_LOOP_SIZE;
                            }
                        if
                            constexpr(columnMultiple == 2) {
                                dataVect.insert(0, *inRdItr);
                                inRdItr += TP_TDM_LOOP_SIZE;
                                dataVect.insert(1, *inRdItr);
                                inRdItr += TP_TDM_LOOP_SIZE;
                            }

                        // Make sure data and coeff vectors are of the same sample size, i.e. both have e.g. 8 samples.
                        // Each load is 256 bit? so schedule load every single or every second iteration.
                        // Alternatively, cover the difference with different loads, e.g. 256-bit load and a 128-bit
                        // load
                        // and schedule both on each iteration.
                        coeffVect = *coeffVectPtr++;
                        acc = macTdm(acc, dataVect, coeffVect);
                    }

                    // output to outVect or to outCascade
                    if
                        constexpr(TP_CASC_OUT == CASC_OUT_FALSE) {
                            outVect = acc.template to_vector<TT_DATA>(TP_SHIFT);
                            // outVect2 = acc2.template to_vector<TT_DATA>(TP_SHIFT);
                            *outPtr++ = outVect;
                            // *outPtr++ = outVect2;
                        }
                    else {
                        writeincr(outInterface.outCascade, acc);
                        // writeincr(outInterface.outCascade, acc2);
                    }
                    // Jump to data of next frame
                    inRdItr++;
                }
        }
};
#undef _DSPLIB_FIR_TDM_HPP_DEBUG_

// FIR filter function overloaded with cascade interface variations
// This is a specialization of the main class for when there is only one kernel for the whole filter.
// Static coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_FIR_RANGE_LEN,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_CASC_LEN,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          int TP_MODIFY_MARGIN_OFFSET,
          unsigned int TP_COEFF_PHASE,
          unsigned int TP_COEFF_PHASE_OFFSET,
          unsigned int TP_COEFF_PHASES,
          unsigned int TP_COEFF_PHASES_LEN,
          unsigned int TP_SAT>
void fir_tdm<TT_DATA,
             TT_COEFF,
             TP_FIR_LEN,
             TP_SHIFT,
             TP_RND,
             TP_INPUT_WINDOW_VSIZE,
             TP_TDM_CHANNELS,
             TP_CASC_IN,
             TP_CASC_OUT,
             TP_FIR_RANGE_LEN,
             TP_KERNEL_POSITION,
             TP_CASC_LEN,
             TP_USE_COEFF_RELOAD,
             TP_NUM_OUTPUTS,
             TP_DUAL_IP,
             TP_API,
             TP_MODIFY_MARGIN_OFFSET,
             TP_COEFF_PHASE,
             TP_COEFF_PHASE_OFFSET,
             TP_COEFF_PHASES,
             TP_COEFF_PHASES_LEN,
             TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                   extents<inherited_extent>,
                                                   margin<thisKernelFilterClass::get_margin()> >& __restrict inWindow,
                             output_circular_buffer<TT_DATA>& __restrict outWindow) {
    T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface;
    T_outputIF<TP_CASC_OUT, TT_DATA> outInterface;
    constexpr int kdummy = 16;
    inInterface.inWindow = inWindow.data();
    outInterface.outWindowPtr = outWindow.data();
    this->filterKernel(inInterface, outInterface);
};
}
}
}
}
}
