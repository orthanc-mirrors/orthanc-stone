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


#pragma once

#include "Internals/CompositorHelper.h"
#include "Internals/OpenGLColorTextureProgram.h"
#include "Internals/OpenGLFloatTextureProgram.h"
#include "Internals/OpenGLLinesProgram.h"
#include "Internals/OpenGLTextProgram.h"

namespace OrthancStone
{
  class OpenGLCompositor : private Internals::CompositorHelper::IRendererFactory
  {
  private:
    class Font;

    typedef std::map<size_t, Font*>  Fonts;

    OpenGL::IOpenGLContext&               context_;
    Fonts                                 fonts_;
    Internals::CompositorHelper           helper_;
    Internals::OpenGLColorTextureProgram  colorTextureProgram_;
    Internals::OpenGLFloatTextureProgram  floatTextureProgram_;
    Internals::OpenGLLinesProgram         linesProgram_;
    Internals::OpenGLTextProgram          textProgram_;
    unsigned int                          canvasWidth_;
    unsigned int                          canvasHeight_;
    
    const Font* GetFont(size_t fontIndex) const;

    virtual Internals::CompositorHelper::ILayerRenderer* Create(const ISceneLayer& layer);

  public:
    OpenGLCompositor(OpenGL::IOpenGLContext& context,
                     const Scene2D& scene);

    ~OpenGLCompositor();

    void UpdateSize();

    void Refresh();

    void SetFont(size_t index,
                 const GlyphBitmapAlphabet& dict);

#if ORTHANC_ENABLE_LOCALE == 1
    void SetFont(size_t index,
                 Orthanc::EmbeddedResources::FileResourceId resource,
                 unsigned int fontSize,
                 Orthanc::Encoding codepage);
#endif
  };
}