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


#include "NativeStoneApplicationContext.h"
#include "../../Platforms/Generic/OracleWebService.h"

namespace OrthancStone
{
  void NativeStoneApplicationContext::GlobalMutexLocker::SetCentralWidget(
    boost::shared_ptr<Deprecated::IWidget> widget)
  {
    that_.centralViewport_.SetCentralWidget(widget);
  }


  void NativeStoneApplicationContext::UpdateThread(NativeStoneApplicationContext* that)
  {
    while (!that->stopped_)
    {
      {
        GlobalMutexLocker locker(*that);
        locker.GetCentralViewport().DoAnimation();
      }
      
      boost::this_thread::sleep(boost::posix_time::milliseconds(that->updateDelayInMs_));
    }
  }
  

  NativeStoneApplicationContext::NativeStoneApplicationContext() :
    stopped_(true),
    updateDelayInMs_(100)   // By default, 100ms between each refresh of the content
  {
    srand(static_cast<unsigned int>(time(NULL))); 
  }


  void NativeStoneApplicationContext::Start()
  {
    boost::recursive_mutex::scoped_lock lock(globalMutex_);
    
    if (stopped_ &&
        centralViewport_.HasAnimation())
    {
      stopped_ = false;
      updateThread_ = boost::thread(UpdateThread, this);
    }
  }


  void NativeStoneApplicationContext::Stop()
  {
    stopped_ = true;
    
    if (updateThread_.joinable())
    {
      updateThread_.join();
    }
  }
}
