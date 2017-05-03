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

#include "../../Framework/Layers/SingleFrameRendererFactory.h"
#include "../../Framework/Widgets/LayeredSceneWidget.h"
#include "../../Resources/Orthanc/Core/Logging.h"

namespace OrthancStone
{
  namespace Samples
  {
    class SingleFrameApplication : public SampleApplicationBase
    {
    public:
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

        std::auto_ptr<SingleFrameRendererFactory>  renderer;
        renderer.reset
          (new SingleFrameRendererFactory(context.GetWebService().GetConnection(), instance, frame));

        std::auto_ptr<LayeredSceneWidget> widget(new LayeredSceneWidget);
        widget->SetSlice(renderer->GetSliceGeometry());
        widget->AddLayer(renderer.release());

        if (parameters["smooth"].as<bool>())
        {
          RenderStyle s; 
          s.interpolation_ = ImageInterpolation_Linear;
          widget->SetLayerStyle(0, s);
        }

        context.SetCentralWidget(widget.release());
      }
    };
  }
}
