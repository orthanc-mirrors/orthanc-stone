/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "../../Qt/QtStoneApplicationRunner.h"

#if ORTHANC_ENABLE_QT != 1
#error this file shall be included only with the ORTHANC_ENABLE_QT set to 1
#endif

namespace OrthancStone
{
  namespace Samples
  {
    class SampleQtApplicationRunner : public OrthancStone::QtStoneApplicationRunner
    {
    protected:
      virtual void InitializeMainWindow(OrthancStone::NativeStoneApplicationContext& context)
      {
        window_.reset(application_.CreateQtMainWindow());
      }
    public:
      SampleQtApplicationRunner(MessageBroker& broker,
                                SampleApplicationBase& application)
        : OrthancStone::QtStoneApplicationRunner(broker, application)
      {
      }

    };
  }
}
