/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "FrameRenderer.h"

#include "GrayscaleFrameRenderer.h"
#include "ColorFrameRenderer.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  static bool ComputePixelTransform(cairo_matrix_t& target,
                                    const SliceGeometry& viewportSlice,
                                    const SliceGeometry& frameSlice,
                                    double pixelSpacingX,
                                    double pixelSpacingY)
  {
    bool isOpposite;
    if (!GeometryToolbox::IsParallelOrOpposite(isOpposite, viewportSlice.GetNormal(), frameSlice.GetNormal()))
    {
      return false;
    }
    else
    {
      double x0, y0, x1, y1, x2, y2;
      viewportSlice.ProjectPoint(x0, y0, frameSlice.GetOrigin() 
                                 - 0.5 * pixelSpacingX * frameSlice.GetAxisX()
                                 - 0.5 * pixelSpacingY * frameSlice.GetAxisY());
      viewportSlice.ProjectPoint(x1, y1, frameSlice.GetOrigin() 
                                 + 0.5 * pixelSpacingX * frameSlice.GetAxisX()
                                 - 0.5 * pixelSpacingY * frameSlice.GetAxisY());
      viewportSlice.ProjectPoint(x2, y2, frameSlice.GetOrigin() 
                                 - 0.5 * pixelSpacingX * frameSlice.GetAxisX()
                                 + 0.5 * pixelSpacingY * frameSlice.GetAxisY());

      /**
       * Now we solve the system of linear equations Ax + b = x', given:
       *   A [0 ; 0] + b = [x0 ; y0]
       *   A [1 ; 0] + b = [x1 ; y1]
       *   A [0 ; 1] + b = [x2 ; y2]
       * <=>
       *   b = [x0 ; y0]
       *   A [1 ; 0] = [x1 ; y1] - b = [x1 - x0 ; y1 - y0]
       *   A [0 ; 1] = [x2 ; y2] - b = [x2 - x0 ; y2 - y0]
       * <=>
       *   b = [x0 ; y0]
       *   [a11 ; a21] = [x1 - x0 ; y1 - y0]
       *   [a12 ; a22] = [x2 - x0 ; y2 - y0]
       **/

      cairo_matrix_init(&target, x1 - x0, y1 - y0, x2 - x0, y2 - y0, x0, y0);

      return true;
    }
  }


  FrameRenderer::FrameRenderer(const SliceGeometry& viewportSlice,
                               const SliceGeometry& frameSlice,
                               double pixelSpacingX,
                               double pixelSpacingY,
                               bool isFullQuality) :
    viewportSlice_(viewportSlice),
    frameSlice_(frameSlice),
    pixelSpacingX_(pixelSpacingX),
    pixelSpacingY_(pixelSpacingY),
    isFullQuality_(isFullQuality)
  {
  }


  bool FrameRenderer::ComputeFrameExtent(double& x1,
                                         double& y1,
                                         double& x2,
                                         double& y2,
                                         const SliceGeometry& viewportSlice,
                                         const SliceGeometry& frameSlice,
                                         unsigned int frameWidth,
                                         unsigned int frameHeight,
                                         double pixelSpacingX,
                                         double pixelSpacingY)
  {
    bool isOpposite;
    if (!GeometryToolbox::IsParallelOrOpposite(isOpposite, viewportSlice.GetNormal(), frameSlice.GetNormal()))
    {
      return false;
    }
    else
    {
      cairo_matrix_t transform;
      if (!ComputePixelTransform(transform, viewportSlice, frameSlice, pixelSpacingX, pixelSpacingY))
      {
        return true;
      }
      
      x1 = 0;
      y1 = 0;
      cairo_matrix_transform_point(&transform, &x1, &y1);
      
      x2 = frameWidth;
      y2 = frameHeight;
      cairo_matrix_transform_point(&transform, &x2, &y2);
      
      if (x1 > x2)
      {
        std::swap(x1, x2);
      }

      if (y1 > y2)
      {
        std::swap(y1, y2);
      }

      return true;
    }
  }


  bool FrameRenderer::RenderLayer(CairoContext& context,
                                  const ViewportGeometry& view)
  {
    if (!style_.visible_)
    {
      return true;
    }

    if (display_.get() == NULL)
    {
      if (!ComputePixelTransform(transform_, viewportSlice_, frameSlice_, pixelSpacingX_, pixelSpacingY_))
      {
        return true;
      }

      display_.reset(GenerateDisplay(style_));
    }

    assert(display_.get() != NULL);

    cairo_t *cr = context.GetObject();

    cairo_save(cr);

    cairo_transform(cr, &transform_);
    //cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(cr, display_->GetObject(), 0, 0);

    switch (style_.interpolation_)
    {
      case ImageInterpolation_Nearest:
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
        break;

      case ImageInterpolation_Linear:
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    cairo_paint_with_alpha(cr, style_.alpha_);

    if (style_.drawGrid_)
    {
      context.SetSourceColor(style_.drawColor_);
      cairo_set_line_width(cr, 0.5 / view.GetZoom());

      for (unsigned int x = 0; x <= display_->GetWidth(); x++)
      {
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, display_->GetHeight());
      }

      for (unsigned int y = 0; y <= display_->GetHeight(); y++)
      {
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, display_->GetWidth(), y);
      }

      cairo_stroke(cr);
    }

    cairo_restore(cr);

    return true;
  }


  void FrameRenderer::SetLayerStyle(const RenderStyle& style)
  {
    style_ = style;
    display_.reset(NULL);
  }


  ILayerRenderer* FrameRenderer::CreateRenderer(Orthanc::ImageAccessor* frame,
                                                const SliceGeometry& viewportSlice,
                                                const SliceGeometry& frameSlice,
                                                const OrthancPlugins::IDicomDataset& dicom,
                                                double pixelSpacingX,
                                                double pixelSpacingY,
                                                bool isFullQuality)
  {  
    std::auto_ptr<Orthanc::ImageAccessor> protect(frame);

    if (frame->GetFormat() == Orthanc::PixelFormat_RGB24)
    {
      return new ColorFrameRenderer(protect.release(), viewportSlice, frameSlice, 
                                    pixelSpacingX, pixelSpacingY, isFullQuality);
    }
    else
    {
      DicomFrameConverter converter;
      converter.ReadParameters(dicom);
      return new GrayscaleFrameRenderer(protect.release(), converter, viewportSlice, frameSlice, 
                                        pixelSpacingX, pixelSpacingY, isFullQuality);
    }
  }
}
