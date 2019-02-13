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

  void RadiographyMaskLayer::DrawLine(const MaskPoint& start, const MaskPoint& end) const
  {
    int dx = (int)(end.x) - (int)(start.x);
    int dy = (int)(end.y) - (int)(start.y);

    if (std::abs(dx) > std::abs(dy))
    { // the line is closer to horizontal

      int incx = dx / std::abs(dx);
      double incy = (double)(dy)/(double)(dx);
      double y = (double)(start.y);
      for (int x = (int)(start.x); x != (int)(end.x); x += incx, y += incy)
      {
        unsigned char* p = reinterpret_cast<unsigned char*>(mask_->GetRow((int)(y + 0.5))) + x;
        *p = IN_MASK_VALUE;
      }
    }
    else
    { // the line is closer to vertical
      int incy = dy / std::abs(dy);
      double incx = (double)(dx)/(double)(dy);
      double x = (double)(start.x);
      for (int y = (int)(start.y); y != (int)(end.y); y += incy, x += incx)
      {
        unsigned char* p = reinterpret_cast<unsigned char*>(mask_->GetRow(y)) + (int)(x + 0.5);
        *p = IN_MASK_VALUE;
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

//    // draw lines
//    for (size_t i = 1; i < corners_.size(); i++)
//    {
//      DrawLine(corners_[i-1], corners_[i]);
//    }
//    DrawLine(corners_[corners_.size()-1], corners_[0]);

//    // fill between lines
//    MaskPoint p(left, top);
//    for (p.y = top; p.y <= bottom; p.y++)
//    {
//      unsigned char* q = reinterpret_cast<unsigned char*>(mask_->GetRow(p.y)) + left;
//      unsigned char previousPixelValue1 = OUT_MASK_VALUE;
//      unsigned char previousPixelValue2 = OUT_MASK_VALUE;
//      for (p.x = left; p.x <= right; p.x++, q++)
//      {
//        if (*p == OUT_MASK_VALUE && previousPixelValue1 == IN_MASK_VALUE && previousPixelValue2 == OUT_MASK_VALUE) // we just passed over a single one pixel line => start filling
//        {

//          *q = IN_MASK_VALUE;
//        }
//      }
//    }


//    MaskPoint p(left, top);
//    for (p.y = top; p.y <= bottom; p.y++)
//    {
//      unsigned char* q = reinterpret_cast<unsigned char*>(mask_->GetRow(p.y)) + left;
//      for (p.x = left; p.x <= right; p.x++, q++)
//      {
//        if (isInside(corners_, p))
//        {
//          *q = IN_MASK_VALUE;
//        }
//      }
//    }

  }

}
