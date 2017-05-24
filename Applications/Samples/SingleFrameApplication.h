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

#include "../../Framework/Layers/OrthancFrameLayerSource.h"
#include "../../Framework/Widgets/LayerWidget.h"
#include "../../Resources/Orthanc/Core/Logging.h"

namespace OrthancStone
{
  namespace Samples
  {
    class SingleFrameApplication :
      public SampleApplicationBase,
      public IVolumeSlicesObserver
    {
    private:
      LayerWidget* widget_;
      
    public:
      SingleFrameApplication() : widget_(NULL)
      {
      }
      
      virtual void NotifySlicesAvailable(const ParallelSlices& slices)
      {
        if (widget_ != NULL &&
            slices.GetSliceCount() > 0)
        {
          widget_->SetSlice(slices.GetSlice(0));
        }
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

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        int frame = parameters["frame"].as<unsigned int>();

        std::auto_ptr<LayerWidget> widget(new LayerWidget);

#if 0
        std::auto_ptr<OrthancFrameLayerSource> layer
          (new OrthancFrameLayerSource(context.GetWebService(), instance, frame));
        layer->SetObserver(*this);
        widget->AddLayer(layer.release());

        if (parameters["smooth"].as<bool>())
        {
          RenderStyle s; 
          s.interpolation_ = ImageInterpolation_Linear;
          widget->SetLayerStyle(0, s);
        }
#else
        // 0178023P**
        std::auto_ptr<OrthancFrameLayerSource> layer;
        layer.reset(new OrthancFrameLayerSource(context.GetWebService(), "c804a1a2-142545c9-33b32fe2-3df4cec0-a2bea6d6", 0));
        //layer.reset(new OrthancFrameLayerSource(context.GetWebService(), "4bd4304f-47478948-71b24af2-51f4f1bc-275b6c1b", 0));  // BAD SLICE
        layer->SetObserver(*this);
        widget->AddLayer(layer.release());

        widget->AddLayer(new OrthancFrameLayerSource(context.GetWebService(), "a1c4dc6b-255d27f0-88069875-8daed730-2f5ee5c6", 0));

        RenderStyle s;
        //s.drawGrid_ = true;
        s.alpha_ = 1;
        widget->SetLayerStyle(0, s);
        s.alpha_ = 0.5;
        s.applyLut_ = true;
        s.lut_ = Orthanc::EmbeddedResources::COLORMAP_JET;
        widget->SetLayerStyle(1, s);
#endif
      
        widget_ = widget.get();
        context.SetCentralWidget(widget.release());
      }
    };
  }
}
