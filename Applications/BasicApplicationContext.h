/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "../../Framework/Volumes/VolumeImage.h"
#include "../../Framework/Viewport/WidgetViewport.h"
#include "../../Framework/Widgets/IWorldSceneInteractor.h"
#include "../../Framework/Toolbox/DicomStructureSet.h"

#include <list>

namespace OrthancStone
{
  class BasicApplicationContext : public boost::noncopyable
  {
  private:
    typedef std::list<ISliceableVolume*>       Volumes;
    typedef std::list<IWorldSceneInteractor*>  Interactors;
    typedef std::list<DicomStructureSet*>      StructureSets;

    OrthancPlugins::IOrthancConnection&  orthanc_;

    WidgetViewport   viewport_;
    Volumes          volumes_;
    Interactors      interactors_;
    StructureSets    structureSets_;

  public:
    BasicApplicationContext(OrthancPlugins::IOrthancConnection& orthanc);

    ~BasicApplicationContext();

    IWidget& SetCentralWidget(IWidget* widget);   // Takes ownership

    IViewport& GetViewport()
    {
      return viewport_;
    }

    OrthancPlugins::IOrthancConnection& GetOrthancConnection()
    {
      return orthanc_;
    }

    VolumeImage& AddSeriesVolume(const std::string& series,
                                 bool isProgressiveDownload,
                                 size_t downloadThreadCount);

    DicomStructureSet& AddStructureSet(const std::string& instance);

    IWorldSceneInteractor& AddInteractor(IWorldSceneInteractor* interactor);

    void Start();

    void Stop();
  };
}
