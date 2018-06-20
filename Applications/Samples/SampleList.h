// The macro "ORTHANC_STONE_SAMPLE" must be set by the CMake script

#if ORTHANC_STONE_SAMPLE == 1
#include "EmptyApplication.h"
typedef OrthancStone::Samples::EmptyApplication Application;

#elif ORTHANC_STONE_SAMPLE == 2
#include "TestPatternApplication.h"
typedef OrthancStone::Samples::TestPatternApplication Application;

#elif ORTHANC_STONE_SAMPLE == 3
#include "SingleFrameApplication.h"
typedef OrthancStone::Samples::SingleFrameApplication Application;

#elif ORTHANC_STONE_SAMPLE == 4
#include "SingleVolumeApplication.h"
typedef OrthancStone::Samples::SingleVolumeApplication Application;

#elif ORTHANC_STONE_SAMPLE == 5
#include "BasicPetCtFusionApplication.h"
typedef OrthancStone::Samples::BasicPetCtFusionApplication Application;

#elif ORTHANC_STONE_SAMPLE == 6
#include "SynchronizedSeriesApplication.h"
typedef OrthancStone::Samples::SynchronizedSeriesApplication Application;

#elif ORTHANC_STONE_SAMPLE == 7
#include "LayoutPetCtFusionApplication.h"
typedef OrthancStone::Samples::LayoutPetCtFusionApplication Application;

#elif ORTHANC_STONE_SAMPLE == 8
#include "SimpleViewerApplication.h"
typedef OrthancStone::Samples::SimpleViewerApplication Application;

#else
#error Please set the ORTHANC_STONE_SAMPLE macro
#endif
