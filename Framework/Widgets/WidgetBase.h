/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include "IWidget.h"

#include "../Viewport/CairoContext.h"
#include "../Toolbox/ObserversRegistry.h"

#include <boost/thread.hpp>

namespace OrthancStone
{
  class WidgetBase : public IWidget
  {
  private:
    IStatusBar*                  statusBar_;
    ObserversRegistry<IWidget>   observers_;
    bool                         started_;
    boost::thread                thread_;
    bool                         backgroundCleared_;
    uint8_t                      backgroundColor_[3];

  protected:
    void ClearBackgroundOrthanc(Orthanc::ImageAccessor& target) const;

    void ClearBackgroundCairo(CairoContext& context) const;

    void ClearBackgroundCairo(Orthanc::ImageAccessor& target) const;

    void NotifyChange();

    void UpdateStatusBar(const std::string& message);

    static void WorkerThread(WidgetBase* that);

    IStatusBar* GetStatusBar() const
    {
      return statusBar_;
    }

    virtual bool HasUpdateThread() const = 0;

    virtual void UpdateStep() = 0;

  public:
    WidgetBase();

    bool IsStarted() const
    {
      return started_;
    }

    void SetBackgroundCleared(bool clear)
    {
      backgroundCleared_ = clear;
    }

    bool IsBackgroundCleared() const
    {
      return backgroundCleared_;
    }

    void SetBackgroundColor(uint8_t red,
                            uint8_t green,
                            uint8_t blue);

    void GetBackgroundColor(uint8_t& red,
                            uint8_t& green,
                            uint8_t& blue) const;

    virtual void Register(IChangeObserver& observer);

    virtual void Unregister(IChangeObserver& observer);
    
    virtual void SetStatusBar(IStatusBar& statusBar)
    {
      statusBar_ = &statusBar;
    }

    virtual void ResetStatusBar()
    {
      statusBar_ = NULL;
    }    

    virtual void Start();

    virtual void Stop();

    virtual bool Render(Orthanc::ImageAccessor& surface);
  };
}
