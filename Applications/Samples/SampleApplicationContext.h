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


#pragma once

#include "../../Framework/Viewport/WidgetViewport.h"
#include "../../Framework/Volumes/ISlicedVolume.h"
#include "../../Framework/Volumes/IVolumeLoader.h"
#include "../../Framework/Widgets/IWorldSceneInteractor.h"
#include "../../Platforms/Generic/OracleWebService.h"
#include "../BasicApplicationContext.h"

#include <list>
#include <boost/thread.hpp>


#if ORTHANC_ENABLE_SDL==1
#include "../Sdl/BasicSdlApplicationContext.h"
#else
#include "../Wasm/BasicWasmApplicationContext.h"
#endif

namespace OrthancStone
{

//#if ORTHANC_ENABLE_SDL==1
//  class SampleApplicationContext : public BasicSdlApplicationContext
//#else
//  class SampleApplicationContext : public BasicWasmApplicationContext
//#endif
  class SampleApplicationContext : public BasicWasmApplicationContext
  {
  private:
    typedef std::list<ISlicedVolume*>          SlicedVolumes;  // this is actually used by the samples and shall be moved to a SampleApplicationContext
    typedef std::list<IVolumeLoader*>          VolumeLoaders;
    typedef std::list<IWorldSceneInteractor*>  Interactors;

    SlicedVolumes       slicedVolumes_;
    VolumeLoaders       volumeLoaders_;
    Interactors         interactors_;

  public:

#if ORTHANC_ENABLE_SDL==1
    SampleApplicationContext(Orthanc::WebServiceParameters& orthanc, WidgetViewport* centralViewport)
    : BasicSdlApplicationContext(orthanc, centralViewport) {
    }
#else
    SampleApplicationContext(IWebService& webService)
    : BasicWasmApplicationContext(webService) {
    }
#endif

    virtual ~SampleApplicationContext();

    ISlicedVolume& AddSlicedVolume(ISlicedVolume* volume);

    IVolumeLoader& AddVolumeLoader(IVolumeLoader* loader);

    IWorldSceneInteractor& AddInteractor(IWorldSceneInteractor* interactor);
  };
}
