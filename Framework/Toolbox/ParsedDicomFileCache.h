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

#include <Core/Cache/MemoryObjectCache.h>
#include <Core/DicomParsing/ParsedDicomFile.h>

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class ParsedDicomFileCache : public boost::noncopyable
  {
  private:
    class Item;
    
    Orthanc::MemoryObjectCache  cache_;

  public:
    ParsedDicomFileCache(size_t size)
    {
      cache_.SetMaximumSize(size);
    }
    
    void Acquire(const std::string& path,
                 boost::shared_ptr<Orthanc::ParsedDicomFile> dicom,
                 size_t fileSize,
                 bool hasPixelData);

    class Reader : public boost::noncopyable
    {
    private:
      Orthanc::MemoryObjectCache::Reader reader_;
      std::auto_ptr<boost::mutex::scoped_lock>  lock_;
      Item*                         item_;

    public:
      Reader(ParsedDicomFileCache& cache,
             const std::string& path);

      bool IsValid() const
      {
        return item_ != NULL;
      }

      bool HasPixelData() const;

      boost::shared_ptr<Orthanc::ParsedDicomFile> GetDicom() const;

      size_t GetFileSize() const;
    };
  };
}
