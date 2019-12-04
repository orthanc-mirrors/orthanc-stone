#pragma once

#include "Scene2D.h"
#include "ScenePoint2D.h"

#include <EmbeddedResources.h>

namespace OrthancStone
{
  class ICompositor : public boost::noncopyable
  {
  public:
    virtual ~ICompositor() 
    {
    }

    virtual unsigned int GetCanvasWidth() const = 0;

    virtual unsigned int GetCanvasHeight() const = 0;

    /**
     * WARNING: "Refresh()" must always be called with the same
     * scene. If the scene changes, a call to "ResetScene()" must be
     * done to reset the tracking of the revisions of the layers.
     **/
    virtual void Refresh(const Scene2D& scene) = 0;

    virtual void ResetScene() = 0;

#if ORTHANC_ENABLE_LOCALE == 1
    virtual void SetFont(size_t index,
                         Orthanc::EmbeddedResources::FileResourceId resource,
                         unsigned int fontSize,
                         Orthanc::Encoding codepage) = 0;
#endif

    // Get the center of the given pixel, in canvas coordinates
    ScenePoint2D GetPixelCenterCoordinates(int x, int y) const
    {
      return ScenePoint2D(
        static_cast<double>(x) + 0.5 - static_cast<double>(GetCanvasWidth()) / 2.0,
        static_cast<double>(y) + 0.5 - static_cast<double>(GetCanvasHeight()) / 2.0);
    }

    void FitContent(Scene2D& scene) const
    {
      scene.FitContent(GetCanvasWidth(), GetCanvasHeight());
    }
  };
}
