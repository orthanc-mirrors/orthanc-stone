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

#include "SampleInteractor.h"

#include "../../Resources/Orthanc/Core/Logging.h"

namespace OrthancStone
{
  namespace Samples
  {
    class BasicPetCtFusionApplication : public SampleApplicationBase
    {
    private:
      class Interactor : public SampleInteractor
      {
      public:
        static void SetStyle(LayeredSceneWidget& widget,
                             bool ct,
                             bool pet)
        {
          if (ct)
          {
            RenderStyle style;
            style.windowing_ = ImageWindowing_Bone;
            widget.SetLayerStyle(0, style);
          }
          else
          {
            RenderStyle style;
            style.visible_ = false;
            widget.SetLayerStyle(0, style);
          }

          if (ct && pet)
          {
            RenderStyle style;
            style.applyLut_ = true;
            style.alpha_ = 0.5;
            widget.SetLayerStyle(1, style);
          }
          else if (pet)
          {
            RenderStyle style;
            style.applyLut_ = true;
            widget.SetLayerStyle(1, style);
          }
          else
          {
            RenderStyle style;
            style.visible_ = false;
            widget.SetLayerStyle(1, style);
          }
        }


        static bool IsVisible(LayeredSceneWidget& widget,
                              size_t layer)
        {
          RenderStyle style = widget.GetLayerStyle(layer);
          return style.visible_;
        }


        static void ToggleInterpolation(LayeredSceneWidget& widget,
                                        size_t layer)
        {
          RenderStyle style = widget.GetLayerStyle(layer);
         
          if (style.interpolation_ == ImageInterpolation_Bilinear)
          {
            style.interpolation_ = ImageInterpolation_Nearest;
          }
          else
          {
            style.interpolation_ = ImageInterpolation_Bilinear;
          }

          widget.SetLayerStyle(layer, style);
        }


        Interactor(VolumeImage& volume,
                   VolumeProjection projection, 
                   bool reverse) :
          SampleInteractor(volume, projection, reverse)
        {
        }


        virtual void KeyPressed(WorldSceneWidget& widget,
                                char key,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
        {
          LayeredSceneWidget& layered = dynamic_cast<LayeredSceneWidget&>(widget);

          switch (key)
          {
            case 'c':
              // Toggle the visibility of the CT layer
              SetStyle(layered, !IsVisible(layered, 0), IsVisible(layered, 1));
              break;

            case 'p':
              // Toggle the visibility of the PET layer
              SetStyle(layered, IsVisible(layered, 0), !IsVisible(layered, 1));
              break;

            case 'i':
            {
              // Toggle on/off the interpolation
              ToggleInterpolation(layered, 0);
              ToggleInterpolation(layered, 1);
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
          ("ct", boost::program_options::value<std::string>(), 
           "Orthanc ID of the CT series")
          ("pet", boost::program_options::value<std::string>(), 
           "Orthanc ID of the PET series")
          ("threads", boost::program_options::value<unsigned int>()->default_value(3), 
           "Number of download threads for the CT series")
          ;

        options.add(generic);    
      }

      virtual void Initialize(BasicApplicationContext& context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        if (parameters.count("ct") != 1 ||
            parameters.count("pet") != 1)
        {
          LOG(ERROR) << "The series ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string ct = parameters["ct"].as<std::string>();
        std::string pet = parameters["pet"].as<std::string>();
        unsigned int threads = parameters["threads"].as<unsigned int>();

        VolumeImage& ctVolume = context.AddSeriesVolume(ct, true /* progressive download */, threads);
        VolumeImage& petVolume = context.AddSeriesVolume(pet, true /* progressive download */, 1);

        // Take the PET volume as the reference for the slices
        std::auto_ptr<Interactor> interactor(new Interactor(petVolume, VolumeProjection_Axial, false /* don't reverse normal */));

        std::auto_ptr<LayeredSceneWidget> widget(new LayeredSceneWidget);
        widget->AddLayer(new VolumeImage::LayerFactory(ctVolume));
        widget->AddLayer(new VolumeImage::LayerFactory(petVolume));
        widget->SetSlice(interactor->GetCursor().GetCurrentSlice());
        widget->SetInteractor(*interactor);

        Interactor::SetStyle(*widget, true, true);   // Initially, show both CT and PET layers

        context.AddInteractor(interactor.release());
        context.SetCentralWidget(widget.release());

        statusBar.SetMessage("Use the key \"t\" to toggle the fullscreen mode");
        statusBar.SetMessage("Use the key \"c\" to show/hide the CT layer");
        statusBar.SetMessage("Use the key \"p\" to show/hide the PET layer");
        statusBar.SetMessage("Use the key \"i\" to toggle the smoothing of the images");
      }
    };
  }
}
