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
#include "../../Framework/dev.h"
//#include "SampleInteractor.h"
#include "../../Framework/Widgets/LayerWidget.h"

#include "../../Resources/Orthanc/Core/Toolbox.h"
#include "../../Framework/Layers/LineMeasureTracker.h"
#include "../../Framework/Layers/CircleMeasureTracker.h"
#include "../../Resources/Orthanc/Core/Logging.h"

namespace OrthancStone
{
  namespace Samples
  {
    class SingleVolumeApplication :
      public SampleApplicationBase,
      private ILayerSource::IObserver
    {
    private:
      class Interactor : public IWorldSceneInteractor
      {
      private:
        SingleVolumeApplication&  application_;
        
      public:
        Interactor(SingleVolumeApplication&  application) :
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


      LayerWidget*                        widget_;
      OrthancVolumeImage*                 volume_;
      VolumeProjection                    projection_;
      std::auto_ptr<VolumeImageGeometry>  slices_;
      size_t                              slice_;

      void OffsetSlice(int offset)
      {
        if (slices_.get() != NULL)
        {
          int slice = static_cast<int>(slice_) + offset;

          if (slice < 0)
          {
            slice = 0;
          }

          if (slice >= static_cast<int>(slices_->GetSliceCount()))
          {
            slice = slices_->GetSliceCount() - 1;
          }

          if (slice != static_cast<int>(slice_)) 
          {
            SetSlice(slice);
          }   
        }
      }
      
      void SetSlice(size_t slice)
      {
        if (slices_.get() != NULL)
        {
          slice_ = slice;
          widget_->SetSlice(slices_->GetSlice(slice_).GetGeometry());
        }
      }
      
      virtual void NotifyGeometryReady(const ILayerSource& source)
      {
        if (slices_.get() == NULL)
        {
          slices_.reset(new VolumeImageGeometry(*volume_, projection_));
          SetSlice(slices_->GetSliceCount() / 2);
          
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

#if 0
      class Interactor : public SampleInteractor
      {
      private:
        enum MouseMode
        {
          MouseMode_None,
          MouseMode_TrackCoordinates,
          MouseMode_LineMeasure,
          MouseMode_CircleMeasure
        };

        MouseMode mouseMode_;

        void SetMouseMode(MouseMode mode,
                          IStatusBar* statusBar)
        {
          if (mouseMode_ == mode)
          {
            mouseMode_ = MouseMode_None;
          }
          else
          {
            mouseMode_ = mode;
          }

          if (statusBar)
          {
            switch (mouseMode_)
            {
              case MouseMode_None:
                statusBar->SetMessage("Disabling the mouse tools");
                break;

              case MouseMode_TrackCoordinates:
                statusBar->SetMessage("Tracking the mouse coordinates");
                break;

              case MouseMode_LineMeasure:
                statusBar->SetMessage("Mouse clicks will now measure the distances");
                break;

              case MouseMode_CircleMeasure:
                statusBar->SetMessage("Mouse clicks will now draw circles");
                break;

              default:
                break;
            }
          }
        }

      public:
        Interactor(VolumeImage& volume,
                   VolumeProjection projection, 
                   bool reverse) :
          SampleInteractor(volume, projection, reverse),
          mouseMode_(MouseMode_None)
        {
        }
        
        virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                            const SliceGeometry& slice,
                                                            const ViewportGeometry& view,
                                                            MouseButton button,
                                                            double x,
                                                            double y,
                                                            IStatusBar* statusBar)
        {
          if (button == MouseButton_Left)
          {
            switch (mouseMode_)
            {
              case MouseMode_LineMeasure:
                return new LineMeasureTracker(NULL, slice, x, y, 255, 0, 0, 14 /* font size */);
              
              case MouseMode_CircleMeasure:
                return new CircleMeasureTracker(NULL, slice, x, y, 255, 0, 0, 14 /* font size */);

              default:
                break;
            }
          }

          return NULL;
        }

        virtual void MouseOver(CairoContext& context,
                               WorldSceneWidget& widget,
                               const SliceGeometry& slice,
                               const ViewportGeometry& view,
                               double x,
                               double y,
                               IStatusBar* statusBar)
        {
          if (mouseMode_ == MouseMode_TrackCoordinates &&
              statusBar != NULL)
          {
            Vector p = slice.MapSliceToWorldCoordinates(x, y);
            
            char buf[64];
            sprintf(buf, "X = %.02f Y = %.02f Z = %.02f (in cm)", p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
            statusBar->SetMessage(buf);
          }
        }


        virtual void KeyPressed(WorldSceneWidget& widget,
                                char key,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
        {
          switch (key)
          {
            case 't':
              SetMouseMode(MouseMode_TrackCoordinates, statusBar);
              break;

            case 'm':
              SetMouseMode(MouseMode_LineMeasure, statusBar);
              break;

            case 'c':
              SetMouseMode(MouseMode_CircleMeasure, statusBar);
              break;

            case 'b':
            {
              if (statusBar)
              {
                statusBar->SetMessage("Setting Hounsfield window to bones");
              }

              RenderStyle style;
              style.windowing_ = ImageWindowing_Bone;
              dynamic_cast<LayeredSceneWidget&>(widget).SetLayerStyle(0, style);
              break;
            }

            case 'l':
            {
              if (statusBar)
              {
                statusBar->SetMessage("Setting Hounsfield window to lung");
              }

              RenderStyle style;
              style.windowing_ = ImageWindowing_Lung;
              dynamic_cast<LayeredSceneWidget&>(widget).SetLayerStyle(0, style);
              break;
            }

            case 'd':
            {
              if (statusBar)
              {
                statusBar->SetMessage("Setting Hounsfield window to what is written in the DICOM file");
              }

              RenderStyle style;
              style.windowing_ = ImageWindowing_Default;
              dynamic_cast<LayeredSceneWidget&>(widget).SetLayerStyle(0, style);
              break;
            }

            default:
              break;
          }
        }
      };
#endif

      
    public:
      SingleVolumeApplication() : 
        widget_(NULL),
        volume_(NULL)
      {
      }
      
      virtual void DeclareCommandLineOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("series", boost::program_options::value<std::string>(), 
           "Orthanc ID of the series")
          ("threads", boost::program_options::value<unsigned int>()->default_value(3), 
           "Number of download threads")
          ("projection", boost::program_options::value<std::string>()->default_value("axial"), 
           "Projection of interest (can be axial, sagittal or coronal)")
          ("reverse", boost::program_options::value<bool>()->default_value(false), 
           "Reverse the normal direction of the volume")
          ;

        options.add(generic);    
      }

      virtual void Initialize(BasicApplicationContext& context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        if (parameters.count("series") != 1)
        {
          LOG(ERROR) << "The series ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string series = parameters["series"].as<std::string>();
        unsigned int threads = parameters["threads"].as<unsigned int>();
        bool reverse = parameters["reverse"].as<bool>();

        std::string tmp = parameters["projection"].as<std::string>();
        Orthanc::Toolbox::ToLowerCase(tmp);
        
        if (tmp == "axial")
        {
          projection_ = VolumeProjection_Axial;
        }
        else if (tmp == "sagittal")
        {
          projection_ = VolumeProjection_Sagittal;
        }
        else if (tmp == "coronal")
        {
          projection_ = VolumeProjection_Coronal;
        }
        else
        {
          LOG(ERROR) << "Unknown projection: " << tmp;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::auto_ptr<LayerWidget> widget(new LayerWidget);
        widget_ = widget.get();

#if 0
        std::auto_ptr<OrthancVolumeImage> volume(new OrthancVolumeImage(context.GetWebService()));
        volume->ScheduleLoadSeries(series);

        volume_ = volume.get();
        
        {
          std::auto_ptr<VolumeImageSource> source(new VolumeImageSource(*volume));
          source->Register(*this);
          widget->AddLayer(source.release());
        }

        context.AddVolume(volume.release());
#else
        std::auto_ptr<OrthancVolumeImage> ct(new OrthancVolumeImage(context.GetWebService()));
        ct->ScheduleLoadSeries("dd069910-4f090474-7d2bba07-e5c10783-f9e4fb1d");

        std::auto_ptr<OrthancVolumeImage> pet(new OrthancVolumeImage(context.GetWebService()));
        pet->ScheduleLoadSeries("aabad2e7-80702b5d-e599d26c-4f13398e-38d58a9e");

        volume_ = pet.get();
        
        {
          std::auto_ptr<VolumeImageSource> source(new VolumeImageSource(*ct));
          //source->Register(*this);
          widget->AddLayer(source.release());
        }

        {
          std::auto_ptr<VolumeImageSource> source(new VolumeImageSource(*pet));
          source->Register(*this);
          widget->AddLayer(source.release());
        }

        context.AddVolume(ct.release());
        context.AddVolume(pet.release());

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


        statusBar.SetMessage("Use the keys \"b\", \"l\" and \"d\" to change Hounsfield windowing");
        statusBar.SetMessage("Use the keys \"t\" to track the (X,Y,Z) mouse coordinates");
        statusBar.SetMessage("Use the keys \"m\" to measure distances");
        statusBar.SetMessage("Use the keys \"c\" to draw circles");

        widget->SetTransmitMouseOver(true);
        widget->SetInteractor(context.AddInteractor(new Interactor(*this)));
        context.SetCentralWidget(widget.release());
      }
    };
  }
}
