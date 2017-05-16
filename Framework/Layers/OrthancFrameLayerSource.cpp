/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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


#include "OrthancFrameLayerSource.h"

#include "FrameRenderer.h"
#include "../../Resources/Orthanc/Core/Images/PngReader.h"
#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <boost/lexical_cast.hpp>


namespace OrthancStone
{
  class OrthancFrameLayerSource::Operation : public Orthanc::IDynamicObject
  {
  private:
    Content        content_;
    SliceGeometry  viewportSlice_;

  public:
    Operation(Content content) : content_(content)
    {
    }

    void SetViewportSlice(const SliceGeometry& slice)
    {
      viewportSlice_ = slice;
    }

    const SliceGeometry& GetViewportSlice() const
    {
      return viewportSlice_;
    } 

    Content GetContent() const
    {
      return content_;
    }
  };
    

  OrthancFrameLayerSource::OrthancFrameLayerSource(IWebService& orthanc,
                                                   const std::string& instanceId,
                                                   unsigned int frame) :
    orthanc_(orthanc),
    instanceId_(instanceId),
    frame_(frame)
  {
    orthanc_.ScheduleGetRequest(*this,
                                "/instances/" + instanceId + "/tags",
                                new Operation(Content_Tags));
  }

  
  void OrthancFrameLayerSource::SetObserver(IObserver& observer)
  {
    LayerSourceBase::SetObserver(observer);

    if (dataset_.get() != NULL)
    {
      NotifySourceChange();
    }
  }

  void OrthancFrameLayerSource::NotifyError(const std::string& uri,
                                            Orthanc::IDynamicObject* payload)
  {
    LOG(ERROR) << "Cannot download " << uri;
  }

  void OrthancFrameLayerSource::NotifySuccess(const std::string& uri,
                                              const void* answer,
                                              size_t answerSize,
                                              Orthanc::IDynamicObject* payload)
  {
    std::auto_ptr<Operation> operation(reinterpret_cast<Operation*>(payload));

    if (operation.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else if (operation->GetContent() == Content_Tags)
    {
      dataset_.reset(new OrthancPlugins::FullOrthancDataset(answer, answerSize));

      DicomFrameConverter converter;
      converter.ReadParameters(*dataset_);
      format_ = converter.GetExpectedPixelFormat();

      NotifySourceChange();
    }
    else if (operation->GetContent() == Content_Frame)
    {
      std::auto_ptr<Orthanc::PngReader>  image(new Orthanc::PngReader);
      image->ReadFromMemory(answer, answerSize);
        
      if (format_ == Orthanc::PixelFormat_SignedGrayscale16)
      {
        if (image->GetFormat() == Orthanc::PixelFormat_Grayscale16)
        {
          image->SetFormat(Orthanc::PixelFormat_SignedGrayscale16);
        }
        else
        {
          NotifyLayerReady(NULL, *this, operation->GetViewportSlice());
        }
      }
        
      SliceGeometry frameSlice(*dataset_);
      NotifyLayerReady(FrameRenderer::CreateRenderer(image.release(),
                                                     operation->GetViewportSlice(),
                                                     frameSlice, *dataset_, 1, 1, true),
                       *this, operation->GetViewportSlice());
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }

  
  bool OrthancFrameLayerSource::GetExtent(double& x1,
                                          double& y1,
                                          double& x2,
                                          double& y2,
                                          const SliceGeometry& viewportSlice /* ignored */)
  {
    if (dataset_.get() == NULL)
    {
      return false;
    }
    else
    {
      // Assume that PixelSpacingX == PixelSpacingY == 1

      OrthancPlugins::DicomDatasetReader reader(*dataset_);
    
      unsigned int width, height;

      if (!reader.GetUnsignedIntegerValue(width, OrthancPlugins::DICOM_TAG_COLUMNS) ||
          !reader.GetUnsignedIntegerValue(height, OrthancPlugins::DICOM_TAG_ROWS))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      x1 = 0;
      y1 = 0;
      x2 = static_cast<double>(width);
      y2 = static_cast<double>(height);

      return true;
    }
  }

  
  void OrthancFrameLayerSource::ScheduleLayerCreation(const SliceGeometry& viewportSlice)
  {
    if (dataset_.get() == NULL)
    {
      NotifyLayerReady(NULL, *this, viewportSlice);
    }
    else
    {
      std::string uri = ("/instances/" + instanceId_ + "/frames/" + 
                         boost::lexical_cast<std::string>(frame_));

      std::string compressed;

      switch (format_)
      {
        case Orthanc::PixelFormat_RGB24:
          uri += "/preview";
          break;

        case Orthanc::PixelFormat_Grayscale16:
          uri += "/image-uint16";
          break;

        case Orthanc::PixelFormat_SignedGrayscale16:
          uri += "/image-int16";
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      std::auto_ptr<Operation> operation(new Operation(Content_Frame));
      operation->SetViewportSlice(viewportSlice);
      orthanc_.ScheduleGetRequest(*this, uri, operation.release());
    }
  }
}
