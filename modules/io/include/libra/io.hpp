#pragma once
#include "io/IRecorder.hpp"

#ifdef WITH_MYNTEYE_DEPTH
#include "io/MyntEyeRecorder.h"
#endif

#ifdef WITH_ZED_OPEN
#include "io/ZedOpenRecorder.h"
#endif