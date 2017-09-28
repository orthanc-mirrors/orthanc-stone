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
#include "../../Framework/Layers/LineMeasureTracker.h"
#include "../../Framework/Layers/CircleMeasureTracker.h"

#include <Core/Toolbox.h>
#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class SingleVolumeApplication :
      public SampleApplicationBase
    {
    public:
      virtual void DeclareCommandLineOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("series", boost::program_options::value<std::string>(), 
           "Orthanc ID of the series")
          ("instance", boost::program_options::value<std::string>(), 
           "Orthanc ID of a multi-frame instance that describes a 3D volume")
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

        if (parameters.count("series") > 1 ||
            parameters.count("instance") > 1)
        {
          LOG(ERROR) << "Only one series or instance is allowed";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        if (parameters.count("series") == 1 &&
            parameters.count("instance") == 1)
        {
          LOG(ERROR) << "Cannot specify both a series and an instance";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string series;
        if (parameters.count("series") == 1)
        {
          series = parameters["series"].as<std::string>();
        }
        
        std::string instance;
        if (parameters.count("instance") == 1)
        {
          instance = parameters["instance"].as<std::string>();
        }
        
        if (series.empty() &&
            instance.empty())
        {
          LOG(ERROR) << "The series ID or instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

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

        std::auto_ptr<LayerWidget> widget(new LayerWidget);

#if 1
        std::auto_ptr<OrthancVolumeImage> volume(new OrthancVolumeImage(context.GetWebService()));
        if (series.empty())
        {
          volume->ScheduleLoadInstance(instance);
        }
        else
        {
          volume->ScheduleLoadSeries(series);
        }

        widget->AddLayer(new VolumeImageSource(*volume));

        context.AddInteractor(new VolumeImageInteractor(*volume, *widget, projection));
        context.AddVolume(volume.release());
#else
        std::auto_ptr<OrthancVolumeImage> ct(new OrthancVolumeImage(context.GetWebService()));
        ct->ScheduleLoadSeries("dd069910-4f090474-7d2bba07-e5c10783-f9e4fb1d");

        std::auto_ptr<OrthancVolumeImage> pet(new OrthancVolumeImage(context.GetWebService()));
        pet->ScheduleLoadSeries("aabad2e7-80702b5d-e599d26c-4f13398e-38d58a9e");

        widget->AddLayer(new VolumeImageSource(*ct));
        widget->AddLayer(new VolumeImageSource(*pet));
        
        context.AddInteractor(new VolumeImageInteractor(*pet, *widget, projection));
        context.AddVolume(ct.release());
        context.AddVolume(pet.release());

        {
          RenderStyle s;
          //s.drawGrid_ = true;
          s.alpha_ = 1;
          s.windowing_ = ImageWindowing_Bone;
          widget->SetLayerStyle(0, s);
        }

        {
          RenderStyle s;
          //s.drawGrid_ = true;
          s.SetColor(255, 0, 0);  // Draw missing PET layer in red
          s.alpha_ = 0.3;
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
        context.SetCentralWidget(widget.release());
      }
    };
  }
}
