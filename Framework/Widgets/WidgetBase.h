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

#include "IWidget.h"

#include "../Viewport/CairoContext.h"
#include "../Toolbox/ObserversRegistry.h"

namespace OrthancStone
{
  class WidgetBase : public IWidget
  {
  private:
    IStatusBar*                  statusBar_;
    ObserversRegistry<IWidget>   observers_;
    bool                         started_;
    bool                         backgroundCleared_;
    uint8_t                      backgroundColor_[3];

  protected:
    void ClearBackgroundOrthanc(Orthanc::ImageAccessor& target) const;

    void ClearBackgroundCairo(CairoContext& context) const;

    void ClearBackgroundCairo(Orthanc::ImageAccessor& target) const;

    void NotifyChange();

    void UpdateStatusBar(const std::string& message);

    IStatusBar* GetStatusBar() const
    {
      return statusBar_;
    }

    bool IsStarted() const
    {
      return started_;
    }

  public:
    WidgetBase();

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

    virtual bool HasUpdateContent() const
    {
      return false;
    }

    virtual void UpdateContent();

    virtual bool HasRenderMouseOver(int x,
                                    int y)
    {
      return false;
    }
  };
}
