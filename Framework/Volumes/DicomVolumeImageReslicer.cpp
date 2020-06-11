/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "DicomVolumeImageReslicer.h"

#include <OrthancException.h>

namespace OrthancStone
{
  class DicomVolumeImageReslicer::Slice : public IVolumeSlicer::IExtractedSlice
  {
  private:
    DicomVolumeImageReslicer&  that_;
    CoordinateSystem3D         cuttingPlane_;
      
  public:
    Slice(DicomVolumeImageReslicer& that,
          const CoordinateSystem3D& cuttingPlane) :
      that_(that),
      cuttingPlane_(cuttingPlane)
    {
    }
      
    virtual bool IsValid()
    {
      return true;
    }

    virtual uint64_t GetRevision()
    {
      return that_.volume_->GetRevision();
    }

    virtual ISceneLayer* CreateSceneLayer(const ILayerStyleConfigurator* configurator,
                                          const CoordinateSystem3D& cuttingPlane)
    {
      VolumeReslicer& reslicer = that_.reslicer_;
        
      if (configurator == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Must provide a layer style configurator");
      }
        
      reslicer.SetOutputFormat(that_.volume_->GetPixelData().GetFormat());
      reslicer.Apply(that_.volume_->GetPixelData(),
                     that_.volume_->GetGeometry(),
                     cuttingPlane);

      if (reslicer.IsSuccess())
      {
        std::unique_ptr<TextureBaseSceneLayer> layer
          (configurator->CreateTextureFromDicom(reslicer.GetOutputSlice(),
                                                that_.volume_->GetDicomParameters()));
        if (layer.get() == NULL)
        {
          return NULL;
        }

        double s = reslicer.GetPixelSpacing();
        layer->SetPixelSpacing(s, s);
        layer->SetOrigin(reslicer.GetOutputExtent().GetX1() + 0.5 * s,
                         reslicer.GetOutputExtent().GetY1() + 0.5 * s);

        // TODO - Angle!!
                           
        return layer.release();
      }
      else
      {
        return NULL;
      }          
    }
  };
    

  DicomVolumeImageReslicer::DicomVolumeImageReslicer(const boost::shared_ptr<DicomVolumeImage>& volume) :
    volume_(volume)
  {
    if (volume.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }

    
  IVolumeSlicer::IExtractedSlice* DicomVolumeImageReslicer::ExtractSlice(const CoordinateSystem3D& cuttingPlane)
  {
    if (volume_->HasGeometry())
    {
      return new Slice(*this, cuttingPlane);
    }
    else
    {
      return new IVolumeSlicer::InvalidSlice;
    }
  }
}
