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


#include "RadiographyMaskLayer.h"
#include "RadiographyDicomLayer.h"

#include "RadiographyScene.h"
#include "Core/Images/Image.h"
#include "Core/Images/ImageProcessing.h"
#include <Core/OrthancException.h>

namespace OrthancStone
{
  const unsigned char IN_MASK_VALUE = 0x00;
  const unsigned char OUT_MASK_VALUE = 0xFF;

  const AffineTransform2D& RadiographyMaskLayer::GetTransform() const
  {
    return dicomLayer_.GetTransform();
  }

  const AffineTransform2D& RadiographyMaskLayer::GetTransformInverse() const
  {
    return dicomLayer_.GetTransformInverse();
  }

  void ComputeMaskExtent(unsigned int& left, unsigned int& right, unsigned int& top, unsigned int& bottom, const std::vector<MaskPoint>& corners)
  {
    left = std::numeric_limits<unsigned int>::max();
    right = std::numeric_limits<unsigned int>::min();
    top = std::numeric_limits<unsigned int>::max();
    bottom = std::numeric_limits<unsigned int>::min();

    for (size_t i = 0; i < corners.size(); i++)
    {
      const MaskPoint& p = corners[i];
      left = std::min(p.x, left);
      right = std::max(p.x, right);
      bottom = std::max(p.y, bottom);
      top = std::min(p.y, top);
    }
  }

  void RadiographyMaskLayer::SetCorners(const std::vector<MaskPoint>& corners)
  {
    corners_ = corners;
    invalidated_ = true;
  }

  void RadiographyMaskLayer::Render(Orthanc::ImageAccessor& buffer,
                                    const AffineTransform2D& viewTransform,
                                    ImageInterpolation interpolation) const
  {
    if (dicomLayer_.GetWidth() == 0) // nothing to do if the DICOM layer is not displayed (or not loaded)
      return;

    if (invalidated_)
    {
      mask_.reset(new Orthanc::Image(Orthanc::PixelFormat_Grayscale8, dicomLayer_.GetWidth(), dicomLayer_.GetHeight(), false));

      DrawMask();

      invalidated_ = false;
    }

    {// rendering
      if (buffer.GetFormat() != Orthanc::PixelFormat_Float32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }

      unsigned int cropX, cropY, cropWidth, cropHeight;
      dicomLayer_.GetCrop(cropX, cropY, cropWidth, cropHeight);

      const AffineTransform2D t = AffineTransform2D::Combine(
            viewTransform, dicomLayer_.GetTransform(),
            AffineTransform2D::CreateOffset(cropX, cropY));

      Orthanc::ImageAccessor cropped;
      mask_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);

      Orthanc::Image tmp(Orthanc::PixelFormat_Grayscale8, buffer.GetWidth(), buffer.GetHeight(), false);

      t.Apply(tmp, cropped, interpolation, true /* clear */);

      // Blit
      const unsigned int width = buffer.GetWidth();
      const unsigned int height = buffer.GetHeight();

      for (unsigned int y = 0; y < height; y++)
      {
        float *q = reinterpret_cast<float*>(buffer.GetRow(y));
        const uint8_t *p = reinterpret_cast<uint8_t*>(tmp.GetRow(y));

        for (unsigned int x = 0; x < width; x++, p++, q++)
        {
          if (*p == OUT_MASK_VALUE)
            *q = foreground_;
          // else keep the underlying pixel value
        }
      }

    }
  }

  void RadiographyMaskLayer::DrawMask() const
  {
    unsigned int left;
    unsigned int right;
    unsigned int top;
    unsigned int bottom;

    ComputeMaskExtent(left, right, top, bottom, corners_);

    // first fill the complete image
    Orthanc::ImageProcessing::Set(*mask_, OUT_MASK_VALUE);

    {
      // from http://alienryderflex.com/polygon_fill/
      std::auto_ptr<int> raiiNodeX(new int(corners_.size()));

      // convert all control points to double only once
      std::vector<double> cpx;
      std::vector<double> cpy;
      int cpSize = corners_.size();
      for (size_t i = 0; i < corners_.size(); i++)
      {
        cpx.push_back((double)corners_[i].x);
        cpy.push_back((double)corners_[i].y);
      }

      std::vector<int> nodeX;
      nodeX.resize(cpSize);
      int  nodes, pixelX, pixelY, i, j, swap ;

      //  Loop through the rows of the image.
      for (pixelY = (int)top; pixelY < (int)bottom; pixelY++)
      {
        double y = (double)pixelY;
        //  Build a list of nodes.
        nodes = 0;
        j = cpSize - 1;

        for (i = 0; i < cpSize; i++)
        {
          if ((cpy[i] <= y && cpy[j] >=  y)
              ||  (cpy[j] <= y && cpy[i] >= y))
          {
            nodeX[nodes++]= (int)(cpx[i] + (y - cpy[i])/(cpy[j] - cpy[i]) *(cpx[j] - cpx[i]));
          }
          j=i;
        }

        //  Sort the nodes, via a simple “Bubble” sort.
        i=0;
        while (i < nodes-1)
        {
          if (nodeX[i] > nodeX[i+1])
          {
            swap = nodeX[i];
            nodeX[i] = nodeX[i+1];
            nodeX[i+1] = swap;
            if (i)
              i--;
          }
          else
          {
            i++;
          }
        }

        unsigned char* row = reinterpret_cast<unsigned char*>(mask_->GetRow(pixelY));
        //  Fill the pixels between node pairs.
        for (i=0; i<nodes; i+=2)
        {
          if   (nodeX[i  ]>=(int)right)
            break;
          if   (nodeX[i+1]>= (int)left)
          {
            if (nodeX[i  ]< (int)left )
              nodeX[i  ]=(int)left ;
            if (nodeX[i+1]> (int)right)
              nodeX[i+1]=(int)right;
            for (pixelX = nodeX[i]; pixelX <= nodeX[i+1]; pixelX++)
            {
              *(row + pixelX) = IN_MASK_VALUE;
            }
          }
        }
      }
    }
  }

}
