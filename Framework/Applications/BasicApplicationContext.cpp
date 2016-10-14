/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "BasicApplicationContext.h"

#include "../Toolbox/OrthancSeriesLoader.h"
#include "../Volumes/VolumeImageSimplePolicy.h"
#include "../Volumes/VolumeImageProgressivePolicy.h"

namespace OrthancStone
{
  BasicApplicationContext::BasicApplicationContext(IOrthancConnection& orthanc) :
    orthanc_(orthanc)
  {
  }


  BasicApplicationContext::~BasicApplicationContext()
  {
    for (Interactors::iterator it = interactors_.begin(); it != interactors_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }

    for (Volumes::iterator it = volumes_.begin(); it != volumes_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }

    for (StructureSets::iterator it = structureSets_.begin(); it != structureSets_.end(); ++it)
    {
      assert(*it != NULL);
      delete *it;
    }
  }


  IWidget& BasicApplicationContext::SetCentralWidget(IWidget* widget)   // Takes ownership
  {
    viewport_.SetCentralWidget(widget);
    return *widget;
  }


  VolumeImage& BasicApplicationContext::AddSeriesVolume(const std::string& series,
                                                        bool isProgressiveDownload,
                                                        size_t downloadThreadCount)
  {
    std::auto_ptr<VolumeImage> volume(new VolumeImage(new OrthancSeriesLoader(orthanc_, series)));

    if (isProgressiveDownload)
    {
      volume->SetDownloadPolicy(new VolumeImageProgressivePolicy);
    }
    else
    {
      volume->SetDownloadPolicy(new VolumeImageSimplePolicy);
    }

    volume->SetThreadCount(downloadThreadCount);

    VolumeImage& result = *volume;
    volumes_.push_back(volume.release());

    return result;
  }


  DicomStructureSet& BasicApplicationContext::AddStructureSet(const std::string& instance)
  {
    std::auto_ptr<DicomStructureSet> structureSet(new DicomStructureSet(orthanc_, instance));

    DicomStructureSet& result = *structureSet;
    structureSets_.push_back(structureSet.release());

    return result;
  }


  IWorldSceneInteractor& BasicApplicationContext::AddInteractor(IWorldSceneInteractor* interactor)
  {
    if (interactor == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    interactors_.push_back(interactor);

    return *interactor;
  }


  void BasicApplicationContext::Start()
  {
    for (Volumes::iterator it = volumes_.begin(); it != volumes_.end(); ++it)
    {
      assert(*it != NULL);
      (*it)->Start();
    }

    viewport_.Start();
  }


  void BasicApplicationContext::Stop()
  {
    viewport_.Stop();

    for (Volumes::iterator it = volumes_.begin(); it != volumes_.end(); ++it)
    {
      assert(*it != NULL);
      (*it)->Stop();
    }
  }
}
