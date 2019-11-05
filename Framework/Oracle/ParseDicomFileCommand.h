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

#if !defined(ORTHANC_ENABLE_DCMTK)
#  error The macro ORTHANC_ENABLE_DCMTK must be defined
#endif

#if ORTHANC_ENABLE_DCMTK != 1
#  error Support for DCMTK must be enabled to use ParseDicomFileCommand
#endif

#include "../Messages/IMessage.h"
#include "OracleCommandBase.h"

#include <Core/DicomParsing/ParsedDicomFile.h>

#include <dcmtk/dcmdata/dcfilefo.h>

namespace OrthancStone
{
  class ParseDicomFileCommand : public OracleCommandBase
  {
  public:
    class SuccessMessage : public OriginMessage<ParseDicomFileCommand>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

    private:
      boost::shared_ptr<Orthanc::ParsedDicomFile>  dicom_;
      size_t                                       fileSize_;
      bool                                         hasPixelData_;
      std::string                                  sopInstanceUid_;

      void Setup();
      
    public:
      SuccessMessage(const ParseDicomFileCommand& command,
                     DcmFileFormat& dicom,
                     size_t fileSize,
                     bool hasPixelData);
      
      SuccessMessage(const ParseDicomFileCommand& command,
                     boost::shared_ptr<Orthanc::ParsedDicomFile> dicom,
                     size_t fileSize,
                     bool hasPixelData);
      
      boost::shared_ptr<Orthanc::ParsedDicomFile> GetDicom() const
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
      
      const std::string& GetSopInstanceUid() const
      {
        return sopInstanceUid_;
      }
    };

  private:
    std::string  path_;
    bool         pixelDataIncluded_;

  public:
    ParseDicomFileCommand(const std::string& path) :
      path_(path),
      pixelDataIncluded_(true)
    {
    }

    ParseDicomFileCommand(const std::string& dicomDirPath,
                          const std::string& file) :
      path_(GetDicomDirPath(dicomDirPath, file)),
      pixelDataIncluded_(true)
    {
    }

    static std::string GetDicomDirPath(const std::string& dicomDirPath,
                                       const std::string& file);

    virtual Type GetType() const
    {
      return Type_ParseDicomFile;
    }

    const std::string& GetPath() const
    {
      return path_;
    }

    bool IsPixelDataIncluded() const
    {
      return pixelDataIncluded_;
    }

    void SetPixelDataIncluded(bool included)
    {
      pixelDataIncluded_ = included;
    }
  };
}
