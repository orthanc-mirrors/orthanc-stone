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

#include "SampleApplicationBase.h"
#include "SampleInteractor.h"

#include "../../Framework/Layers/OrthancFrameLayerSource.h"
#include "../../Framework/Widgets/LayerWidget.h"
#include "../../Resources/Orthanc/Core/Logging.h"

namespace OrthancStone
{
  namespace Samples
  {
    class SingleFrameApplication :
      public SampleApplicationBase,
      private ILayerSource::IObserver
    {
    private:
      class Interactor : public IWorldSceneInteractor
      {
      public:
        virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                            const ViewportGeometry& view,
                                                            MouseButton button,
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
        }

        virtual void KeyPressed(WorldSceneWidget& widget,
                                char key,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
        {
          switch (key)
          {
            case 's':
              widget.SetDefaultView();
              break;

            default:
              break;
          }
        }
      };

      virtual void NotifyGeometryReady(const ILayerSource& source)
      {
        // Once the geometry of the series is downloaded from Orthanc,
        // display its first slice, and adapt the viewport to fit this
        // slice
        
        const OrthancFrameLayerSource& frame =
          dynamic_cast<const OrthancFrameLayerSource&>(source);

        if (frame.GetSliceCount() > 0)
        {
          widget_->SetSlice(frame.GetSlice(0).GetGeometry());
          widget_->SetDefaultView();
        }
      }
      
      virtual void NotifyGeometryError(const ILayerSource& source)
      {
      }
      
      virtual void NotifyContentChange(const ILayerSource& source)
      {
      }

      virtual void NotifySliceChange(const ILayerSource& source,
                                     const Slice& slice)
      {
      }
 
      virtual void NotifyLayerReady(std::auto_ptr<ILayerRenderer>& layer,
                                    const ILayerSource& source,
                                    const Slice& slice,
                                    bool isError)
      {
      }

      LayerWidget*    widget_;
      
    public:
      SingleFrameApplication() : 
        widget_(NULL)
      {
      }
      
      virtual void DeclareCommandLineOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("instance", boost::program_options::value<std::string>(), 
           "Orthanc ID of the instance")
          ("frame", boost::program_options::value<unsigned int>()->default_value(0),
           "Number of the frame, for multi-frame DICOM instances")
          ("smooth", boost::program_options::value<bool>()->default_value(true), 
           "Enable linear interpolation to smooth the image")
          ;

        options.add(generic);    
      }

      virtual void Initialize(BasicApplicationContext& context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        int frame = parameters["frame"].as<unsigned int>();

        std::auto_ptr<LayerWidget> widget(new LayerWidget);

#if 1
        std::auto_ptr<OrthancFrameLayerSource> layer
          (new OrthancFrameLayerSource(context.GetWebService(), instance, frame));
        layer->Register(*this);
        widget->AddLayer(layer.release());

        if (parameters["smooth"].as<bool>())
        {
          RenderStyle s; 
          s.interpolation_ = ImageInterpolation_Linear;
          widget->SetLayerStyle(0, s);
        }
#else
        // 0178023P**
        std::auto_ptr<OrthancFrameLayerSource> ct;
        ct.reset(new OrthancFrameLayerSource(context.GetWebService(), "c804a1a2-142545c9-33b32fe2-3df4cec0-a2bea6d6", 0));
        //ct.reset(new OrthancFrameLayerSource(context.GetWebService(), "4bd4304f-47478948-71b24af2-51f4f1bc-275b6c1b", 0));  // BAD SLICE
        ct->Register(*this);
        widget->AddLayer(ct.release());

        std::auto_ptr<OrthancFrameLayerSource> pet;
        pet.reset(new OrthancFrameLayerSource(context.GetWebService(), "a1c4dc6b-255d27f0-88069875-8daed730-2f5ee5c6", 0));
        widget->AddLayer(pet.release());

        {
          RenderStyle s;
          s.alpha_ = 1;
          widget->SetLayerStyle(0, s);
        }

        {
          RenderStyle s;
          //s.drawGrid_ = true;
          s.SetColor(255, 0, 0);  // Draw missing PET layer in red
          s.alpha_ = 0.5;
          s.applyLut_ = true;
          s.lut_ = Orthanc::EmbeddedResources::COLORMAP_JET;
          widget->SetLayerStyle(1, s);
        }
#endif

        widget_ = widget.get();
        widget_->SetTransmitMouseOver(true);
        widget_->SetInteractor(context.AddInteractor(new Interactor));
        context.SetCentralWidget(widget.release());
      }
    };
  }
}
