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


#include "RadiographyWidget.h"

#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>

#include "RadiographyMaskLayer.h"

namespace OrthancStone
{

  bool RadiographyWidget::IsInvertedInternal() const
  {
    // MONOCHROME1 images must be inverted and the user can invert the 
    // image, too -> XOR the two
    return (scene_->GetPreferredPhotomotricDisplayMode() == 
            RadiographyPhotometricDisplayMode_Monochrome1) ^ invert_; 
  }

  void RadiographyWidget::RenderBackground(
    Orthanc::ImageAccessor& image, float minValue, float maxValue)
  {
    // wipe background before rendering
    float backgroundValue = minValue;

    switch (scene_->GetPreferredPhotomotricDisplayMode())
    {
    case RadiographyPhotometricDisplayMode_Monochrome1:
    case RadiographyPhotometricDisplayMode_Default:
      if (IsInvertedInternal())
        backgroundValue = maxValue;
      else
        backgroundValue = minValue;
      break;
    case RadiographyPhotometricDisplayMode_Monochrome2:
      if (IsInvertedInternal())
        backgroundValue = minValue;
      else
        backgroundValue = maxValue;
      break;
    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    Orthanc::ImageProcessing::Set(image, static_cast<int64_t>(backgroundValue));
  }

  bool RadiographyWidget::RenderInternal(unsigned int width,
                                         unsigned int height,
                                         ImageInterpolation interpolation)
  {
    if (floatBuffer_.get() == NULL ||
        floatBuffer_->GetWidth() != width ||
        floatBuffer_->GetHeight() != height)
    {
      floatBuffer_.reset(new Orthanc::Image(
        Orthanc::PixelFormat_Float32, width, height, false));

      if (floatBuffer_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory, "RadiographyWidget::RenderInternal: unable to allocate float buffer");
      }
    }

    if (cairoBuffer_.get() == NULL ||
        cairoBuffer_->GetWidth() != width ||
        cairoBuffer_->GetHeight() != height)
    {
      cairoBuffer_.reset(new CairoSurface(width, height, false /* no alpha */));

      if (cairoBuffer_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory, "RadiographyWidget::RenderInternal: unable to allocate cairo buffer");
      }
    }

    RenderBackground(*floatBuffer_, 0.0, 65535.0);

    scene_->Render(*floatBuffer_, GetView().GetMatrix(), interpolation, true);

    // Conversion from Float32 to BGRA32 (cairo). Very similar to
    // GrayscaleFrameRenderer => TODO MERGE?
    Orthanc::ImageAccessor target;
    cairoBuffer_->GetWriteableAccessor(target);

    bool invert = IsInvertedInternal();

    for (unsigned int y = 0; y < height; y++)
    {
      const float* p = reinterpret_cast<const float*>(floatBuffer_->GetConstRow(y));
      uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));

      for (unsigned int x = 0; x < width; x++, p++, q += 4)
      {
        uint8_t v = 0;
        if (*p >= 65535.0)
        {
          v = 255;
        }
        else if (*p <= 0.0)
        {
          v = 0;
        }
        else
        {
          v = static_cast<uint8_t>(*p / 256.0);
        }

        if (invert)
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


  bool RadiographyWidget::RenderScene(CairoContext& context,
                                      const Deprecated::ViewportGeometry& view)
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
      scene_->DrawBorder(
        context, static_cast<unsigned int>(selectedLayer_), view.GetZoom());
    }

    return true;
  }


  RadiographyWidget::RadiographyWidget(boost::shared_ptr<RadiographyScene> scene,
                                       const std::string& name) :
    WorldSceneWidget(name),
    invert_(false),
    interpolation_(ImageInterpolation_Nearest),
    hasSelection_(false),
    selectedLayer_(0)    // Dummy initialization
  {
    SetScene(scene);
  }


  void RadiographyWidget::Select(size_t layer)
  {
    hasSelection_ = true;
    selectedLayer_ = layer;

    NotifyContentChanged();
    BroadcastMessage(SelectionChangedMessage(*this));
  }

  void RadiographyWidget::Unselect()
  {
    hasSelection_ = false;

    NotifyContentChanged();
    BroadcastMessage(SelectionChangedMessage(*this));
  }

  bool RadiographyWidget::LookupSelectedLayer(size_t& layer) const
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
//    LOG(INFO) << "Scene geometry has changed";
    FitContent();
  }

  
  void RadiographyWidget::OnContentChanged(const RadiographyScene::ContentChangedMessage& message)
  {
//    LOG(INFO) << "Scene content has changed";
    NotifyContentChanged();
  }

  void RadiographyWidget::OnLayerRemoved(const RadiographyScene::LayerRemovedMessage& message)
  {
    size_t removedLayerIndex = message.GetLayerIndex();
    if (hasSelection_ && selectedLayer_ == removedLayerIndex)
    {
      Unselect();
    }
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

  void RadiographyWidget::SetScene(boost::shared_ptr<RadiographyScene> scene)
  {
    scene_ = scene;

    Register<RadiographyScene::GeometryChangedMessage>(*scene_, &RadiographyWidget::OnGeometryChanged);
    Register<RadiographyScene::ContentChangedMessage>(*scene_, &RadiographyWidget::OnContentChanged);
    Register<RadiographyScene::LayerRemovedMessage>(*scene_, &RadiographyWidget::OnLayerRemoved);

    Unselect();

    NotifyContentChanged();

    // force redraw
    FitContent();
  }
}