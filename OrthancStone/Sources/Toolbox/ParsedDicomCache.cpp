/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "ParsedDicomCache.h"

#include <Logging.h>

#include <cassert>


namespace OrthancStone
{
  ParsedDicomCache::Item::Item(const boost::shared_ptr<Orthanc::ParsedDicomFile>& dicom,
                               size_t fileSize,
                               bool hasPixelData) :
    dicom_(dicom),
    fileSize_(fileSize),
    hasPixelData_(hasPixelData)
  {
    if (dicom == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }


  void ParsedDicomCache::Store(const std::string& key,
                               const boost::shared_ptr<Orthanc::ParsedDicomFile>& dicom,
                               size_t fileSize,
                               bool hasPixelData)
  {
    if (dicom == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      cache_.Store(key, boost::shared_ptr<Item>(new Item(dicom, fileSize, hasPixelData)), fileSize);
    }
  }


  ParsedDicomCache::Accessor::Accessor(ParsedDicomCache& cache,
                                       const std::string& key) :
    item_(cache.cache_.GetCachedValue(key))
  {
    if (item_)
    {
      lock_.reset(new Orthanc::Mutex::ScopedLock(dynamic_cast<Item&>(*item_).GetMutex()));
    }
  }


  bool ParsedDicomCache::Accessor::IsValid() const
  {
    return (item_ ? true : false);
  }


  const ParsedDicomCache::Item& ParsedDicomCache::Accessor::GetItem() const
  {
    if (item_)
    {
      return dynamic_cast<Item&>(*item_);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
}
