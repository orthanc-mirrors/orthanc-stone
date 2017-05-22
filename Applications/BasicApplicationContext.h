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
#include "../../Framework/Toolbox/OrthancSynchronousWebService.h"

#include <list>
#include <boost/thread.hpp>

namespace OrthancStone
{
  class BasicApplicationContext : public boost::noncopyable
  {
  private:
    typedef std::list<ISliceableVolume*>       Volumes;
    typedef std::list<IWorldSceneInteractor*>  Interactors;
    typedef std::list<DicomStructureSet*>      StructureSets;

    static void UpdateThread(BasicApplicationContext* that);

    OrthancSynchronousWebService&  orthanc_;
    boost::mutex        viewportMutex_;
    WidgetViewport      viewport_;
    Volumes             volumes_;
    Interactors         interactors_;
    StructureSets       structureSets_;
    boost::thread       updateThread_;
    bool                stopped_;
    unsigned int        updateDelay_;

  public:
    class ViewportLocker : public boost::noncopyable
    {
    private:
      boost::mutex::scoped_lock  lock_;
      IViewport&                 viewport_;

    public:
      ViewportLocker(BasicApplicationContext& that) :
        lock_(that.viewportMutex_),
        viewport_(that.viewport_)
      {
      }

      IViewport& GetViewport() const
      {
        return viewport_;
      }
    };

    
    BasicApplicationContext(OrthancSynchronousWebService& orthanc);

    ~BasicApplicationContext();

    IWidget& SetCentralWidget(IWidget* widget);   // Takes ownership

    OrthancSynchronousWebService& GetWebService()
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

    void SetUpdateDelay(unsigned int delay)  // In milliseconds
    {
      updateDelay_ = delay;
    }
  };
}
