

from aie_common import *
import json
import sys

# static_assert(fnCheckDataType<TT_DATA>() ,"ERROR: TT_IN_DATA is not a supported type");
# static_assert(fnCheckTwiddleType<TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is not a supported type");
# static_assert(fnCheckDataTwiddleType<TT_DATA,TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is incompatible with data type");
# static_assert(fnCheckPointSize<TP_POINT_SIZE>(),"ERROR: TP_POINT_SIZE is not a supported value {4-128");
# static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1,"ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
# static_assert(fnCheckShift<TP_SHIFT>(), "ERROR: TP_SHIFT is out of range (0 to 60)");
# static_assert(fnCheckShiftFloat<TT_DATA,TP_SHIFT>(), "ERROR: TP_SHIFT is ignored for data type cfloat so must be set to 0");

# Validate Twiddle type 
def fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE):
  validTypeCombos = [
      ("cint16", "cint16"),
      ("cint32", "cint16"),
      ("cfloat", "cfloat")
    ]
  return (
    isValid if ((TT_DATA,TT_TWIDDLE) in validTypeCombos)
    else (
    isError(f"Invalid Data/Twiddle type combination ({TT_DATA},{TT_TWIDDLE}). ")
    )
  )
def validate_TT_TWIDDLE(args):
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  return fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE)

# Validate point size
def fn_validate_point_size(TP_POINT_SIZE):
  return (
    isValid if (TP_POINT_SIZE >= 8 and TP_POINT_SIZE <=128)
    else (
    isError(f"Invalid Point Size ({TP_POINT_SIZE}). ")
    )
  )
def validate_TP_POINT_SIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  return fn_validate_point_size(TP_POINT_SIZE)

# Validate SHIFT
def validate_TP_SHIFT(args):
  TP_SHIFT = args["TP_SHIFT"]
  TT_DATA = args["TT_DATA"]
  return fn_validate_shift(TT_DATA, TP_SHIFT)

# Validate CASC_LEN
def fn_validate_casc_len(TP_POINT_SIZE, TP_NUM_FRAMES, TP_CASC_LEN):
  return (
    isValid if ((TP_NUM_FRAMES*TP_POINT_SIZE)/TP_CASC_LEN >= 4)
    else (
    isError(f"Invalid CASC_LEN ({TP_CASC_LEN}). ")
    )
  )
def validate_TP_CASC_LEN(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  
  return fn_validate_casc_len(TP_POINT_SIZE, TP_NUM_FRAMES, TP_CASC_LEN)



  ######### Finished Validation ###########

  ######### Updators ###########

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.
def get_window_sizes(TT_DATA,TP_POINT_SIZE,TP_NUM_FRAMES,TP_CASC_LEN ):
  kSamplesInVect = 256/8/fn_size_by_byte(TT_DATA)
  paddedDataSize = CEIL(TP_POINT_SIZE, kSamplesInVect)
  OUT_WINDOW_VSIZE = paddedDataSize*TP_NUM_FRAMES
  PAD_WINDOW_VSIZE = (CEIL(paddedDataSize,(kSamplesInVect*TP_CASC_LEN))*TP_NUM_FRAMES)
  CASC_WINDOW_VSIZE = PAD_WINDOW_VSIZE/TP_CASC_LEN

  return OUT_WINDOW_VSIZE, CASC_WINDOW_VSIZE

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has dynamic number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]  
  
  OUT_WINDOW_VSIZE, CASC_WINDOW_VSIZE = get_window_sizes(TT_DATA,TP_POINT_SIZE,TP_NUM_FRAMES,TP_CASC_LEN)


  in_ports = get_port_info("in", "in", TT_DATA, CASC_WINDOW_VSIZE, TP_CASC_LEN, 0)
  out_ports = get_port_info("out", "out", TT_DATA, OUT_WINDOW_VSIZE, 1, 0)
  return in_ports + out_ports

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_NUM_FRAMES = args["TP_NUM_FRAMES"]


  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
  template <typename dir>

  std::array<adf::port<input>,TP_CASC_LEN> in;
  adf::port<output> out;


  xf::dsp::aie::fft::dft::dft_graph<
    {TT_DATA}, // TT_DATA
    {TT_TWIDDLE}, // TT_TWIDDLE
    {TP_POINT_SIZE}, // TP_POINT_SIZE
    {TP_FFT_NIFFT}, // TP_FFT_NIFFT
    {TP_SHIFT}, // TP_SHIFT
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_NUM_FRAMES} //TP_NUM_FRAMES

  > dft_graph;

  {graphname}() : dft_graph() {{
    for (int i=0; i < TP_CASC_LEN; i++) {{
      adf::connect<> net_in(in[i], dft_graph.in[i]);
    }}
    adf::connect<> net_out(dft_graph.out[TP_CASC_LEN-1], out);
  }}
}};
"""
  )
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "dft_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out
