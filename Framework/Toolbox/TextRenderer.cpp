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


#include "TextRenderer.h"

#include "../Scene2D/CairoCompositor.h"
#include "../Scene2D/ColorTextureSceneLayer.h"
#include "../Scene2D/FloatTextureSceneLayer.h"
#include "../Scene2D/TextSceneLayer.h"
#include "../Fonts/GlyphBitmapAlphabet.h"
#include "../Fonts/FontRenderer.h"
#include <Core/Images/PngWriter.h>
#include <Core/Toolbox.h>

#include "Core/Images/Image.h"
#include "Core/Images/ImageProcessing.h"

namespace OrthancStone
{
  Orthanc::ImageAccessor* TextRenderer::Render(Orthanc::EmbeddedResources::FileResourceId font,
                                               unsigned int fontSize,
                                               const std::string& utf8String
                                               )
  {
    FontRenderer renderer;
    renderer.LoadFont(font, fontSize);

    // add each char to be rendered to the alphabet
    std::auto_ptr<GlyphBitmapAlphabet> alphabet(new GlyphBitmapAlphabet);

    size_t posInString = 0;
    uint32_t unicode;
    size_t utf8CharLength;

    while (posInString < utf8String.size())
    {
      Orthanc::Toolbox::Utf8ToUnicodeCharacter(unicode, utf8CharLength, utf8String, posInString);
      alphabet->AddUnicodeCharacter(renderer, unicode);
      posInString += utf8CharLength;
    }

    return alphabet->RenderText(utf8String);
  }


  Orthanc::ImageAccessor* TextRenderer::RenderWithAlpha(Orthanc::EmbeddedResources::FileResourceId resource,
                                                        unsigned int fontSize,
                                                        const std::string& utf8String,
                                                        uint8_t foreground)
  {
    std::auto_ptr<Orthanc::ImageAccessor> renderedText8(Render(resource, fontSize, utf8String));
    std::auto_ptr<Orthanc::Image> target(new Orthanc::Image(Orthanc::PixelFormat_RGBA32, renderedText8->GetWidth(), renderedText8->GetHeight(), true));

    Orthanc::ImageProcessing::Set(*target, foreground, foreground, foreground, *renderedText8);
    return target.release();
  }


  // currently disabled because the background is actually not transparent once we use the Cairo Compositor !
  //
  //  // renders text in color + a border with alpha in a RGBA32 image
  //  Orthanc::ImageAccessor* TextRenderer::RenderWithAlpha(Orthanc::EmbeddedResources::FileResourceId resource,
  //                                                        unsigned int fontSize,
  //                                                        const std::string& utf8String,
  //                                                        uint8_t foreground,
  //                                                        uint8_t borderColor)
  //  {
  //    std::auto_ptr<Orthanc::ImageAccessor> renderedBorderAlpha(RenderWithAlpha(resource, fontSize, utf8String, borderColor));
  //    std::auto_ptr<Orthanc::ImageAccessor> renderedTextAlpha(RenderWithAlpha(resource, fontSize, utf8String, foreground));

  //    unsigned int textWidth = renderedBorderAlpha->GetWidth();
  //    unsigned int textHeight = renderedBorderAlpha->GetHeight();

  //    Scene2D targetScene;
  //    std::auto_ptr<ColorTextureSceneLayer> borderLayerLeft(new ColorTextureSceneLayer(*renderedBorderAlpha));
  //    std::auto_ptr<ColorTextureSceneLayer> borderLayerRight(new ColorTextureSceneLayer(*renderedBorderAlpha));
  //    std::auto_ptr<ColorTextureSceneLayer> borderLayerTop(new ColorTextureSceneLayer(*renderedBorderAlpha));
  //    std::auto_ptr<ColorTextureSceneLayer> borderLayerBottom(new ColorTextureSceneLayer(*renderedBorderAlpha));
  //    std::auto_ptr<ColorTextureSceneLayer> textLayerCenter(new ColorTextureSceneLayer(*renderedTextAlpha));

  //    borderLayerLeft->SetOrigin(0, 1);
  //    borderLayerRight->SetOrigin(2, 1);
  //    borderLayerTop->SetOrigin(1, 0);
  //    borderLayerBottom->SetOrigin(1, 2);
  //    textLayerCenter->SetOrigin(1, 1);
  //    targetScene.SetLayer(1, borderLayerLeft.release());
  //    targetScene.SetLayer(2, borderLayerRight.release());
  //    targetScene.SetLayer(3, borderLayerTop.release());
  //    targetScene.SetLayer(4, borderLayerBottom.release());
  //    targetScene.SetLayer(5, textLayerCenter.release());

  //    targetScene.FitContent(textWidth + 2, textHeight + 2);
  //    CairoCompositor compositor(targetScene, textWidth + 2, textHeight + 2);
  //    compositor.Refresh();

  //    Orthanc::ImageAccessor canvas;
  //    compositor.GetCanvas().GetReadOnlyAccessor(canvas);

  //    std::auto_ptr<Orthanc::Image> output(new Orthanc::Image(Orthanc::PixelFormat_RGBA32, canvas.GetWidth(), canvas.GetHeight(), false));
  //    Orthanc::ImageProcessing::Convert(*output, canvas);
  //    return output.release();
  //  }
}