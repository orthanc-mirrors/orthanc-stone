/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include "../../Framework/Widgets/TestCairoWidget.h"
#include "../../Framework/Widgets/TestWorldSceneWidget.h"
#include "../../Framework/Widgets/LayoutWidget.h"

namespace OrthancStone
{
  namespace Samples
  {
    class TestPatternApplication : public SampleApplicationBase
    {
    public:
      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("animate", boost::program_options::value<bool>()->default_value(true), "Animate the test pattern")
          ;

        options.add(generic);    
      }

      virtual void Initialize(IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        std::auto_ptr<LayoutWidget> layout(new LayoutWidget);
        layout->SetPadding(10);
        layout->SetBackgroundCleared(true);
        layout->AddWidget(new TestCairoWidget(parameters["animate"].as<bool>()));
        layout->AddWidget(new TestWorldSceneWidget(parameters["animate"].as<bool>()));

        context_->SetCentralWidget(layout.release());
        context_->SetUpdateDelay(25);  // If animation, update the content each 25ms
      }
    };
  }
}
