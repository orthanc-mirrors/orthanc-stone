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


#include "ParseDicomFileCommand.h"

#include <Core/OrthancException.h>

#include <boost/filesystem/path.hpp>

namespace OrthancStone
{
  void ParseDicomFileCommand::SuccessMessage::Setup()
  {
    if (!dicom_->GetTagValue(sopInstanceUid_, Orthanc::DICOM_TAG_SOP_INSTANCE_UID))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                      "DICOM instance missing tag SOPInstanceUID");
    }
  }


  ParseDicomFileCommand::SuccessMessage::SuccessMessage(const ParseDicomFileCommand& command,
                                                        DcmFileFormat& dicom,
                                                        size_t fileSize,
                                                        bool hasPixelData) :
    OriginMessage(command),
    fileSize_(fileSize),
    hasPixelData_(hasPixelData)
  {
    dicom_.reset(new Orthanc::ParsedDicomFile(dicom));
    Setup();
  }


  ParseDicomFileCommand::SuccessMessage::SuccessMessage(const ParseDicomFileCommand& command,
                                                        boost::shared_ptr<Orthanc::ParsedDicomFile> dicom,
                                                        size_t fileSize,
                                                        bool hasPixelData) :
    OriginMessage(command),
    dicom_(dicom),
    fileSize_(fileSize),
    hasPixelData_(hasPixelData)
  {
    Setup();
  }


  std::string ParseDicomFileCommand::GetDicomDirPath(const std::string& dicomDirPath,
                                                     const std::string& file)
  {
    std::string tmp = file;

#if !defined(_WIN32)
    std::replace(tmp.begin(), tmp.end(), '\\', '/');
#endif

    boost::filesystem::path base = boost::filesystem::path(dicomDirPath).parent_path();

    return (base / tmp).string();
  }
}
