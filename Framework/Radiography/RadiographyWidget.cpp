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


#include "RadiographyWidget.h"

#include <Core/Images/Image.h>


namespace OrthancStone
{
  bool RadiographyWidget::RenderInternal(unsigned int width,
                                         unsigned int height,
                                         ImageInterpolation interpolation)
  {
    float windowCenter, windowWidth;
    scene_.GetWindowingWithDefault(windowCenter, windowWidth);
      
    float x0 = windowCenter - windowWidth / 2.0f;
    float x1 = windowCenter + windowWidth / 2.0f;

    if (windowWidth <= 0.001f)  // Avoid division by zero at (*)
    {
      return false;
    }
    else
    {
      if (floatBuffer_.get() == NULL ||
          floatBuffer_->GetWidth() != width ||
          floatBuffer_->GetHeight() != height)
      {
        floatBuffer_.reset(new Orthanc::Image(Orthanc::PixelFormat_Float32, width, height, false));
      }

      if (cairoBuffer_.get() == NULL ||
          cairoBuffer_->GetWidth() != width ||
          cairoBuffer_->GetHeight() != height)
      {
        cairoBuffer_.reset(new CairoSurface(width, height));
      }

      scene_.Render(*floatBuffer_, GetView().GetMatrix(), interpolation);
        
      // Conversion from Float32 to BGRA32 (cairo). Very similar to
      // GrayscaleFrameRenderer => TODO MERGE?

      Orthanc::ImageAccessor target;
      cairoBuffer_->GetWriteableAccessor(target);

      float scaling = 255.0f / (x1 - x0);
        
      for (unsigned int y = 0; y < height; y++)
      {
        const float* p = reinterpret_cast<const float*>(floatBuffer_->GetConstRow(y));
        uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));

        for (unsigned int x = 0; x < width; x++, p++, q += 4)
        {
          uint8_t v = 0;
          if (*p >= x1)
          {
            v = 255;
          }
          else if (*p <= x0)
          {
            v = 0;
          }
          else
          {
            // https://en.wikipedia.org/wiki/Linear_interpolation
            v = static_cast<uint8_t>(scaling * (*p - x0));  // (*)
          }

          if (invert_)
          {
            v = 255 - v;
          }

          q[0] = v;
          q[1] = v;
          q[2] = v;
          q[3] = 255;
        }
      }

      return true;
    }
  }


  bool RadiographyWidget::RenderScene(CairoContext& context,
                                      const ViewportGeometry& view)
  {
    cairo_t* cr = context.GetObject();

    if (RenderInternal(context.GetWidth(), context.GetHeight(), interpolation_))
    {
      // https://www.cairographics.org/FAQ/#paint_from_a_surface
      cairo_save(cr);
      cairo_identity_matrix(cr);
      cairo_set_source_surface(cr, cairoBuffer_->GetObject(), 0, 0);
      cairo_paint(cr);
      cairo_restore(cr);
    }
    else
    {
      // https://www.cairographics.org/FAQ/#clear_a_surface
      context.SetSourceColor(0, 0, 0);
      cairo_paint(cr);
    }

    if (hasSelection_)
    {
      scene_.DrawBorder(context, selectedLayer_, view.GetZoom());
    }

    return true;
  }


  RadiographyWidget::RadiographyWidget(MessageBroker& broker,
                                       RadiographyScene& scene,
                                       const std::string& name) :
    WorldSceneWidget(name),
    IObserver(broker),
    scene_(scene),
    invert_(false),
    interpolation_(ImageInterpolation_Nearest),
    hasSelection_(false),
    selectedLayer_(0)    // Dummy initialization
  {
    scene.RegisterObserverCallback(
      new Callable<RadiographyWidget, RadiographyScene::GeometryChangedMessage>
      (*this, &RadiographyWidget::OnGeometryChanged));

    scene.RegisterObserverCallback(
      new Callable<RadiographyWidget, RadiographyScene::ContentChangedMessage>
      (*this, &RadiographyWidget::OnContentChanged));
  }


  void RadiographyWidget::Select(size_t layer)
  {
    hasSelection_ = true;
    selectedLayer_ = layer;
  }


  bool RadiographyWidget::LookupSelectedLayer(size_t& layer)
  {
    if (hasSelection_)
    {
      layer = selectedLayer_;
      return true;
    }
    else
    {
      return false;
    }
  }

  
  void RadiographyWidget::OnGeometryChanged(const RadiographyScene::GeometryChangedMessage& message)
  {
    LOG(INFO) << "Geometry has changed";
    FitContent();
  }

  
  void RadiographyWidget::OnContentChanged(const RadiographyScene::ContentChangedMessage& message)
  {
    LOG(INFO) << "Content has changed";
    NotifyContentChanged();
  }

  
  void RadiographyWidget::SetInvert(bool invert)
  {
    if (invert_ != invert)
    {
      invert_ = invert;
      NotifyContentChanged();
    }
  }

  
  void RadiographyWidget::SwitchInvert()
  {
    invert_ = !invert_;
    NotifyContentChanged();
  }


  void RadiographyWidget::SetInterpolation(ImageInterpolation interpolation)
  {
    if (interpolation_ != interpolation)
    {
      interpolation_ = interpolation;
      NotifyContentChanged();
    }
  }
}
