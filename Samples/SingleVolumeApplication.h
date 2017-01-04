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


#pragma once

#include "SampleInteractor.h"

#include "../Resources/Orthanc/Core/Toolbox.h"
#include "../Framework/Layers/LineMeasureTracker.h"
#include "../Framework/Layers/CircleMeasureTracker.h"
#include "../Resources/Orthanc/Core/Logging.h"

namespace OrthancStone
{
  namespace Samples
  {
    class SingleVolumeApplication : public SampleApplicationBase
    {
    private:
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


    public:
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
        
        VolumeProjection projection;
        if (tmp == "axial")
        {
          projection = VolumeProjection_Axial;
        }
        else if (tmp == "sagittal")
        {
          projection = VolumeProjection_Sagittal;
        }
        else if (tmp == "coronal")
        {
          projection = VolumeProjection_Coronal;
        }
        else
        {
          LOG(ERROR) << "Unknown projection: " << tmp;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        VolumeImage& volume = context.AddSeriesVolume(series, true /* progressive download */, threads);

        std::auto_ptr<Interactor> interactor(new Interactor(volume, projection, reverse));

        std::auto_ptr<LayeredSceneWidget> widget(new LayeredSceneWidget);
        widget->AddLayer(new VolumeImage::LayerFactory(volume));
        widget->SetSlice(interactor->GetCursor().GetCurrentSlice());
        widget->SetInteractor(*interactor);

        context.AddInteractor(interactor.release());
        context.SetCentralWidget(widget.release());

        statusBar.SetMessage("Use the keys \"b\", \"l\" and \"d\" to change Hounsfield windowing");
        statusBar.SetMessage("Use the keys \"t\" to track the (X,Y,Z) mouse coordinates");
        statusBar.SetMessage("Use the keys \"m\" to measure distances");
        statusBar.SetMessage("Use the keys \"c\" to draw circles");
      }
    };
  }
}
