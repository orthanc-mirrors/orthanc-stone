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

//      for (unsigned int i = 0; i < 100; i++)
//      {
//        for (unsigned int j = 0; j < 50; j++)
//        {
//          if ((i + j) % 2 == 1)
//          {
//            Orthanc::ImageAccessor region;
//            mask_->GetRegion(region, i* 20, j * 20, 20, 20);
//            Orthanc::ImageProcessing::Set(region, 255);
//          }
//        }
//      }
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
          if (*p == 0)
            *q = foreground_;
          // else keep the underlying pixel value
        }
      }

    }
  }

  // from https://www.geeksforgeeks.org/how-to-check-if-a-given-point-lies-inside-a-polygon/
  // Given three colinear points p, q, r, the function checks if
  // point q lies on line segment 'pr'
  bool onSegment(const MaskPoint& p, const MaskPoint& q, const MaskPoint& r)
  {
      if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
              q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
          return true;
      return false;
  }

  // To find orientation of ordered triplet (p, q, r).
  // The function returns following values
  // 0 --> p, q and r are colinear
  // 1 --> Clockwise
  // 2 --> Counterclockwise
  int orientation(const MaskPoint& p, const MaskPoint& q, const MaskPoint& r)
  {
      int val = (q.y - p.y) * (r.x - q.x) -
                (q.x - p.x) * (r.y - q.y);

      if (val == 0) return 0;  // colinear
      return (val > 0)? 1: 2; // clock or counterclock wise
  }

  // The function that returns true if line segment 'p1q1'
  // and 'p2q2' intersect.
  bool doIntersect(const MaskPoint& p1, const MaskPoint& q1, const MaskPoint& p2, const MaskPoint& q2)
  {
      // Find the four orientations needed for general and
      // special cases
      int o1 = orientation(p1, q1, p2);
      int o2 = orientation(p1, q1, q2);
      int o3 = orientation(p2, q2, p1);
      int o4 = orientation(p2, q2, q1);

      // General case
      if (o1 != o2 && o3 != o4)
          return true;

      // Special Cases
      // p1, q1 and p2 are colinear and p2 lies on segment p1q1
      if (o1 == 0 && onSegment(p1, p2, q1)) return true;

      // p1, q1 and p2 are colinear and q2 lies on segment p1q1
      if (o2 == 0 && onSegment(p1, q2, q1)) return true;

      // p2, q2 and p1 are colinear and p1 lies on segment p2q2
      if (o3 == 0 && onSegment(p2, p1, q2)) return true;

       // p2, q2 and q1 are colinear and q1 lies on segment p2q2
      if (o4 == 0 && onSegment(p2, q1, q2)) return true;

      return false; // Doesn't fall in any of the above cases
  }

  // Define Infinite (Using INT_MAX caused overflow problems)
  #define MASK_INF 1000000

  // Returns true if the point p lies inside the polygon[] with n vertices
  bool isInside(const std::vector<MaskPoint>& polygon, const MaskPoint& p)
  {
      // There must be at least 3 vertices in polygon[]
      if (polygon.size() < 3)  return false;

      // Create a point for line segment from p to infinite
      MaskPoint extreme = {MASK_INF, p.y};

      // Count intersections of the above line with sides of polygon
      int count = 0, i = 0;
      do
      {
          int next = (i+1) % polygon.size();

          // Check if the line segment from 'p' to 'extreme' intersects
          // with the line segment from 'polygon[i]' to 'polygon[next]'
          if (doIntersect(polygon[i], polygon[next], p, extreme))
          {
              // If the point 'p' is colinear with line segment 'i-next',
              // then check if it lies on segment. If it lies, return true,
              // otherwise false
              if (orientation(polygon[i], p, polygon[next]) == 0)
                 return onSegment(polygon[i], p, polygon[next]);

              count++;
          }
          i = next;
      } while (i != 0);

      // Return true if count is odd, false otherwise
      return count&1;  // Same as (count%2 == 1)
  }


  void RadiographyMaskLayer::DrawMask() const
  {
    unsigned int left;
    unsigned int right;
    unsigned int top;
    unsigned int bottom;

    ComputeMaskExtent(left, right, top, bottom, corners_);

    Orthanc::ImageProcessing::Set(*mask_, 0);

    MaskPoint p(left, top);
    for (p.y = top; p.y <= bottom; p.y++)
    {
      unsigned char* q = reinterpret_cast<unsigned char*>(mask_->GetRow(p.y));
      for (p.x = left; p.x <= right; p.x++, q++)
      {
        if (isInside(corners_, p))
        {
          *q = 255;
        }
      }
    }

//    Orthanc::ImageAccessor region;
//    mask_->GetRegion(region, 100, 100, 1000, 1000);
//    Orthanc::ImageProcessing::Set(region, 255);
  }

}
