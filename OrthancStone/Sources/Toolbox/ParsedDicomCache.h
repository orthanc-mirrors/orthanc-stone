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


#pragma once

#include <Cache/SharedObjectCache.h>
#include <Cache/MemoryObjectCache.h>  // TODO Refactoring - Remove this
#include <DicomParsing/ParsedDicomFile.h>

namespace OrthancStone
{
  class ParsedDicomCache : public boost::noncopyable  // TODO Refactoring - Remove this
  {
  private:
    class Item;

    static std::string GetIndex(unsigned int bucket,
                                const std::string& bucketKey);
    
    Orthanc::MemoryObjectCache  cache_;
    size_t                      lowCacheSizeWarning_;

  public:
    explicit ParsedDicomCache(size_t size) :
      lowCacheSizeWarning_(0)
    {
      cache_.SetMaximumSize(size);
    }

    void Invalidate(unsigned int bucket,
                    const std::string& bucketKey)
    {
      cache_.Invalidate(GetIndex(bucket, bucketKey));
    }
    
    void Acquire(unsigned int bucket,
                 const std::string& bucketKey,
                 Orthanc::ParsedDicomFile* dicom,
                 size_t fileSize,
                 bool hasPixelData);

    class Reader : public boost::noncopyable
    {
    private:
      Orthanc::MemoryObjectCache::Accessor accessor_;
      Item*                                item_;

    public:
      Reader(ParsedDicomCache& cache,
             unsigned int bucket,
             const std::string& bucketKey);

      bool IsValid() const
      {
        return item_ != NULL;
      }

      bool HasPixelData() const;

      Orthanc::ParsedDicomFile& GetDicom() const;

      size_t GetFileSize() const;
    };
  };


  namespace New
  {
    class ParsedDicomCache : public boost::noncopyable
    {
    private:
      class Item : public Orthanc::IDynamicObject
      {
      private:
        Orthanc::Mutex                               mutex_;
        boost::shared_ptr<Orthanc::ParsedDicomFile>  dicom_;
        size_t                                       fileSize_;
        bool                                         hasPixelData_;

      public:
        Item(const boost::shared_ptr<Orthanc::ParsedDicomFile>& dicom,
             size_t fileSize,
             bool hasPixelData);

        Orthanc::Mutex& GetMutex()
        {
          return mutex_;
        }

        const boost::shared_ptr<Orthanc::ParsedDicomFile>& GetDicom() const
        {
          return dicom_;
        }

        size_t GetFileSize() const
        {
          return fileSize_;
        }

        bool HasPixelData() const
        {
          return hasPixelData_;
        }
      };

      Orthanc::SharedObjectCache  cache_;

    public:
      ParsedDicomCache(uint64_t capacity) :
        cache_(capacity)
      {
      }

      void Store(const std::string& key,
                 const boost::shared_ptr<Orthanc::ParsedDicomFile>& dicom,
                 size_t fileSize,
                 bool hasPixelData);

      void Invalidate(const std::string& key)
      {
        cache_.Invalidate(key);
      }

      class Accessor : public boost::noncopyable
      {
      private:
        boost::shared_ptr<Orthanc::IDynamicObject>   item_;
        std::unique_ptr<Orthanc::Mutex::ScopedLock>  lock_;

        const Item& GetItem() const;

      public:
        Accessor(ParsedDicomCache& cache,
                 const std::string& key);

        bool IsValid() const;

        const boost::shared_ptr<Orthanc::ParsedDicomFile>& GetDicom() const
        {
          return GetItem().GetDicom();
        }

        size_t GetFileSize() const
        {
          return GetItem().GetFileSize();
        }

        bool HasPixelData() const
        {
          return GetItem().HasPixelData();
        }
      };
    };
  }
}
