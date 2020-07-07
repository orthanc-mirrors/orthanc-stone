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

#include "IFlexiblePointerTracker.h"
#include "../Scene2D/IPointerTracker.h"


namespace OrthancStone
{
#if 0
  namespace
  {
    class SimpleTrackerAdapter : public IFlexiblePointerTracker
    {
    public:
      SimpleTrackerAdapter(boost::shared_ptr<IPointerTracker> wrappedTracker)
        : wrappedTracker_(wrappedTracker)
        , active_(true)
      {
      }

      virtual void PointerMove(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        if(active_)
          wrappedTracker_->Update(event);
      };
      virtual void PointerUp(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        if (wrappedTracker_)
        {
          wrappedTracker_->Release();
          wrappedTracker_ = NULL;
        }
        active_ = false;
      }
      virtual void PointerDown(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        // nothing to do atm
      }
      virtual bool IsActive() const ORTHANC_OVERRIDE
      {
        return active_;
      }

      virtual void Cancel() ORTHANC_OVERRIDE
      {
        wrappedTracker_ = NULL;
        active_ = false;
      }

    private:
      boost::shared_ptr<IPointerTracker> wrappedTracker_;
      bool active_;
    };
  }

  boost::shared_ptr<IFlexiblePointerTracker> CreateSimpleTrackerAdapter(boost::shared_ptr<IPointerTracker> t)
  {
    return boost::shared_ptr<IFlexiblePointerTracker>(new SimpleTrackerAdapter(t));
  }
#endif
}
