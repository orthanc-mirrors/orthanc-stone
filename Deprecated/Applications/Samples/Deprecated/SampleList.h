/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#pragma once

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

#elif ORTHANC_STONE_SAMPLE == 9
#include "SingleFrameEditorApplication.h"
typedef OrthancStone::Samples::SingleFrameEditorApplication SampleApplication;

#else
#error Please set the ORTHANC_STONE_SAMPLE macro
#endif
