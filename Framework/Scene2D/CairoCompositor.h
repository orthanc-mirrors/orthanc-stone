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

#include "../Fonts/GlyphBitmapAlphabet.h"
#include "../Viewport/CairoContext.h"
#include "Internals/CompositorHelper.h"
#include "Internals/ICairoContextProvider.h"

namespace OrthancStone
{
  class CairoCompositor :
    private Internals::CompositorHelper::IRendererFactory,
    private Internals::ICairoContextProvider
  {
  private:
    typedef std::map<size_t, GlyphBitmapAlphabet*>   Fonts;

    Internals::CompositorHelper  helper_;
    CairoSurface                 canvas_;
    Fonts                        fonts_;

    // Only valid during a call to "Refresh()"
    std::auto_ptr<CairoContext>  context_;

    virtual cairo_t* GetCairoContext();

    virtual unsigned int GetCairoWidth()
    {
      return canvas_.GetWidth();
    }

    virtual unsigned int GetCairoHeight()
    {
      return canvas_.GetHeight();
    }
    
    virtual Internals::CompositorHelper::ILayerRenderer* Create(const ISceneLayer& layer);

  public:
    CairoCompositor(const Scene2D& scene,
                    unsigned int canvasWidth,
                    unsigned int canvasHeight);
    
    ~CairoCompositor();

    const CairoSurface& GetCanvas() const
    {
      return canvas_;
    }
    
    void SetFont(size_t index,
                 GlyphBitmapAlphabet* dict);  // Takes ownership

#if ORTHANC_ENABLE_LOCALE == 1
    void SetFont(size_t index,
                 Orthanc::EmbeddedResources::FileResourceId resource,
                 unsigned int fontSize,
                 Orthanc::Encoding codepage);
#endif

    void Refresh();

    Orthanc::ImageAccessor* RenderText(size_t fontIndex,
                                       const std::string& utf8) const;
  };
}