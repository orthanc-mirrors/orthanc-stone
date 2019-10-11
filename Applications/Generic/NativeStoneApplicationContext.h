/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "../../Framework/Deprecated/Viewport/WidgetViewport.h"
#include "../../Framework/Deprecated/Volumes/ISlicedVolume.h"
#include "../../Framework/Deprecated/Volumes/IVolumeLoader.h"

#include <list>
#include <boost/thread.hpp>
#include "../StoneApplicationContext.h"

namespace OrthancStone
{
  class NativeStoneApplicationContext : public StoneApplicationContext
  {
  private:
    static void UpdateThread(NativeStoneApplicationContext* that);

    boost::recursive_mutex    globalMutex_;
    Deprecated::WidgetViewport  centralViewport_;
    boost::thread   updateThread_;
    bool            stopped_;
    unsigned int    updateDelayInMs_;

  public:
    class GlobalMutexLocker: public boost::noncopyable
    {
    private:
      NativeStoneApplicationContext&        that_;
      boost::recursive_mutex::scoped_lock   lock_;
      
    public:
      GlobalMutexLocker(NativeStoneApplicationContext& that) :
        that_(that),
        lock_(that.globalMutex_)
      {
      }

      Deprecated::IWidget& SetCentralWidget(Deprecated::IWidget* widget);   // Takes ownership

      Deprecated::IViewport& GetCentralViewport() 
      {
        return that_.centralViewport_;
      }

      void SetUpdateDelay(unsigned int delayInMs)
      {
        that_.updateDelayInMs_ = delayInMs;
      }
    };

    NativeStoneApplicationContext();

    void Start();

    void Stop();
  };
}
