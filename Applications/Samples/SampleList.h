// The macro "ORTHANC_STONE_SAMPLE" must be set by the CMake script

#if ORTHANC_STONE_SAMPLE == 1
#include "EmptyApplication.h"
typedef OrthancStone::Samples::EmptyApplication SampleApplication;

#elif ORTHANC_STONE_SAMPLE == 2
#include "TestPatternApplication.h"
typedef OrthancStone::Samples::TestPatternApplication SampleApplication;

#elif ORTHANC_STONE_SAMPLE == 3
#include "SingleFrameApplication.h"
typedef OrthancStone::Samples::SingleFrameApplication SampleApplication;

#elif ORTHANC_STONE_SAMPLE == 4
#include "SingleVolumeApplication.h"
typedef OrthancStone::Samples::SingleVolumeApplication SampleApplication;

#elif ORTHANC_STONE_SAMPLE == 5
#include "BasicPetCtFusionApplication.h"
typedef OrthancStone::Samples::BasicPetCtFusionApplication SampleApplication;

#elif ORTHANC_STONE_SAMPLE == 6
#include "SynchronizedSeriesApplication.h"
typedef OrthancStone::Samples::SynchronizedSeriesApplication SampleApplication;

#elif ORTHANC_STONE_SAMPLE == 7
#include "LayoutPetCtFusionApplication.h"
typedef OrthancStone::Samples::LayoutPetCtFusionApplication SampleApplication;

#elif ORTHANC_STONE_SAMPLE == 8
#include "SimpleViewerApplicationSingleFile.h"
typedef OrthancStone::Samples::SimpleViewerApplication SampleApplication;

#else
#error Please set the ORTHANC_STONE_SAMPLE macro
#endif
