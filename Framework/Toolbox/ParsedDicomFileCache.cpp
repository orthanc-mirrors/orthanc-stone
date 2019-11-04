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
    std::auto_ptr<Orthanc::ParsedDicomFile>  dicom_;
    size_t                                   fileSize_;

  public:
    Item(Orthanc::ParsedDicomFile* dicom,
         size_t fileSize) :
      dicom_(dicom),
      fileSize_(fileSize)
    {
      if (dicom == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }           
           
    virtual size_t GetMemoryUsage() const
    {
      return fileSize_;
    }

    const Orthanc::ParsedDicomFile& GetDicom() const
    {
      assert(dicom_.get() != NULL);
      return *dicom_;
    }
  };
    

  void ParsedDicomFileCache::Acquire(const std::string& sopInstanceUid,
                                     Orthanc::ParsedDicomFile* dicom,
                                     size_t fileSize)
  {
    cache_.Acquire(sopInstanceUid, new Item(dicom, fileSize));
  }

  
  const Orthanc::ParsedDicomFile& ParsedDicomFileCache::Reader::GetDicom() const
  {
    return dynamic_cast<const Item&>(reader_.GetValue()).GetDicom();
  }
}
