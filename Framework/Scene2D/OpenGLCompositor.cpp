/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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


#include "OpenGLCompositor.h"

#include "Internals/OpenGLAdvancedPolylineRenderer.h"
#include "Internals/OpenGLBasicPolylineRenderer.h"
#include "Internals/OpenGLColorTextureRenderer.h"
#include "Internals/OpenGLFloatTextureRenderer.h"
#include "Internals/OpenGLInfoPanelRenderer.h"
#include "Internals/OpenGLTextRenderer.h"

namespace OrthancStone
{
  class OpenGLCompositor::Font : public boost::noncopyable
  {
  private:
    std::auto_ptr<GlyphTextureAlphabet>   alphabet_;
    std::auto_ptr<OpenGL::OpenGLTexture>  texture_;

  public:
    Font(const GlyphBitmapAlphabet& dict)
    {
      alphabet_.reset(new GlyphTextureAlphabet(dict));
      texture_.reset(new OpenGL::OpenGLTexture);

      std::auto_ptr<Orthanc::ImageAccessor> bitmap(alphabet_->ReleaseTexture());
      texture_->Load(*bitmap, true /* enable linear interpolation */);
    }

    OpenGL::OpenGLTexture& GetTexture() const
    {
      assert(texture_.get() != NULL);
      return *texture_;
    }

    const GlyphTextureAlphabet& GetAlphabet() const
    {
      assert(alphabet_.get() != NULL);
      return *alphabet_;
    }
  };


  const OpenGLCompositor::Font* OpenGLCompositor::GetFont(size_t fontIndex) const
  {
    Fonts::const_iterator found = fonts_.find(fontIndex);

    if (found == fonts_.end())
    {
      return NULL;  // Unknown font, nothing should be drawn
    }
    else
    {
      assert(found->second != NULL);
      return found->second;
    }
  }


  Internals::CompositorHelper::ILayerRenderer* OpenGLCompositor::Create(const ISceneLayer& layer)
  {
    switch (layer.GetType())
    {
      case ISceneLayer::Type_InfoPanel:
        return new Internals::OpenGLInfoPanelRenderer
          (context_, colorTextureProgram_, dynamic_cast<const InfoPanelSceneLayer&>(layer));

      case ISceneLayer::Type_ColorTexture:
        return new Internals::OpenGLColorTextureRenderer
          (context_, colorTextureProgram_, dynamic_cast<const ColorTextureSceneLayer&>(layer));

      case ISceneLayer::Type_FloatTexture:
        return new Internals::OpenGLFloatTextureRenderer
          (context_, floatTextureProgram_, dynamic_cast<const FloatTextureSceneLayer&>(layer));

      case ISceneLayer::Type_Polyline:
        return new Internals::OpenGLAdvancedPolylineRenderer
          (context_, linesProgram_, dynamic_cast<const PolylineSceneLayer&>(layer));
        //return new Internals::OpenGLBasicPolylineRenderer(context_, dynamic_cast<const PolylineSceneLayer&>(layer));

      case ISceneLayer::Type_Text:
      {
        const TextSceneLayer& l = dynamic_cast<const TextSceneLayer&>(layer);
        const Font* font = GetFont(l.GetFontIndex());
        if (font == NULL)
        {
          return NULL;
        }
        else
        {
          return new Internals::OpenGLTextRenderer
            (context_, textProgram_, font->GetAlphabet(), font->GetTexture(), l);
        }
      }

      default:
        return NULL;
    }
  }


  OpenGLCompositor::OpenGLCompositor(OpenGL::IOpenGLContext& context,
                                     const Scene2D& scene) :
    context_(context),
    helper_(scene, *this),
    colorTextureProgram_(context),
    floatTextureProgram_(context),
    linesProgram_(context),
    textProgram_(context),
    canvasWidth_(0),
    canvasHeight_(0)
  {
    UpdateSize();
  }

  
  OpenGLCompositor::~OpenGLCompositor()
  {
    for (Fonts::iterator it = fonts_.begin(); it != fonts_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }

  
  void OpenGLCompositor::UpdateSize()
  {
    canvasWidth_ = context_.GetCanvasWidth();
    canvasHeight_ = context_.GetCanvasHeight();

    context_.MakeCurrent();
    glViewport(0, 0, canvasWidth_, canvasHeight_);
  }

  
  void OpenGLCompositor::Refresh()
  {
    context_.MakeCurrent();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    helper_.Refresh(canvasWidth_, canvasHeight_);

    context_.SwapBuffer();
  }

  
  void OpenGLCompositor::SetFont(size_t index,
                                 const GlyphBitmapAlphabet& dict)
  {
    std::auto_ptr<Font> font(new Font(dict));
      
    Fonts::iterator found = fonts_.find(index);

    if (found == fonts_.end())
    {
      fonts_[index] = font.release();
    }
    else
    {
      assert(found->second != NULL);
      delete found->second;

      found->second = font.release();
    }
  }
  

#if ORTHANC_ENABLE_LOCALE == 1
  void OpenGLCompositor::SetFont(size_t index,
                                 Orthanc::EmbeddedResources::FileResourceId resource,
                                 unsigned int fontSize,
                                 Orthanc::Encoding codepage)
  {
    FontRenderer renderer;
    renderer.LoadFont(resource, fontSize);

    GlyphBitmapAlphabet dict;
    dict.LoadCodepage(renderer, codepage);

    SetFont(index, dict);
  }
#endif
}