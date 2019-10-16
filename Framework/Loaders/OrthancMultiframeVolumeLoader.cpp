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


#include "OrthancMultiframeVolumeLoader.h"

#include <Core/Endianness.h>
#include <Core/Toolbox.h>

namespace OrthancStone
{
  class OrthancMultiframeVolumeLoader::LoadRTDoseGeometry : public LoaderStateMachine::State
  {
  private:
    std::auto_ptr<Orthanc::DicomMap>  dicom_;

  public:
    LoadRTDoseGeometry(OrthancMultiframeVolumeLoader& that,
                       Orthanc::DicomMap* dicom) :
      State(that),
      dicom_(dicom)
    {
      if (dicom == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

    }

    virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      // Complete the DICOM tags with just-received "Grid Frame Offset Vector"
      std::string s = Orthanc::Toolbox::StripSpaces(message.GetAnswer());
      dicom_->SetValue(Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR, s, false);

      GetLoader<OrthancMultiframeVolumeLoader>().SetGeometry(*dicom_);
    }      
  };


  static std::string GetSopClassUid(const Orthanc::DicomMap& dicom)
  {
    std::string s;
    if (!dicom.LookupStringValue(s, Orthanc::DICOM_TAG_SOP_CLASS_UID, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                      "DICOM file without SOP class UID");
    }
    else
    {
      return s;
    }
  }
    

  class OrthancMultiframeVolumeLoader::LoadGeometry : public State
  {
  public:
    LoadGeometry(OrthancMultiframeVolumeLoader& that) :
    State(that)
    {
    }
      
    virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      OrthancMultiframeVolumeLoader& loader = GetLoader<OrthancMultiframeVolumeLoader>();
        
      Json::Value body;
      message.ParseJsonBody(body);
        
      if (body.type() != Json::objectValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      std::auto_ptr<Orthanc::DicomMap> dicom(new Orthanc::DicomMap);
      dicom->FromDicomAsJson(body);

      if (StringToSopClassUid(GetSopClassUid(*dicom)) == SopClassUid_RTDose)
      {
        // Download the "Grid Frame Offset Vector" DICOM tag, that is
        // mandatory for RT-DOSE, but is too long to be returned by default
          
        std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
        command->SetUri("/instances/" + loader.GetInstanceId() + "/content/" +
                        Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR.Format());
        command->SetPayload(new LoadRTDoseGeometry(loader, dicom.release()));

        Schedule(command.release());
      }
      else
      {
        loader.SetGeometry(*dicom);
      }
    }
  };

  class OrthancMultiframeVolumeLoader::LoadTransferSyntax : public State
  {
  public:
    LoadTransferSyntax(OrthancMultiframeVolumeLoader& that) :
    State(that)
    {
    }
      
    virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      GetLoader<OrthancMultiframeVolumeLoader>().SetTransferSyntax(message.GetAnswer());
    }
  };

  class OrthancMultiframeVolumeLoader::LoadUncompressedPixelData : public State
  {
  public:
    LoadUncompressedPixelData(OrthancMultiframeVolumeLoader& that) :
    State(that)
    {
    }
      
    virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      GetLoader<OrthancMultiframeVolumeLoader>().SetUncompressedPixelData(message.GetAnswer());
    }
  };

  const std::string& OrthancMultiframeVolumeLoader::GetInstanceId() const
  {
    if (IsActive())
    {
      return instanceId_;
    }
    else
    {
      LOG(ERROR) << "OrthancMultiframeVolumeLoader::GetInstanceId(): (!IsActive())";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  void OrthancMultiframeVolumeLoader::ScheduleFrameDownloads()
  {
    if (transferSyntaxUid_.empty() ||
        !volume_->HasGeometry())
    {
      return;
    }
    /*
      1.2.840.10008.1.2	Implicit VR Endian: Default Transfer Syntax for DICOM
      1.2.840.10008.1.2.1	Explicit VR Little Endian
      1.2.840.10008.1.2.2	Explicit VR Big Endian

      See https://www.dicomlibrary.com/dicom/transfer-syntax/
    */
    if (transferSyntaxUid_ == "1.2.840.10008.1.2" ||
        transferSyntaxUid_ == "1.2.840.10008.1.2.1" ||
        transferSyntaxUid_ == "1.2.840.10008.1.2.2")
    {
      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetHttpHeader("Accept-Encoding", "gzip");
      command->SetUri("/instances/" + instanceId_ + "/content/" +
                      Orthanc::DICOM_TAG_PIXEL_DATA.Format() + "/0");
      command->SetPayload(new LoadUncompressedPixelData(*this));
      Schedule(command.release());
    }
    else
    {
      throw Orthanc::OrthancException(
        Orthanc::ErrorCode_NotImplemented,
        "No support for multiframe instances with transfer syntax: " + transferSyntaxUid_);
    }
  }
      

  void OrthancMultiframeVolumeLoader::SetTransferSyntax(const std::string& transferSyntax)
  {
    transferSyntaxUid_ = Orthanc::Toolbox::StripSpaces(transferSyntax);
    ScheduleFrameDownloads();
  }

  void OrthancMultiframeVolumeLoader::SetGeometry(const Orthanc::DicomMap& dicom)
  {
    DicomInstanceParameters parameters(dicom);
    volume_->SetDicomParameters(parameters);
      
    Orthanc::PixelFormat format;
    if (!parameters.GetImageInformation().ExtractPixelFormat(format, true))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    double spacingZ;
    switch (parameters.GetSopClassUid())
    {
      case SopClassUid_RTDose:
        spacingZ = parameters.GetThickness();
        break;

      default:
        throw Orthanc::OrthancException(
          Orthanc::ErrorCode_NotImplemented,
          "No support for multiframe instances with SOP class UID: " + GetSopClassUid(dicom));
    }

    const unsigned int width = parameters.GetImageInformation().GetWidth();
    const unsigned int height = parameters.GetImageInformation().GetHeight();
    const unsigned int depth = parameters.GetImageInformation().GetNumberOfFrames();

    {
      VolumeImageGeometry geometry;
      geometry.SetSizeInVoxels(width, height, depth);
      geometry.SetAxialGeometry(parameters.GetGeometry());
      geometry.SetVoxelDimensions(parameters.GetPixelSpacingX(),
                                  parameters.GetPixelSpacingY(), spacingZ);
      volume_->Initialize(geometry, format, true /* Do compute range */);
    }

    volume_->GetPixelData().Clear();

    ScheduleFrameDownloads();



    BroadcastMessage(DicomVolumeImage::GeometryReadyMessage(*volume_));
  }


  ORTHANC_FORCE_INLINE
  static void CopyPixel(uint32_t& target, const void* source)
  {
    // TODO - check alignement?
    target = le32toh(*reinterpret_cast<const uint32_t*>(source));
  }

  ORTHANC_FORCE_INLINE
    static void CopyPixel(uint16_t& target, const void* source)
  {
    // TODO - check alignement?
    target = le16toh(*reinterpret_cast<const uint16_t*>(source));
  }

  template <typename T>
  void OrthancMultiframeVolumeLoader::CopyPixelData(const std::string& pixelData)
  {
    ImageBuffer3D& target = volume_->GetPixelData();
      
    const unsigned int bpp = target.GetBytesPerPixel();
    const unsigned int width = target.GetWidth();
    const unsigned int height = target.GetHeight();
    const unsigned int depth = target.GetDepth();

    if (pixelData.size() != bpp * width * height * depth)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                      "The pixel data has not the proper size");
    }

    if (pixelData.empty())
    {
      return;
    }

    const uint8_t* source = reinterpret_cast<const uint8_t*>(pixelData.c_str());

    for (unsigned int z = 0; z < depth; z++)
    {
      ImageBuffer3D::SliceWriter writer(target, VolumeProjection_Axial, z);

      assert (writer.GetAccessor().GetWidth() == width &&
              writer.GetAccessor().GetHeight() == height);

      for (unsigned int y = 0; y < height; y++)
      {
        assert(sizeof(T) == Orthanc::GetBytesPerPixel(target.GetFormat()));

        T* target = reinterpret_cast<T*>(writer.GetAccessor().GetRow(y));

        for (unsigned int x = 0; x < width; x++)
        {
          CopyPixel(*target, source);
            
          target ++;
          source += bpp;
        }
      }
    }
  }

  void OrthancMultiframeVolumeLoader::SetUncompressedPixelData(const std::string& pixelData)
  {
    switch (volume_->GetPixelData().GetFormat())
    {
      case Orthanc::PixelFormat_Grayscale32:
        CopyPixelData<uint32_t>(pixelData);
        break;
      case Orthanc::PixelFormat_Grayscale16:
        CopyPixelData<uint16_t>(pixelData);
        break;
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    volume_->IncrementRevision();

    pixelDataLoaded_ = true;
    BroadcastMessage(DicomVolumeImage::ContentUpdatedMessage(*volume_));
  }
  
  bool OrthancMultiframeVolumeLoader::HasGeometry() const
  {
    return volume_->HasGeometry();
  }

  const OrthancStone::VolumeImageGeometry& OrthancMultiframeVolumeLoader::GetImageGeometry() const
  {
    return volume_->GetGeometry();
  }

  OrthancMultiframeVolumeLoader::OrthancMultiframeVolumeLoader(boost::shared_ptr<DicomVolumeImage> volume,
                                                               IOracle& oracle,
                                                               IObservable& oracleObservable) :
    LoaderStateMachine(oracle, oracleObservable),
    IObservable(oracleObservable.GetBroker()),
    volume_(volume),
    pixelDataLoaded_(false)
  {
    if (volume.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }

  OrthancMultiframeVolumeLoader::~OrthancMultiframeVolumeLoader()
  {
    LOG(TRACE) << "OrthancMultiframeVolumeLoader::~OrthancMultiframeVolumeLoader()";
  }

  void OrthancMultiframeVolumeLoader::LoadInstance(const std::string& instanceId)
  {
    Start();

    instanceId_ = instanceId;

    {
      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetHttpHeader("Accept-Encoding", "gzip");
      command->SetUri("/instances/" + instanceId + "/tags");
      command->SetPayload(new LoadGeometry(*this));
      Schedule(command.release());
    }

    {
      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetUri("/instances/" + instanceId + "/metadata/TransferSyntax");
      command->SetPayload(new LoadTransferSyntax(*this));
      Schedule(command.release());
    }
  }
}
