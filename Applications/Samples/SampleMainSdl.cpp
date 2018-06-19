/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


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


int main(int argc, char* argv[]) 
{
  Application application;

  return OrthancStone::BasicSdlApplication::ExecuteWithSdl(application, argc, argv);
}
