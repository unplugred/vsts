#pragma once

#if 0

BEGIN_JUCE_MODULE_DECLARATION

      ID:               ea_soundtouch
      vendor:           Eyal Amir
      version:          0.0.1
      name:             Soundtouch
      description:      Wrapper around the soundtouch library
      website:
      license:          BSD
      dependencies:     

     END_JUCE_MODULE_DECLARATION

#endif

#include <juce_core/system/juce_TargetPlatform.h>

#include "warnings/WarningsStart.h"

#include "include/SoundTouch.h"
#include "include/BPMDetect.h"

#include "warnings/WarningsEnd.h"