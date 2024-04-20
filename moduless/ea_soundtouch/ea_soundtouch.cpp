#include "ea_soundtouch.h"

#include "warnings/WarningsStart.h"

#include "source/SoundTouch/BPMDetect.cpp"
#undef max
#include "source/SoundTouch/PeakFinder.cpp"
#undef max
#include "source/SoundTouch/FIFOSampleBuffer.cpp"

#include "source/SoundTouch/AAFilter.cpp"
#undef PI
#include "source/SoundTouch/cpu_detect_x86.cpp"
#include "source/SoundTouch/FIRFilter.cpp"
#include "source/SoundTouch/InterpolateCubic.cpp"
#include "source/SoundTouch/InterpolateLinear.cpp"
#include "source/SoundTouch/InterpolateShannon.cpp"
#include "source/SoundTouch/mmx_optimized.cpp"
#include "source/SoundTouch/RateTransposer.cpp"
#include "source/SoundTouch/SoundTouch.cpp"
#include "source/SoundTouch/sse_optimized.cpp"
#include "source/SoundTouch/TDStretch.cpp"

#include "warnings/WarningsEnd.h"