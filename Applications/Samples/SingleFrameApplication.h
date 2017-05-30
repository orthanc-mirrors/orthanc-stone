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
          unsigned int scale = (modifiers & KeyboardModifiers_Control ? 10 : 1);
          
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
          widget_->SetSlice(source_->GetSlice(slice_).GetGeometry());
#else
          // TEST for scene extents - Rotate the axes
          double a = 15.0 / 180.0 * M_PI;
          
          Vector x; GeometryToolbox::AssignVector(x, cos(a), sin(a), 0);
          Vector y; GeometryToolbox::AssignVector(y, -sin(a), cos(a), 0);
          SliceGeometry s(source_->GetSlice(slice_).GetGeometry().GetOrigin(), x, y);
          widget_->SetSlice(s);
#endif
        }
      }
        
      
      virtual void NotifyGeometryReady(const ILayerSource& source)
      {
        // Once the geometry of the series is downloaded from Orthanc,
        // display its first slice, and adapt the viewport to fit this
        // slice
        if (source_ == &source)
        {
          SetSlice(source_->GetSliceCount() / 2);
        }

        widget_->SetDefaultView();
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

      LayerWidget*                    widget_;
      const OrthancFrameLayerSource*  source_;
      unsigned int                    slice_;
      
    public:
      SingleFrameApplication() : 
        widget_(NULL),
        source_(NULL),
        slice_(0)
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
          (new OrthancFrameLayerSource(context.GetWebService()));
        //layer->SetImageQuality(SliceImageQuality_Jpeg50);
        layer->LoadInstance(instance, frame);
        //layer->LoadSeries("6f1b492a-e181e200-44e51840-ef8db55e-af529ab6");
        layer->Register(*this);
        source_ = layer.get();
        widget->AddLayer(layer.release());

        RenderStyle s;

        if (parameters["smooth"].as<bool>())
        {
          s.interpolation_ = ImageInterpolation_Linear;
        }

        //s.drawGrid_ = true;
        widget->SetLayerStyle(0, s);
#else
        // 0178023P**
        // Extent of the CT layer: (-35.068 -20.368) => (34.932 49.632)
        std::auto_ptr<OrthancFrameLayerSource> ct;
        ct.reset(new OrthancFrameLayerSource(context.GetWebService()));
        //ct->LoadInstance("c804a1a2-142545c9-33b32fe2-3df4cec0-a2bea6d6", 0);
        //ct->LoadInstance("4bd4304f-47478948-71b24af2-51f4f1bc-275b6c1b", 0);  // BAD SLICE
        ct->SetImageQuality(SliceImageQuality_Jpeg50);
        ct->LoadSeries("dd069910-4f090474-7d2bba07-e5c10783-f9e4fb1d");

        ct->Register(*this);
        widget->AddLayer(ct.release());

        std::auto_ptr<OrthancFrameLayerSource> pet;
        pet.reset(new OrthancFrameLayerSource(context.GetWebService()));
        //pet->LoadInstance("a1c4dc6b-255d27f0-88069875-8daed730-2f5ee5c6", 0);
        pet->LoadSeries("aabad2e7-80702b5d-e599d26c-4f13398e-38d58a9e");
        pet->Register(*this);
        source_ = pet.get();
        widget->AddLayer(pet.release());

        {
          RenderStyle s;
          //s.drawGrid_ = true;
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
          s.interpolation_ = ImageInterpolation_Linear;
          widget->SetLayerStyle(1, s);
        }
#endif

        widget_ = widget.get();
        widget_->SetTransmitMouseOver(true);
        widget_->SetInteractor(context.AddInteractor(new Interactor(*this)));
        context.SetCentralWidget(widget.release());
      }
    };
  }
}
