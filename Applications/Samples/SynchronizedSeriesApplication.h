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

#include "../../Framework/Toolbox/OrthancSeriesLoader.h"
#include "../../Framework/Layers/SeriesFrameRendererFactory.h"
#include "../../Framework/Layers/ReferenceLineFactory.h"
#include "../../Framework/Widgets/LayoutWidget.h"

#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class SynchronizedSeriesApplication : public SampleApplicationBase
    {
    private:   
      LayeredSceneWidget* CreateSeriesWidget(BasicApplicationContext& context,
                                             const std::string& series)
      {
        std::auto_ptr<ISeriesLoader> loader
          (new OrthancSeriesLoader(context.GetWebService().GetConnection(), series));

        std::auto_ptr<SampleInteractor> interactor(new SampleInteractor(*loader, false));

        std::auto_ptr<LayeredSceneWidget> widget(new LayeredSceneWidget);
        widget->AddLayer(new SeriesFrameRendererFactory(loader.release(), false));
        widget->SetSlice(interactor->GetCursor().GetCurrentSlice());
        widget->SetInteractor(*interactor);

        context.AddInteractor(interactor.release());

        return widget.release();
      }

    public:
      virtual void DeclareCommandLineOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("a", boost::program_options::value<std::string>(), 
           "Orthanc ID of the 1st series")
          ("b", boost::program_options::value<std::string>(), 
           "Orthanc ID of the 2nd series")
          ("c", boost::program_options::value<std::string>(), 
           "Orthanc ID of the 3rd series")
          ;

        options.add(generic);    
      }

      virtual void Initialize(BasicApplicationContext& context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        if (parameters.count("a") != 1 ||
            parameters.count("b") != 1 ||
            parameters.count("c") != 1)
        {
          LOG(ERROR) << "At least one of the three series IDs is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::auto_ptr<LayeredSceneWidget> a(CreateSeriesWidget(context, parameters["a"].as<std::string>()));
        std::auto_ptr<LayeredSceneWidget> b(CreateSeriesWidget(context, parameters["b"].as<std::string>()));
        std::auto_ptr<LayeredSceneWidget> c(CreateSeriesWidget(context, parameters["c"].as<std::string>()));

        ReferenceLineFactory::Configure(*a, *b);
        ReferenceLineFactory::Configure(*a, *c);
        ReferenceLineFactory::Configure(*b, *c);

        std::auto_ptr<LayoutWidget> layout(new LayoutWidget);
        layout->SetPadding(5);
        layout->AddWidget(a.release());

        std::auto_ptr<LayoutWidget> layoutB(new LayoutWidget);
        layoutB->SetVertical();
        layoutB->SetPadding(5);
        layoutB->AddWidget(b.release());
        layoutB->AddWidget(c.release());
        layout->AddWidget(layoutB.release());

        context.SetCentralWidget(layout.release());        
      }
    };
  }
}
