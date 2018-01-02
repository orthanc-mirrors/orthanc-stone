/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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


#include "SingleFrameRendererFactory.h"

#include "FrameRenderer.h"
#include "../Toolbox/MessagingToolbox.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/FullOrthancDataset.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/DicomDatasetReader.h"
#include "../Toolbox/DicomFrameConverter.h"

namespace OrthancStone
{
  SingleFrameRendererFactory::SingleFrameRendererFactory(OrthancPlugins::IOrthancConnection& orthanc,
                                                         const std::string& instanceId,
                                                         unsigned int frame) :
    orthanc_(orthanc),
    instance_(instanceId),
    frame_(frame)
  {
    dicom_.reset(new OrthancPlugins::FullOrthancDataset(orthanc, "/instances/" + instanceId + "/tags"));

    DicomFrameConverter converter;
    converter.ReadParameters(*dicom_);
    format_ = converter.GetExpectedPixelFormat();
  }


  bool SingleFrameRendererFactory::GetExtent(double& x1,
                                             double& y1,
                                             double& x2,
                                             double& y2,
                                             const SliceGeometry& viewportSlice)
  {
    // Assume that PixelSpacingX == PixelSpacingY == 1

    OrthancPlugins::DicomDatasetReader reader(*dicom_);

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


  ILayerRenderer* SingleFrameRendererFactory::CreateLayerRenderer(const SliceGeometry& viewportSlice)
  {
    SliceGeometry frameSlice(*dicom_);
    return FrameRenderer::CreateRenderer(MessagingToolbox::DecodeFrame(orthanc_, instance_, frame_, format_), 
                                         viewportSlice, frameSlice, *dicom_, 1, 1, true);
  }


  ISliceableVolume& SingleFrameRendererFactory::GetSourceVolume() const
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
}
