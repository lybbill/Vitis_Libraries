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
#ifndef _DSPLIB_FIR_TDM_GRAPH_HPP_
#define _DSPLIB_FIR_TDM_GRAPH_HPP_

/**
 * @file fir_tdm_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "fir_graph_utils.hpp"
#include <tuple>
#include <typeinfo>
#include "fir_tdm.hpp"
#include "fir_common_traits.hpp"
#include "fir_utils.hpp"
#include <adf/arch/aie_arch_properties.hpp>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// fir_tdm_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief fir_tdm is a Time-Division Multiplexing (TDM) FIR filter
 *
 * @ingroup fir_graphs
 *
 * These are the templates to configure the TDM FIR class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the filter function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_COEFF describes the type of individual coefficients of the filter
 *         taps. \n It must be one of the same set of types listed for TT_DATA
 *         and must also satisfy the following rules:
 *         - Complex types are only supported when TT_DATA is also complex.
 *         - TT_COEFF must be an integer type if TT_DATA is an integer type
 *         - TT_COEFF must be a float type if TT_DATA is a float type.
 * @tparam TP_FIR_LEN is an unsigned integer which describes the number of taps
 *         in the filter.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. \n TP_SHIFT must be in the range 0 to 61.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although, TP_RND accepts unsigned integer values descriptive macros are recommended where
 *         - rnd_floor      = Truncate LSB, always round down (towards negative infinity).
 *         - rnd_ceil       = Always round up (towards positive infinity).
 *         - rnd_sym_floor  = Truncate LSB, always round towards 0.
 *         - rnd_sym_ceil   = Always round up towards infinity.
 *         - rnd_pos_inf    = Round halfway towards positive infinity.
 *         - rnd_neg_inf    = Round halfway towards negative infinity.
 *         - rnd_sym_inf    = Round halfway towards infinity (away from zero).
 *         - rnd_sym_zero   = Round halfway towards zero (away from infinity).
 *         - rnd_conv_even  = Round halfway towards nearest even number.
 *         - rnd_conv_odd   = Round halfway towards nearest odd number. \n
 *         No rounding is performed on ceil or floor mode variants. \n
 *         Other modes round to the nearest integer. They differ only in how
 *         they round for values of 0.5. \n
 *
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML device. \n
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples processed by the graph
 *         in a single iteration run.  \n
 *         Samples are buffered and stored in a ping-pong window buffer mapped onto Memory Group banks. \n
 *
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE. \n
 *
 *         Note: ``TP_INPUT_WINDOW_VSIZE`` must be an integer multiple of number
 *         of TDM Channels ``TP_TDM_CHANNELS``. \n
 * @tparam TP_TDM_CHANNELS describes the number of TDM Channels processed by the FIR. \n
 *         Each kernel requires storage for all taps and all channels it is required to operate on,
 *         i.e. requires storage for: ``TP_FIR_LEN * TP_TDM_CHANNELS``.  \n
 *
 *         Note: For SSR configurations (TP_SSR>1), TDM Channels coefficients will be split over multiple paths,
 *         in a round-robin fashion. \n
 * @tparam TP_NUM_OUTPUTS sets the number of ports to broadcast the output to. \n
 *
 *         Note: Dual output ports are not supported at this time.
 * @tparam TP_DUAL_IP allows 2 stream inputs to be connected to FIR, increasing available throughput. \n
 *
 *         Note: Dual input ports are not supported at this time.
 * @tparam TP_SSR specifies the number of parallel input/output paths where samples are interleaved between paths,
 *         giving an overall higher throughput.   \n
 *         A TP_SSR of 1 means just one output leg and 1 input phase. \n
 *         The number of AIE kernels created is equal to ``TP_SSR``. \n
 *         For SSR configurations (TP_SSR>1), the input data must be split over multiple ports,
 *         where each successive sample is sent to a different input port in a round-robin fashion. \n
 *         Each path will have a dedicated input buffer of size defined by ``TP_INPUT_WINDOW_VSIZE / TP_SSR``.  \n
 *         In addition, TDM Channels coefficients will be split over multiple paths, in a round-robin fashion. \n
 *         Finally, computed output samples will also be split over SSR number of output paths. \n
 *         Each path will have a dedicated output buffer of size defined by ``TP_INPUT_WINDOW_VSIZE / TP_SSR``.  \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 **/

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS = 1,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_DUAL_IP = 0,
          //   unsigned int TP_API = 0,
          unsigned int TP_SSR = 1,
          unsigned int TP_SAT = 1>
/**
 **/
class fir_tdm_graph : public graph {
   private:
    static constexpr unsigned int TP_CASC_LEN = 1; // auto-calculated based on FIR length?
    static constexpr unsigned int TP_USE_COEFF_RELOAD = 0;
    static constexpr unsigned int TP_API = 0;
    // static constexpr unsigned int TP_DUAL_IP = 0;
    // static constexpr unsigned int TP_NUM_OUTPUTS = 1;

    static constexpr bool TP_CASC_IN = CASC_IN_FALSE;   // should be unsigned int if exposed on graph interface
    static constexpr bool TP_CASC_OUT = CASC_OUT_FALSE; // should be unsigned int if exposed on graph interface
    // 3d array for storing net information.
    // address[inPhase][outPath][cascPos]
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net;
    std::array<std::array<std::array<connect<stream, stream>*, TP_CASC_LEN>, TP_SSR>, TP_SSR> net2;

#if __HAS_ACCUM_PERMUTES__ == 1
    // cint16/int16 combo can be overloaded with 2 column MUL/MACs.
    static constexpr unsigned int columnMultiple =
        (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int16>::value) ? 2 : 1;
#else
    static constexpr unsigned int columnMultiple = 1;
#endif

    template <int dim, int firlen = TP_FIR_LEN>
    struct ssr_params : public fir_params_defaults {
        static constexpr int Bdim = dim;
        using BTT_DATA = TT_DATA;
        using BTT_COEFF = TT_COEFF;
        static constexpr int BTP_FIR_LEN = firlen;
        static constexpr int BTP_SHIFT = TP_SHIFT;
        static constexpr int BTP_RND = TP_RND;
        static constexpr int BTP_INPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / TP_SSR;
        static constexpr int BTP_CASC_LEN = TP_CASC_LEN;
        static constexpr int BTP_TDM_CHANNELS = TP_TDM_CHANNELS;
        static constexpr int BTP_USE_COEFF_RELOAD = TP_USE_COEFF_RELOAD;
        static constexpr int BTP_NUM_OUTPUTS = TP_NUM_OUTPUTS;
        static constexpr int BTP_DUAL_IP = TP_DUAL_IP;
        static constexpr int BTP_API = TP_API;
        static constexpr int BTP_SSR = TP_SSR;
        static constexpr int BTP_COEFF_PHASES = TP_SSR;
        static constexpr int BTP_COEFF_PHASES_LEN = firlen;
        static constexpr int BTP_CASC_IN = TP_CASC_IN;
        static constexpr int BTP_CASC_OUT = TP_CASC_OUT;
        static constexpr int BTP_FIR_RANGE_LEN = 1;
        static constexpr int BTP_SAT = TP_SAT;
        // 0 - default: decompose to array of TP_SSR^2; 1 - decompose to a vector of TP_SSR, where kernels form
        // independent paths; otherwise set it to 1.
        static constexpr int BTP_SSR_MODE = 1;
    };
    template <int ssr_dim>
    using ssrKernelLookup = ssr_kernels<ssr_params<ssr_dim>, fir_tdm_tl>;
    using lastSSRKernel = ssrKernelLookup<(TP_SSR)-1>; // TP_SSR vector

    struct first_casc_params : public ssr_params<0> {
        static constexpr int Bdim = 0;
        static constexpr int BTP_FIR_LEN = CEIL(TP_FIR_LEN, TP_SSR) / TP_SSR;
        static constexpr int BTP_FIR_RANGE_LEN =
            fir_tdm_tl<ssr_params<0, BTP_FIR_LEN> >::template getKernelFirRangeLen<0>();
    };

    using first_casc_kernel_in_first_ssr = fir_tdm_tl<first_casc_params>;

    static_assert(
        (TP_FIR_LEN % columnMultiple == 0),
        "ERROR: Unsupported FIR length. TP_FIR_LEN must be divisible by 2 for this data & coeff type combination.");
    static_assert((TP_DUAL_IP == 0),
                  "ERROR: Dual input ports are currently not supported. Please set TP_DUAL_IP to 0.");
    static_assert((TP_NUM_OUTPUTS == 1),
                  "ERROR: Dual output ports are currently not supported. Please set TP_NUM_OUTPUTS to 1.");

    static_assert(TP_SSR >= 1, "ERROR: TP_SSR must be 1 or higher");

    static_assert(TP_FIR_LEN % TP_SSR == 0, "ERROR: TP_FIR LEN must be divisible by TP_SSR"); //

    static_assert(TP_CASC_LEN <= 40, "ERROR: Unsupported Cascade length");

    static_assert(TP_INPUT_WINDOW_VSIZE % TP_SSR == 0,
                  "ERROR: Unsupported frame size. TP_INPUT_WINDOW_VSIZE must be divisible by TP_SSR");
    static_assert(TP_INPUT_WINDOW_VSIZE % TP_TDM_CHANNELS == 0,
                  "ERROR: Unsupported frame size. TP_INPUT_WINDOW_VSIZE must be divisible by TP_TDM_CHANNELS");
    // Dual input streams offer no throughput gain if only single output stream would be used.
    // Therefore, dual input streams are only supported with 2 output streams.
    static_assert(TP_NUM_OUTPUTS > TP_DUAL_IP,
                  "ERROR: Dual input streams only supported when number of output streams is also 2. ");
    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(TP_API == USE_STREAM_API || TP_DUAL_IP == DUAL_IP_SINGLE,
                  "ERROR: Dual input ports only supported when port API is a stream. ");
    // Dual input ports offer no throughput gain if port api is windows.
    // Therefore, dual input ports are only supported with streams and not windows.
    static_assert(not(TP_API == USE_WINDOW_API && TP_SSR > 1 && TP_NUM_OUTPUTS == 2),
                  "ERROR: Dual output ports is not supported when port API is a window and SSR > 1. ");

    static constexpr unsigned int kMaxTapsPerKernel = 16384 / sizeof(TT_COEFF); // 8kB 16 kB?
    // Limit FIR length per kernel. Longer FIRs may exceed Program Memory and/or system memory combined with window
    // buffers may exceed Memory Module size
    static_assert(((TP_FIR_LEN * TP_TDM_CHANNELS) / TP_SSR) / TP_CASC_LEN <= kMaxTapsPerKernel,
                  "ERROR: Requested FIR length and Cascade length exceeds supported number of taps per kernel. Please "
                  "increase the cascade length to accommodate the FIR design.");

    static constexpr unsigned int kMemoryModuleSize = 32768;
    static constexpr unsigned int bufferSize =
        (((TP_FIR_LEN / TP_SSR) + TP_INPUT_WINDOW_VSIZE / TP_SSR) * sizeof(TT_DATA));
    // Requested Window buffer exceeds memory module size
    static_assert(TP_API != 0 || bufferSize < kMemoryModuleSize,
                  "ERROR: Input Window size (based on requested window size and FIR length margin) exceeds Memory "
                  "Module size of 32kB");

    static std::vector<TT_COEFF> revert_channel_taps(const std::vector<TT_COEFF>& taps) {
        // processed output
        std::vector<TT_COEFF> revertedTaps;
        // split the array so each kernel has a lane-worth of coeffs on each read/operation.
        unsigned int lanes = TP_SSR * fir_tdm_tl<ssr_params<0> >::getLanes();

        // Input:
        // 30, 31, 32, 33, 34, 35, 36, 37
        // 20, 21, 22, 22, 24, 25, 26, 27
        // 10, 11, 12, 11, 14, 15, 16, 17
        // 00, 01, 02, 00, 04, 05, 06, 07
        // Output:
        // need to split M number of channels into chunks of vector lanes.
        // This way, the coeff readout will not require any extra processing on kernel side.
        // 00, 01, 02, 03,
        // 10, 11, 12, 13,
        // 20, 21, 22, 23,
        // 30, 31, 32, 33,

        // 04, 05, 06, 07
        // 14, 15, 16, 17
        // 24, 25, 26, 27
        // 34, 35, 36, 37

        for (unsigned int channels = 0; channels < TP_TDM_CHANNELS / lanes; channels++) {
            for (unsigned int coeffNo = 0; coeffNo < TP_FIR_LEN; coeffNo++) {
                for (unsigned int laneNo = 0; laneNo < lanes; laneNo++) {
                    int coeffIndex = (TP_FIR_LEN * TP_TDM_CHANNELS) - TP_TDM_CHANNELS - coeffNo * TP_TDM_CHANNELS +
                                     laneNo + channels * lanes;
                    revertedTaps.push_back(taps.at(coeffIndex));
                }
            }
        }
#define _FIR_TDM_DEBUG_
        return revertedTaps;
    }

    /**
     * The conditional input array data to the function.
     * This input is (generated when TP_CASC_IN == CASC_IN_TRUE) either a cascade input.
     **/
    port_conditional_array<output, (TP_CASC_IN == CASC_IN_TRUE), TP_SSR> casc_in;

   public:
    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     **/
    kernel m_firKernels[TP_SSR];

    /**
     * The input data array to the function. This input array is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Note: Margin is added internally to the graph, when connecting input port
     * with kernel port. Therefore, margin should not be added when connecting
     * graph to a higher level design unit.
     * Margin size (in Bytes) equals to TP_FIR_LEN rounded up to a nearest
     * multiple of 32 bytes.
     **/
    port_array<input, TP_SSR> in;

    /**
     * The output data array from the function. This output is either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_array<output, TP_SSR> out;

   private:
    // Hide currently unused ports.

    /**
     * The conditional input array data to the function.
     * This input is (generated when TP_DUAL_IP == 1) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     *
     **/
    port_conditional_array<input, (TP_DUAL_IP == 1), TP_SSR> in2;

    /**
     * The conditional array of input async ports used to pass run-time programmable (RTP) coefficients.
     * This port_conditional_array is (generated when TP_USE_COEFF_RELOAD == 1) an array of input ports, which size is
     *defined by TP_SSR.
     * Each port in the array holds a duplicate of the coefficient array, required to connect to each SSR input path.
     **/
    port_conditional_array<input, (TP_USE_COEFF_RELOAD == 1), TP_SSR> coeff;

    /**
     * The output data array from the function.
     * This output is (generated when TP_NUM_OUTPUTS == 2) either a window API of
     * samples of TT_DATA type or stream API (depending on TP_API).
     * Number of output samples is determined by interpolation & decimation factors (if present).
     **/
    port_conditional_array<output, (TP_NUM_OUTPUTS == 2), TP_SSR> out2;

   public:
    /**
     * Access function to get pointer to kernel (or first kernel in a chained and/or SSR configurations).
     * No arguments required.
     **/
    kernel* getKernels() { return m_firKernels; };

    /**
     * Access function to get pointer to an indexed kernel.
     * @param[in] ssrIndex      an index to the SSR data Path.
     **/
    kernel* getKernels(int ssrIndex) { return m_firKernels + ssrIndex; };

    /**
    * @brief Access function to get kernel's architecture (or first kernel's architecture in a chained configuration).
    **/
    unsigned int getKernelArchs() {
        // return the architecture for first kernel in the design (only one for single kernel designs).
        // First kernel will always be the slowest of the kernels and so it will reflect on the designs performance
        // best.
        return first_casc_kernel_in_first_ssr::parent_class::get_m_kArch();
    };

    /**
     * @brief This is the constructor function for the FIR graph with static coefficients.
     * @param[in] taps   a reference to the std::vector array of taps values of type TT_COEFF.
     * Coefficients are expected to be in a form that presents each tap for all TDM channels, followed by next tap for
     *all TDM channels. For example, a 4 tap, 8 TDM channel vector would look like like the vector below:
     * @code {.c++}
     * std::vector<int16> taps {
     *            t30, t31, t32, t33, t34, t35, t36, t37,
     *            t20, t21, t22, t22, t24, t25, t26, t27,
     *            t10, t11, t12, t11, t14, t15, t16, t17,
     *            t00, t01, t02, t00, t04, t05, t06, t07}
     * @endcode
     * where each element ``tnm`` represents a ''n-th'' tap for a ''m-th'' TDM channel.
     **/
    fir_tdm_graph(const std::vector<TT_COEFF>& taps) {
        lastSSRKernel::create_and_recurse(m_firKernels, revert_channel_taps(taps));
        lastSSRKernel::create_tdm_connections(m_firKernels, &in[0], in2, &out[0], out2, coeff, net, net2, casc_in,
                                              "fir_tdm.cpp");
    };
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_FIR_TDM_GRAPH_HPP_
