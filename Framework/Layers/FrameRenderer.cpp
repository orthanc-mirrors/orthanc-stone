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


#include "FrameRenderer.h"

#include "GrayscaleFrameRenderer.h"
#include "ColorFrameRenderer.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  FrameRenderer::FrameRenderer(const CoordinateSystem3D& frameSlice,
                               double pixelSpacingX,
                               double pixelSpacingY,
                               bool isFullQuality) :
    frameSlice_(frameSlice),
    pixelSpacingX_(pixelSpacingX),
    pixelSpacingY_(pixelSpacingY),
    isFullQuality_(isFullQuality)
  {
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
      display_.reset(GenerateDisplay(style_));
    }

    assert(display_.get() != NULL);

    cairo_t *cr = context.GetObject();

    cairo_save(cr);

    cairo_matrix_t transform;
    cairo_matrix_init_identity(&transform);
    cairo_matrix_scale(&transform, pixelSpacingX_, pixelSpacingY_);
    cairo_matrix_translate(&transform, -0.5, -0.5);
    cairo_transform(cr, &transform);

    //cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(cr, display_->GetObject(), 0, 0);

    switch (style_.interpolation_)
    {
      case ImageInterpolation_Nearest:
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
        break;

      case ImageInterpolation_Bilinear:
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
                                                const Slice& frameSlice,
                                                bool isFullQuality)
  {
    std::auto_ptr<Orthanc::ImageAccessor> protect(frame);

    if (frame->GetFormat() == Orthanc::PixelFormat_RGB24)
    {
      return new ColorFrameRenderer(protect.release(),
                                    frameSlice.GetGeometry(), 
                                    frameSlice.GetPixelSpacingX(),
                                    frameSlice.GetPixelSpacingY(), isFullQuality);
    }
    else
    {
      return new GrayscaleFrameRenderer(protect.release(),
                                        frameSlice.GetConverter(),
                                        frameSlice.GetGeometry(), 
                                        frameSlice.GetPixelSpacingX(),
                                        frameSlice.GetPixelSpacingY(), isFullQuality);
    }
  }
}
