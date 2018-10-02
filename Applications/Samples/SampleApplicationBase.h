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

#include "../../Applications/IStoneApplication.h"

namespace OrthancStone
{
  namespace Samples
  {
    class SampleApplicationBase : public IStoneApplication
    {
    protected:
      BaseCommandBuilder commandBuilder_;
    public:
      virtual void Initialize(StoneApplicationContext* context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
      }

      virtual std::string GetTitle() const
      {
        return "Stone of Orthanc - Sample";
      }

      virtual void OnPushButton1Clicked() {}
      virtual void OnPushButton2Clicked() {}
      virtual void OnTool1Clicked() {}
      virtual void OnTool2Clicked() {}

      virtual void GetButtonNames(std::string& pushButton1,
                                  std::string& pushButton2,
                                  std::string& tool1,
                                  std::string& tool2
                                  ) {
        pushButton1 = "action1";
        pushButton2 = "action2";
        tool1 = "tool1";
        tool2 = "tool2";
      }

      virtual BaseCommandBuilder& GetCommandBuilder() {return commandBuilder_;}

    };
  }
}
