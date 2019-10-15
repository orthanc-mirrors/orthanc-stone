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


#include "SampleList.h"
#if ORTHANC_ENABLE_SDL==1
#include "../Sdl/SdlStoneApplicationRunner.h"
#endif
#if ORTHANC_ENABLE_QT==1
#include "Qt/SampleQtApplicationRunner.h"
#endif

int main(int argc, char* argv[]) 
{
  boost::shared_ptr<SampleApplication> sampleStoneApplication(new SampleApplication);

#if ORTHANC_ENABLE_SDL==1
  OrthancStone::SdlStoneApplicationRunner sdlApplicationRunner(sampleStoneApplication);
  return sdlApplicationRunner.Execute(argc, argv);
#endif
  
#if ORTHANC_ENABLE_QT==1
  OrthancStone::Samples::SampleQtApplicationRunner qtAppRunner(sampleStoneApplication);
  return qtAppRunner.Execute(argc, argv);
#endif
}

