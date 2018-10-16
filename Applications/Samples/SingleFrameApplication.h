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

#include "SampleApplicationBase.h"

#include "../../Framework/Layers/OrthancFrameLayerSource.h"
#include "../../Framework/Widgets/LayerWidget.h"

#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class SingleFrameApplication :
      public SampleSingleCanvasApplicationBase,
      public IObserver
    {
    private:
      class Interactor : public IWorldSceneInteractor
      {
      private:
        SingleFrameApplication&  application_;
        
      public:
        Interactor(SingleFrameApplication&  application) :
          application_(application)
        {
        }
        
        virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                            const ViewportGeometry& view,
                                                            MouseButton button,
                                                            KeyboardModifiers modifiers,
                                                            double x,
                                                            double y,
                                                            IStatusBar* statusBar)
        {
          return NULL;
        }

        virtual void MouseOver(CairoContext& context,
                               WorldSceneWidget& widget,
                               const ViewportGeometry& view,
                               double x,
                               double y,
                               IStatusBar* statusBar)
        {
          if (statusBar != NULL)
          {
            Vector p = dynamic_cast<LayerWidget&>(widget).GetSlice().MapSliceToWorldCoordinates(x, y);
            
            char buf[64];
            sprintf(buf, "X = %.02f Y = %.02f Z = %.02f (in cm)", 
                    p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
            statusBar->SetMessage(buf);
          }
        }

        virtual void MouseWheel(WorldSceneWidget& widget,
                                MouseWheelDirection direction,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
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

        virtual void KeyPressed(WorldSceneWidget& widget,
                                KeyboardKeys key,
                                char keyChar,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
        {
          switch (keyChar)
          {
            case 's':
              widget.SetDefaultView();
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

          if (slice >= static_cast<int>(source_->GetSliceCount()))
          {
            slice = source_->GetSliceCount() - 1;
          }

          if (slice != static_cast<int>(slice_)) 
          {
            SetSlice(slice);
          }   
        }
      }
      

      void SetSlice(size_t index)
      {
        if (source_ != NULL &&
            index < source_->GetSliceCount())
        {
          slice_ = index;
          
#if 1
          mainWidget_->SetSlice(source_->GetSlice(slice_).GetGeometry());
#else
          // TEST for scene extents - Rotate the axes
          double a = 15.0 / 180.0 * M_PI;

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
        
      
      void OnMainWidgetGeometryReady(const ILayerSource::GeometryReadyMessage& message)
      {
        // Once the geometry of the series is downloaded from Orthanc,
        // display its middle slice, and adapt the viewport to fit this
        // slice
        if (source_ == &message.origin_)
        {
          SetSlice(source_->GetSliceCount() / 2);
        }

        mainWidget_->SetDefaultView();
      }
      
      std::unique_ptr<Interactor>           mainWidgetInteractor_;
      std::unique_ptr<OrthancApiClient>     orthancApiClient_;

      const OrthancFrameLayerSource*        source_;
      unsigned int                          slice_;

    public:
      SingleFrameApplication(MessageBroker& broker) :
        IObserver(broker),
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
                              IStatusBar& statusBar,
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

        orthancApiClient_.reset(new OrthancApiClient(IObserver::broker_, context_->GetWebService()));
        mainWidget_ = new LayerWidget(broker_, "main-widget");

        std::auto_ptr<OrthancFrameLayerSource> layer(new OrthancFrameLayerSource(broker_, *orthancApiClient_));
        source_ = layer.get();
        layer->LoadFrame(instance, frame);
        layer->RegisterObserverCallback(new Callable<SingleFrameApplication, ILayerSource::GeometryReadyMessage>(*this, &SingleFrameApplication::OnMainWidgetGeometryReady));
        mainWidget_->AddLayer(layer.release());

        RenderStyle s;

        if (parameters["smooth"].as<bool>())
        {
          s.interpolation_ = ImageInterpolation_Bilinear;
        }

        mainWidget_->SetLayerStyle(0, s);
        mainWidget_->SetTransmitMouseOver(true);

        mainWidgetInteractor_.reset(new Interactor(*this));
        mainWidget_->SetInteractor(*mainWidgetInteractor_);
      }
    };


  }
}
