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


#include "WidgetBase.h"

#include <Core/OrthancException.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Logging.h>

namespace OrthancStone
{
  void WidgetBase::NotifyContentChanged()
  {
    if (parent_ != NULL)
    {
      parent_->NotifyContentChanged();
    }

    if (viewport_ != NULL)
    {
      viewport_->NotifyContentChanged(*this);
    }
  }


  void WidgetBase::SetParent(IWidget& parent)
  {
    if (parent_ != NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      parent_ = &parent;
    }
  }    

  
  void WidgetBase::ClearBackgroundOrthanc(Orthanc::ImageAccessor& target) const 
  {
    // Clear the background using Orthanc

    if (backgroundCleared_)
    {
      Orthanc::ImageProcessing::Set(target, 
                                    backgroundColor_[0],
                                    backgroundColor_[1],
                                    backgroundColor_[2],
                                    255 /* alpha */);
    }
  }


  void WidgetBase::ClearBackgroundCairo(CairoContext& context) const
  {
    // Clear the background using Cairo

    if (IsBackgroundCleared())
    {
      uint8_t red, green, blue;
      GetBackgroundColor(red, green, blue);

      context.SetSourceColor(red, green, blue);
      cairo_paint(context.GetObject());
    }
  }


  void WidgetBase::ClearBackgroundCairo(Orthanc::ImageAccessor& target) const
  {
    CairoSurface surface(target);
    CairoContext context(surface);
    ClearBackgroundCairo(context);
  }


  void WidgetBase::UpdateStatusBar(const std::string& message)
  {
    if (statusBar_ != NULL)
    {
      statusBar_->SetMessage(message);
    }
  }


  WidgetBase::WidgetBase(const std::string& name) :
    parent_(NULL),
    viewport_(NULL),
    statusBar_(NULL),
    backgroundCleared_(false),
    transmitMouseOver_(false),
    name_(name)
  {
    backgroundColor_[0] = 0;
    backgroundColor_[1] = 0;
    backgroundColor_[2] = 0;
  }


  void WidgetBase::SetViewport(IViewport& viewport)
  {
    if (viewport_ != NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      viewport_ = &viewport;
    }
  }

  
  void WidgetBase::SetBackgroundColor(uint8_t red,
                                      uint8_t green,
                                      uint8_t blue)
  {
    backgroundColor_[0] = red;
    backgroundColor_[1] = green;
    backgroundColor_[2] = blue;
  }

  void WidgetBase::GetBackgroundColor(uint8_t& red,
                                      uint8_t& green,
                                      uint8_t& blue) const
  {
    red = backgroundColor_[0];
    green = backgroundColor_[1];
    blue = backgroundColor_[2];
  }


  bool WidgetBase::Render(Orthanc::ImageAccessor& surface)
  {
#if 0
    ClearBackgroundOrthanc(surface);
#else
    ClearBackgroundCairo(surface);  // Faster than Orthanc
#endif

    return true;
  }

  
  void WidgetBase::UpdateContent()
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
}
