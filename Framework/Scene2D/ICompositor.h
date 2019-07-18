#pragma once

#include <boost/noncopyable.hpp>
#include <EmbeddedResources.h>
#include <Core/Enumerations.h>

namespace OrthancStone
{
  class ICompositor : public boost::noncopyable
  {

  public:
    virtual ~ICompositor() {}

    virtual unsigned int GetCanvasWidth() const = 0;

    virtual unsigned int GetCanvasHeight() const = 0;

    virtual void Refresh() = 0;

#if ORTHANC_ENABLE_LOCALE == 1
    virtual void SetFont(size_t index,
                         Orthanc::EmbeddedResources::FileResourceId resource,
                         unsigned int fontSize,
                         Orthanc::Encoding codepage) = 0;
#endif

  };
}
