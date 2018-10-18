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

#include <list>
#include <boost/thread.hpp>
#include "../StoneApplicationContext.h"

namespace OrthancStone
{
  class NativeStoneApplicationContext : public StoneApplicationContext
  {
  private:

    static void UpdateThread(NativeStoneApplicationContext* that);

    boost::mutex                   globalMutex_;
    std::auto_ptr<WidgetViewport>  centralViewport_;
    boost::thread                  updateThread_;
    bool                           stopped_;
    unsigned int                   updateDelayInMs_;

  public:
    class GlobalMutexLocker: public boost::noncopyable
    {
      boost::mutex::scoped_lock  lock_;
    public:
      GlobalMutexLocker(NativeStoneApplicationContext& that):
        lock_(that.globalMutex_)
      {
      }
    };

    NativeStoneApplicationContext();

    virtual ~NativeStoneApplicationContext()
    {
    }

    virtual IWidget& SetCentralWidget(IWidget* widget);   // Takes ownership

    IViewport& GetCentralViewport() 
    {
      return *(centralViewport_.get());
    }

    void Start();

    void Stop();

    void SetUpdateDelay(unsigned int delayInMs)
    {
      updateDelayInMs_ = delayInMs;
    }
  };
}
