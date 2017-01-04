/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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


#include "WidgetBase.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"
#include "../../Resources/Orthanc/Core/Logging.h"

namespace OrthancStone
{
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


  void WidgetBase::NotifyChange()
  {
    observers_.NotifyChange(this);
  }


  void WidgetBase::UpdateStatusBar(const std::string& message)
  {
    if (statusBar_ != NULL)
    {
      statusBar_->SetMessage(message);
    }
  }


  void WidgetBase::WorkerThread(WidgetBase* that)
  {
    while (that->started_)
    {
      that->UpdateStep();
    }
  }


  WidgetBase::WidgetBase() :
    statusBar_(NULL),
    started_(false),
    backgroundCleared_(false)
  {
    backgroundColor_[0] = 0;
    backgroundColor_[1] = 0;
    backgroundColor_[2] = 0;
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


  void WidgetBase::Register(IChangeObserver& observer)
  {
    observers_.Register(observer);
  }


  void WidgetBase::Unregister(IChangeObserver& observer)
  {
    observers_.Unregister(observer);
  }


  void WidgetBase::Start()
  {
    if (started_)
    {
      LOG(ERROR) << "Cannot Start() twice";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    started_ = true;

    if (HasUpdateThread())
    {
      thread_ = boost::thread(WorkerThread, this);
    }
  }


  void WidgetBase::Stop()
  {
    if (!started_)
    {
      LOG(ERROR) << "Cannot Stop() if Start() has not been invoked";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    started_ = false;

    if (HasUpdateThread() &&
        thread_.joinable())
    {
      thread_.join();
    }
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
}
