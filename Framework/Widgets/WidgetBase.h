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

namespace OrthancStone
{
  class WidgetBase : public IWidget
  {
  private:
    IWidget*     parent_;
    IViewport*   viewport_;
    IStatusBar*  statusBar_;
    bool         backgroundCleared_;
    uint8_t      backgroundColor_[3];
    bool         transmitMouseOver_;

  protected:
    void ClearBackgroundOrthanc(Orthanc::ImageAccessor& target) const;

    void ClearBackgroundCairo(CairoContext& context) const;

    void ClearBackgroundCairo(Orthanc::ImageAccessor& target) const;

    void UpdateStatusBar(const std::string& message);

    IStatusBar* GetStatusBar() const
    {
      return statusBar_;
    }

  public:
    WidgetBase();

    virtual void SetDefaultView()
    {
    }
  
    virtual void SetParent(IWidget& parent);
    
    virtual void SetViewport(IViewport& viewport);

    void SetBackgroundCleared(bool clear)
    {
      backgroundCleared_ = clear;
    }

    bool IsBackgroundCleared() const
    {
      return backgroundCleared_;
    }

    void SetTransmitMouseOver(bool transmit)
    {
      transmitMouseOver_ = transmit;
    }

    void SetBackgroundColor(uint8_t red,
                            uint8_t green,
                            uint8_t blue);

    void GetBackgroundColor(uint8_t& red,
                            uint8_t& green,
                            uint8_t& blue) const;

    virtual void SetStatusBar(IStatusBar& statusBar)
    {
      statusBar_ = &statusBar;
    }

    virtual bool Render(Orthanc::ImageAccessor& surface);

    virtual bool HasUpdateContent() const
    {
      return false;
    }

    virtual void UpdateContent();

    virtual bool HasRenderMouseOver()
    {
      return transmitMouseOver_;
    }

    virtual void NotifyChange();

    virtual void Start()
    {
    }
  };
}
