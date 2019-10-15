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

#include "SampleApplicationBase.h"

#include "../../Framework/Deprecated/Layers/DicomSeriesVolumeSlicer.h"
#include "../../Framework/Deprecated/Widgets/SliceViewerWidget.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/math/constants/constants.hpp>


namespace OrthancStone
{
  namespace Samples
  {
    class SingleFrameApplication :
      public SampleSingleCanvasApplicationBase,
      public ObserverBase<SingleFrameApplication>
    {
    private:
      class Interactor : public Deprecated::IWorldSceneInteractor
      {
      private:
        SingleFrameApplication&  application_;
        
      public:
        Interactor(SingleFrameApplication&  application) :
          application_(application)
        {
        }
        
        virtual Deprecated::IWorldSceneMouseTracker* CreateMouseTracker(Deprecated::WorldSceneWidget& widget,
                                                                        const Deprecated::ViewportGeometry& view,
                                                                        MouseButton button,
                                                                        KeyboardModifiers modifiers,
                                                                        int viewportX,
                                                                        int viewportY,
                                                                        double x,
                                                                        double y,
                                                                        Deprecated::IStatusBar* statusBar,
                                                                        const std::vector<Deprecated::Touch>& displayTouches)
        {
          return NULL;
        }

        virtual void MouseOver(CairoContext& context,
                               Deprecated::WorldSceneWidget& widget,
                               const Deprecated::ViewportGeometry& view,
                               double x,
                               double y,
                               Deprecated::IStatusBar* statusBar)
        {
          if (statusBar != NULL)
          {
            Vector p = dynamic_cast<Deprecated::SliceViewerWidget&>(widget).GetSlice().MapSliceToWorldCoordinates(x, y);
            
            char buf[64];
            sprintf(buf, "X = %.02f Y = %.02f Z = %.02f (in cm)", 
                    p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
            statusBar->SetMessage(buf);
          }
        }

        virtual void MouseWheel(Deprecated::WorldSceneWidget& widget,
                                MouseWheelDirection direction,
                                KeyboardModifiers modifiers,
                                Deprecated::IStatusBar* statusBar)
        {
          int scale = (modifiers & KeyboardModifiers_Control ? 10 : 1);
          
          switch (direction)
          {
            case MouseWheelDirection_Up:
              application_.OffsetSlice(-scale);
              break;

            case MouseWheelDirection_Down:
              application_.OffsetSlice(scale);
              break;

            default:
              break;
          }
        }

        virtual void KeyPressed(Deprecated::WorldSceneWidget& widget,
                                KeyboardKeys key,
                                char keyChar,
                                KeyboardModifiers modifiers,
                                Deprecated::IStatusBar* statusBar)
        {
          switch (keyChar)
          {
            case 's':
              widget.FitContent();
              break;

            default:
              break;
          }
        }
      };


      void OffsetSlice(int offset)
      {
        if (source_ != NULL)
        {
          int slice = static_cast<int>(slice_) + offset;

          if (slice < 0)
          {
            slice = 0;
          }

          if (slice >= static_cast<int>(source_->GetSlicesCount()))
          {
            slice = static_cast<int>(source_->GetSlicesCount()) - 1;
          }

          if (slice != static_cast<int>(slice_)) 
          {
            SetSlice(slice);
          }   
        }
      }


      Deprecated::SliceViewerWidget& GetMainWidget()
      {
        return *dynamic_cast<Deprecated::SliceViewerWidget*>(mainWidget_);
      }
      

      void SetSlice(size_t index)
      {
        if (source_ != NULL &&
            index < source_->GetSlicesCount())
        {
          slice_ = static_cast<unsigned int>(index);
          
#if 1
          GetMainWidget().SetSlice(source_->GetSlice(slice_).GetGeometry());
#else
          // TEST for scene extents - Rotate the axes
          double a = 15.0 / 180.0 * boost::math::constants::pi<double>();

#if 1
          Vector x; GeometryToolbox::AssignVector(x, cos(a), sin(a), 0);
          Vector y; GeometryToolbox::AssignVector(y, -sin(a), cos(a), 0);
#else
          // Flip the normal
          Vector x; GeometryToolbox::AssignVector(x, cos(a), sin(a), 0);
          Vector y; GeometryToolbox::AssignVector(y, sin(a), -cos(a), 0);
#endif
          
          SliceGeometry s(source_->GetSlice(slice_).GetGeometry().GetOrigin(), x, y);
          widget_->SetSlice(s);
#endif
        }
      }
        
      
      void OnMainWidgetGeometryReady(const Deprecated::IVolumeSlicer::GeometryReadyMessage& message)
      {
        // Once the geometry of the series is downloaded from Orthanc,
        // display its middle slice, and adapt the viewport to fit this
        // slice
        if (source_ == &message.GetOrigin())
        {
          SetSlice(source_->GetSlicesCount() / 2);
        }

        GetMainWidget().FitContent();
      }
      
      std::auto_ptr<Interactor>         mainWidgetInteractor_;
      const Deprecated::DicomSeriesVolumeSlicer*    source_;
      unsigned int                      slice_;

    public:
      SingleFrameApplication() :
        source_(NULL),
        slice_(0)
      {
      }
      
      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("instance", boost::program_options::value<std::string>(), 
           "Orthanc ID of the instance")
          ("frame", boost::program_options::value<unsigned int>()->default_value(0),
           "Number of the frame, for multi-frame DICOM instances")
          ("smooth", boost::program_options::value<bool>()->default_value(true), 
           "Enable bilinear interpolation to smooth the image")
          ;

        options.add(generic);    
      }

      virtual void Initialize(StoneApplicationContext* context,
                              Deprecated::IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        context_ = context;

        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        int frame = parameters["frame"].as<unsigned int>();

        mainWidget_ = new Deprecated::SliceViewerWidget("main-widget");

        std::auto_ptr<Deprecated::DicomSeriesVolumeSlicer> layer(new Deprecated::DicomSeriesVolumeSlicer(context->GetOrthancApiClient()));
        source_ = layer.get();
        layer->LoadFrame(instance, frame);
        Register<Deprecated::IVolumeSlicer::GeometryReadyMessage>(*layer, &SingleFrameApplication::OnMainWidgetGeometryReady);
        GetMainWidget().AddLayer(layer.release());

        Deprecated::RenderStyle s;

        if (parameters["smooth"].as<bool>())
        {
          s.interpolation_ = ImageInterpolation_Bilinear;
        }

        GetMainWidget().SetLayerStyle(0, s);
        GetMainWidget().SetTransmitMouseOver(true);

        mainWidgetInteractor_.reset(new Interactor(*this));
        GetMainWidget().SetInteractor(*mainWidgetInteractor_);
      }
    };


  }
}
