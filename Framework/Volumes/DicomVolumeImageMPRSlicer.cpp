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


#include "DicomVolumeImageMPRSlicer.h"

#include "../StoneException.h"

#include <Core/OrthancException.h>
//#include <Core/Images/PngWriter.h>
#include <Core/Images/JpegWriter.h>

namespace OrthancStone
{
  void DicomVolumeImageMPRSlicer::Slice::CheckValid() const
  {
    if (!valid_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  DicomVolumeImageMPRSlicer::Slice::Slice(const DicomVolumeImage& volume,
                                          const CoordinateSystem3D& cuttingPlane) :
    volume_(volume),
    revision_(volume_.GetRevision())
  {
    valid_ = (volume_.HasDicomParameters() &&
              volume_.GetGeometry().DetectSlice(projection_, sliceIndex_, cuttingPlane));
  }


  VolumeProjection DicomVolumeImageMPRSlicer::Slice::GetProjection() const
  {
    CheckValid();
    return projection_;
  }


  unsigned int DicomVolumeImageMPRSlicer::Slice::GetSliceIndex() const
  {
    CheckValid();
    return sliceIndex_;
  }
  

  ISceneLayer* DicomVolumeImageMPRSlicer::Slice::CreateSceneLayer(const ILayerStyleConfigurator* configurator,
                                                                  const CoordinateSystem3D& cuttingPlane)
  {
    CheckValid();

    if (configurator == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer,
                                      "A style configurator is mandatory for textures");
    }

    std::auto_ptr<TextureBaseSceneLayer> texture;
        
    {
      const DicomInstanceParameters& parameters = volume_.GetDicomParameters();
      ImageBuffer3D::SliceReader reader(volume_.GetPixelData(), projection_, sliceIndex_);
      texture.reset(dynamic_cast<TextureBaseSceneLayer*>
                    (configurator->CreateTextureFromDicom(reader.GetAccessor(), parameters)));

      // <DEBUG-BLOCK>
#if 0
      Orthanc::JpegWriter writer;
      writer.SetQuality(60);
      static int index = 0;
      std::string filePath = "C:\\temp\\sliceReader_P";
      filePath += boost::lexical_cast<std::string>(projection_);
      filePath += "_I";
      filePath += boost::lexical_cast<std::string>(index);
      filePath += ".jpg";
      index++;
      writer.WriteToFile(filePath, reader.GetAccessor());
#endif
      // <END-OF-DEBUG-BLOCK>
    }
    
    const CoordinateSystem3D& system = volume_.GetGeometry().GetProjectionGeometry(projection_);
      
    double x0, y0, x1, y1;
    cuttingPlane.ProjectPoint(x0, y0, system.GetOrigin());
    cuttingPlane.ProjectPoint(x1, y1, system.GetOrigin() + system.GetAxisX());

    // <DEBUG-BLOCK>
#if 0
    {
      LOG(ERROR) << "+----------------------------------------------------+";
      LOG(ERROR) << "| DicomVolumeImageMPRSlicer::Slice::CreateSceneLayer |";
      LOG(ERROR) << "+----------------------------------------------------+";
      std::string projectionString;
      switch (projection_)
      {
      case VolumeProjection_Coronal:
        projectionString = "CORONAL";
        break;
      case VolumeProjection_Axial:
        projectionString = "CORONAL";
        break;
      case VolumeProjection_Sagittal:
        projectionString = "SAGITTAL";
        break;
      default:
        ORTHANC_ASSERT(false);
      }
      if(volume_.GetGeometry().GetDepth() == 200)
        LOG(ERROR) << "| CT     IMAGE 512x512 with projection " << projectionString;
      else
        LOG(ERROR) << "| RTDOSE IMAGE NNNxNNN with projection " << projectionString;
      LOG(ERROR) << "+----------------------------------------------------+";
      LOG(ERROR) << "| cuttingPlane = " << cuttingPlane;
      LOG(ERROR) << "| point to project = " << system.GetOrigin();
      LOG(ERROR) << "| result = x0: " << x0 << " y0: " << y0;
      LOG(ERROR) << "+----------------------- END ------------------------+";
    }
#endif
    // <END-OF-DEBUG-BLOCK>

#if 1 // BGO 2019-08-13
    // The sagittal coordinate system has a Y vector going down. The displayed
    // image (scene coords) has a Y vector pointing upwards (towards the patient 
    // coord Z index)
    // we need to flip the Y local coordinates to get the scene-coord offset.
    // TODO: this is quite ugly. Isn't there a better way?
    if(projection_ == VolumeProjection_Sagittal)
      texture->SetOrigin(x0, -y0);
    else
      texture->SetOrigin(x0, y0);
#else
    texture->SetOrigin(x0, y0);
#endif

    double dx = x1 - x0;
    double dy = y1 - y0;
    if (!LinearAlgebra::IsCloseToZero(dx) ||
        !LinearAlgebra::IsCloseToZero(dy))
    {
      texture->SetAngle(atan2(dy, dx));
    }
        
    Vector tmp = volume_.GetGeometry().GetVoxelDimensions(projection_);
    texture->SetPixelSpacing(tmp[0], tmp[1]);

    // <DEBUG-BLOCK>
    {
      //using std::endl;
      //std::stringstream ss;
      //ss << "DicomVolumeImageMPRSlicer::Slice::CreateSceneLayer | cuttingPlane = " << cuttingPlane << " | projection_ = " << projection_ << endl;
      //ss << "volume_.GetGeometry().GetProjectionGeometry(projection_) = " << system << endl;
      //ss << "cuttingPlane.ProjectPoint(x0, y0, system.GetOrigin()); --> | x0 = " << x0 << " | y0 = " << y0 << "| x1 = " << x1 << " | y1 = " << y1 << endl;
      //ss << "volume_.GetGeometry() = " << volume_.GetGeometry() << endl;
      //ss << "volume_.GetGeometry() = " << volume_.GetGeometry() << endl;
      //LOG(ERROR) << ss.str();
    }
    // <END-OF-DEBUG-BLOCK>

    return texture.release();
  }


  DicomVolumeImageMPRSlicer::~DicomVolumeImageMPRSlicer()
  {
    LOG(TRACE) << "DicomVolumeImageMPRSlicer::~DicomVolumeImageMPRSlicer()";
  }

  IVolumeSlicer::IExtractedSlice*
  DicomVolumeImageMPRSlicer::ExtractSlice(const CoordinateSystem3D& cuttingPlane)
  {
    if (volume_->HasGeometry())
    {
      return new Slice(*volume_, cuttingPlane);
    }
    else
    {
      return new IVolumeSlicer::InvalidSlice;
    }
  }
}
