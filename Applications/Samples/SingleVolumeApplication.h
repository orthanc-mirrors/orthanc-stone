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
#include "../../Framework/Layers/ILayerSource.h"
#include "../../Framework/Layers/LineMeasureTracker.h"
#include "../../Framework/Layers/CircleMeasureTracker.h"

#include <Core/Toolbox.h>
#include <Core/Logging.h>

#include <Plugins/Samples/Common/OrthancHttpConnection.h>   // TODO REMOVE
#include "../../Framework/Layers/DicomStructureSetRendererFactory.h"   // TODO REMOVE
#include "../../Framework/Toolbox/MessagingToolbox.h"   // TODO REMOVE

namespace OrthancStone
{
  namespace Samples
  {
    class SingleVolumeApplication : public SampleApplicationBase
    {
    private:
      class Interactor : public VolumeImageInteractor
      {
      private:
        LayerWidget&  widget_;
        size_t        layer_;
        
      protected:
        virtual void NotifySliceChange(const ISlicedVolume& volume,
                                       const size_t& sliceIndex,
                                       const Slice& slice)
        {
          const OrthancVolumeImage& image = dynamic_cast<const OrthancVolumeImage&>(volume);

          RenderStyle s = widget_.GetLayerStyle(layer_);

          if (image.FitWindowingToRange(s, slice.GetConverter()))
          {
            //printf("Windowing: %f => %f\n", s.customWindowCenter_, s.customWindowWidth_);
            widget_.SetLayerStyle(layer_, s);
          }
        }

        virtual void MouseOver(CairoContext& context,
                               WorldSceneWidget& widget,
                               const ViewportGeometry& view,
                               double x,
                               double y,
                               IStatusBar* statusBar)
        {
          const LayerWidget& w = dynamic_cast<const LayerWidget&>(widget);
          Vector p = w.GetSlice().MapSliceToWorldCoordinates(x, y);
          printf("%f %f %f\n", p[0], p[1], p[2]);
        }
      
      public:
        Interactor(OrthancVolumeImage& volume,
                   LayerWidget& widget,
                   VolumeProjection projection,
                   size_t layer) :
          VolumeImageInteractor(volume, widget, projection),
          widget_(widget),
          layer_(layer)
        {
        }
      };


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

#if 0
        std::auto_ptr<OrthancVolumeImage> volume(new OrthancVolumeImage(context.GetWebService(), true));
        if (series.empty())
        {
          volume->ScheduleLoadInstance(instance);
        }
        else
        {
          volume->ScheduleLoadSeries(series);
        }

        widget->AddLayer(new VolumeImageSource(*volume));

        context.AddInteractor(new Interactor(*volume, *widget, projection, 0));
        context.AddVolume(volume.release());

        {
          RenderStyle s;
          s.alpha_ = 1;
          s.applyLut_ = true;
          s.lut_ = Orthanc::EmbeddedResources::COLORMAP_JET;
          s.interpolation_ = ImageInterpolation_Linear;
          widget->SetLayerStyle(0, s);
        }
#else
        std::auto_ptr<OrthancVolumeImage> ct(new OrthancVolumeImage(context.GetWebService(), false));
        //ct->ScheduleLoadSeries("dd069910-4f090474-7d2bba07-e5c10783-f9e4fb1d");
        ct->ScheduleLoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");  // IBA
        //ct->ScheduleLoadSeries("03677739-1d8bca40-db1daf59-d74ff548-7f6fc9c0");  // 0522c0001 TCIA
        
        std::auto_ptr<OrthancVolumeImage> pet(new OrthancVolumeImage(context.GetWebService(), true));
        //pet->ScheduleLoadSeries("aabad2e7-80702b5d-e599d26c-4f13398e-38d58a9e");
        pet->ScheduleLoadInstance("830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb"); // IBA 1
        //pet->ScheduleLoadInstance("337876a1-a68a9718-f15abccd-38faafa1-b99b496a"); // IBA 2
        //pet->ScheduleLoadInstance("830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb");  // IBA 3
        //pet->ScheduleLoadInstance("269f26f4-0c83eeeb-2e67abbd-5467a40f-f1bec90c");  // 0522c0001 TCIA

        const std::string s = "54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9";  // IBA
        //const std::string s = "17cd032b-ad92a438-ca05f06a-f9e96668-7e3e9e20";  // 0522c0001 TCIA
        
        widget->AddLayer(new VolumeImageSource(*ct));
        widget->AddLayer(new VolumeImageSource(*pet));
        widget->AddLayer(new DicomStructureSetRendererFactory(context.GetWebService(), s));
        
        context.AddInteractor(new Interactor(*pet, *widget, projection, 1));
        //context.AddInteractor(new VolumeImageInteractor(*ct, *widget, projection));

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
          s.alpha_ = 0.5;
          s.applyLut_ = true;
          s.lut_ = Orthanc::EmbeddedResources::COLORMAP_JET;
          s.interpolation_ = ImageInterpolation_Linear;
          s.windowing_ = ImageWindowing_Custom;
          s.customWindowCenter_ = 0;
          s.customWindowWidth_ = 128;
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
