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


#include "ParsedDicomFileCache.h"

namespace OrthancStone
{
  class ParsedDicomFileCache::Item : public Orthanc::ICacheable
  {
  private:
    boost::mutex                             mutex_;
    std::auto_ptr<Orthanc::ParsedDicomFile>  dicom_;
    size_t                                   fileSize_;
    bool                                     hasPixelData_;
    
  public:
    Item(Orthanc::ParsedDicomFile* dicom,
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

    boost::mutex& GetMutex()
    {
      return mutex_;
    }
           
    virtual size_t GetMemoryUsage() const
    {
      return fileSize_;
    }

    Orthanc::ParsedDicomFile& GetDicom() const
    {
      assert(dicom_.get() != NULL);
      return *dicom_;
    }

    bool HasPixelData() const
    {
      return hasPixelData_;
    }
  };
    

  void ParsedDicomFileCache::Acquire(const std::string& path,
                                     Orthanc::ParsedDicomFile* dicom,
                                     size_t fileSize,
                                     bool hasPixelData)
  {
    cache_.Acquire(path, new Item(dicom, fileSize, hasPixelData));
  }

  
  ParsedDicomFileCache::Reader::Reader(ParsedDicomFileCache& cache,
                                       const std::string& path) :
    /**
     * The "DcmFileFormat" object cannot be accessed from multiple
     * threads, even if using only getters. An unique lock (mutex) is
     * mandatory.
     **/
    accessor_(cache.cache_, path, true /* unique */)
  {
    if (accessor_.IsValid())
    {
      item_ = &dynamic_cast<Item&>(accessor_.GetValue());
    }
    else
    {
      item_ = NULL;
    }
  }


  bool ParsedDicomFileCache::Reader::HasPixelData() const
  {
    if (item_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return item_->HasPixelData();
    }
  }

  
  Orthanc::ParsedDicomFile& ParsedDicomFileCache::Reader::GetDicom() const
  {
    if (item_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return item_->GetDicom();
    }
  }

  
  size_t ParsedDicomFileCache::Reader::GetFileSize() const
  {
    if (item_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return item_->GetMemoryUsage();
    }
  }
}
