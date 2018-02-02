#pragma once

#include "Extent2D.h"
#include "OrientedBoundingBox.h"
#include "../Volumes/ImageBuffer3D.h"

namespace OrthancStone
{
  // Hypothesis: The output voxels always have square size
  class VolumeReslicer : public boost::noncopyable
  {
  private:
    // Input parameters
    Orthanc::PixelFormat           outputFormat_;
    bool                           hasLinearFunction_;
    float                          scaling_;  // "a" in "f(x) = a * x + b"
    float                          offset_;   // "b" in "f(x) = a * x + b"
    ImageInterpolation             interpolation_;
    bool                           fastMode_;

    // Output of reslicing
    bool                           success_;
    Extent2D                       extent_;
    std::auto_ptr<Orthanc::Image>  slice_;

    void CheckIterators(const ImageBuffer3D& source,
                        const CoordinateSystem3D& plane,
                        const OrientedBoundingBox& box) const;

    void Reset();

    float GetMinOutputValue() const;

    float GetMaxOutputValue() const;

    void SetWindow(float low,
                   float high);
    
  public:
    VolumeReslicer();

    void GetLinearFunction(float& scaling,
                           float& offset) const;

    void ResetLinearFunction();
    
    void SetLinearFunction(float scaling,
                           float offset);

    void FitRange(const ImageBuffer3D& image);

    void SetWindowing(ImageWindowing windowing,
                      const ImageBuffer3D& image,
                      float rescaleSlope,
                      float rescaleIntercept);

    Orthanc::PixelFormat GetOutputFormat() const
    {
      return outputFormat_;
    }

    void SetOutputFormat(Orthanc::PixelFormat format);

    ImageInterpolation GetInterpolation() const
    {
      return interpolation_;
    }

    void SetInterpolation(ImageInterpolation interpolation);

    bool IsFastMode() const
    {
      return fastMode_;
    }

    void EnableFastMode(bool enabled)
    {
      fastMode_ = enabled;
    }

    bool IsSuccess() const
    {
      return success_;
    }

    const Extent2D& GetOutputExtent() const;

    const Orthanc::ImageAccessor& GetOutputSlice() const;

    Orthanc::ImageAccessor* ReleaseOutputSlice();

    void Apply(const ImageBuffer3D& source,
               const CoordinateSystem3D& plane);

    void Apply(const ImageBuffer3D& source,
               const CoordinateSystem3D& plane,
               double voxelSize);
  };
}
