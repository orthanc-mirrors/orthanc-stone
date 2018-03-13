#include "VolumeReslicer.h"

#include "../Toolbox/GeometryToolbox.h"

#include <Core/Images/ImageTraits.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/math/special_functions/round.hpp>

#if defined(_MSC_VER)
#  define ORTHANC_STONE_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__) || defined(__EMSCRIPTEN__)
#  define ORTHANC_STONE_FORCE_INLINE inline __attribute((always_inline))
#else
#  error Please support your compiler here
#endif


namespace OrthancStone
{
  // Anonymous namespace to avoid clashes between compilation modules
  namespace
  {
    enum TransferFunction
    {
      TransferFunction_Copy,
      TransferFunction_Float,
      TransferFunction_Linear
    };

    template <Orthanc::PixelFormat InputFormat,
              Orthanc::PixelFormat OutputFormat>
    class PixelWriter
    {
    public:
      typedef typename Orthanc::PixelTraits<InputFormat>::PixelType   InputPixelType;
      typedef Orthanc::PixelTraits<OutputFormat>                      OutputPixelTraits;
      typedef typename Orthanc::PixelTraits<OutputFormat>::PixelType  OutputPixelType;

    private:
      template <typename T>
      static void SetValueInternal(OutputPixelType* pixel,
                                   const T& value)
      {
        if (value < std::numeric_limits<OutputPixelType>::min())
        {
          *pixel = std::numeric_limits<OutputPixelType>::min();
        }
        else if (value > std::numeric_limits<OutputPixelType>::max())
        {
          *pixel = std::numeric_limits<OutputPixelType>::max();
        }
        else
        {
          *pixel = static_cast<OutputPixelType>(value);
        }
      }

    public:
      ORTHANC_STONE_FORCE_INLINE
      void SetFloatValue(OutputPixelType* pixel,
                         float value) const
      {
        SetValueInternal<float>(pixel, value);
      }
      
      ORTHANC_STONE_FORCE_INLINE
      void SetValue(OutputPixelType* pixel,
                    const InputPixelType& value) const
      {
        SetValueInternal<InputPixelType>(pixel, value);
      }
    };


    template <Orthanc::PixelFormat InputFormat>
    class PixelWriter<InputFormat, Orthanc::PixelFormat_BGRA32>
    {
    public:
      typedef typename Orthanc::PixelTraits<InputFormat>::PixelType         InputPixelType;
      typedef Orthanc::PixelTraits<Orthanc::PixelFormat_BGRA32>             OutputPixelTraits;
      typedef Orthanc::PixelTraits<Orthanc::PixelFormat_BGRA32>::PixelType  OutputPixelType;

    private:
      template <typename T>
      static void SetValueInternal(OutputPixelType* pixel,
                                   const T& value)
      {
        uint8_t v;
        if (value < 0)
        {
          v = 0;
        }
        else if (value >= 255.0f)
        {
          v = 255;
        }
        else
        {
          v = static_cast<uint8_t>(value);
        }

        pixel->blue_ = v;
        pixel->green_ = v;
        pixel->red_ = v;
        pixel->alpha_ = 255;
      }

    public:
      ORTHANC_STONE_FORCE_INLINE
      void SetFloatValue(OutputPixelType* pixel,
                         float value) const
      {
        SetValueInternal<float>(pixel, value);
      }

      ORTHANC_STONE_FORCE_INLINE
      void SetValue(OutputPixelType* pixel,
                    const InputPixelType& value) const
      {
        SetValueInternal<InputPixelType>(pixel, value);
      }
    };



    class VoxelReaderBase : public boost::noncopyable
    {
    private:
      const Orthanc::ImageAccessor&  image_;
      unsigned int                   width_;
      unsigned int                   height_;
      unsigned int                   depth_;
      float                          widthFloat_;
      float                          heightFloat_;
      float                          depthFloat_;

    public:
      VoxelReaderBase(const ImageBuffer3D& image) :
        image_(image.GetInternalImage()),
        width_(image.GetWidth()),
        height_(image.GetHeight()),
        depth_(image.GetDepth()),
        widthFloat_(static_cast<float>(image.GetWidth())),
        heightFloat_(static_cast<float>(image.GetHeight())),
        depthFloat_(static_cast<float>(image.GetDepth()))
      {
      }

      const Orthanc::ImageAccessor& GetImage() const
      {
        return image_;
      }

      const unsigned int GetImageWidth() const
      {
        return width_;
      }

      const unsigned int GetImageHeight() const
      {
        return height_;
      }

      const unsigned int GetImageDepth() const
      {
        return depth_;
      }

      bool GetNearestCoordinates(unsigned int& imageX,
                                 unsigned int& imageY,
                                 unsigned int& imageZ,
                                 float& fractionalX,
                                 float& fractionalY,
                                 float& fractionalZ,
                                 float volumeX,
                                 float volumeY,
                                 float volumeZ) const
      {
        if (volumeX >= 0 &&
            volumeY >= 0 &&
            volumeZ >= 0)
        {
          const float x = volumeX * widthFloat_;
          const float y = volumeY * heightFloat_;
          const float z = volumeZ * depthFloat_;
          
          imageX = static_cast<unsigned int>(std::floor(x));
          imageY = static_cast<unsigned int>(std::floor(y));
          imageZ = static_cast<unsigned int>(std::floor(z));

          if (imageX < width_ &&
              imageY < height_ &&
              imageZ < depth_)
          {
            fractionalX = x - static_cast<float>(imageX);
            fractionalY = y - static_cast<float>(imageY);
            fractionalZ = z - static_cast<float>(imageZ);
            return true;
          }
          else
          {
            return false;
          }
        }
        else
        {
          return false;
        }
      }
    };


    template <Orthanc::PixelFormat InputFormat,
              ImageInterpolation Interpolation>
    class VoxelReader;

    
    template <Orthanc::PixelFormat InputFormat>
    class VoxelReader<InputFormat, ImageInterpolation_Nearest> :
      public VoxelReaderBase
    {
    public:
      typedef typename Orthanc::PixelTraits<InputFormat>::PixelType   InputPixelType;

      VoxelReader(const ImageBuffer3D& image) :
        VoxelReaderBase(image)
      {
      }

      ORTHANC_STONE_FORCE_INLINE
      float GetFloatValue(float volumeX,
                          float volumeY,
                          float volumeZ) const
      {
        InputPixelType value;
        GetValue(value, volumeX, volumeY, volumeZ);
        return static_cast<float>(value);
      }

      ORTHANC_STONE_FORCE_INLINE
      void GetValue(InputPixelType& target,
                    float volumeX,
                    float volumeY,
                    float volumeZ) const
      {
        unsigned int imageX, imageY, imageZ;
        float fractionalX, fractionalY, fractionalZ;  // unused

        if (GetNearestCoordinates(imageX, imageY, imageZ,
                                  fractionalX, fractionalY, fractionalZ,
                                  volumeX, volumeY, volumeZ))
        {
          Orthanc::ImageTraits<InputFormat>::GetPixel(target, GetImage(), imageX, 
                                                      imageY + imageZ * GetImageHeight());
        }
        else
        {
          target = std::numeric_limits<InputPixelType>::min();
        }
      }
    };
    
    
    template <Orthanc::PixelFormat InputFormat>
    class VoxelReader<InputFormat, ImageInterpolation_Bilinear> :
      public VoxelReaderBase
    {
    private:
      float outOfVolume_;
      
    public:
      VoxelReader(const ImageBuffer3D& image) :
        VoxelReaderBase(image)
      {
        typedef typename Orthanc::PixelTraits<InputFormat>::PixelType Pixel;
        outOfVolume_ = static_cast<float>(std::numeric_limits<Pixel>::min());
      }

      void SampleVoxels(float& f00,
                        float& f01,
                        float& f10,
                        float& f11,
                        unsigned int imageX,
                        unsigned int imageY,
                        unsigned int imageZ) const
      {
        f00 = Orthanc::ImageTraits<InputFormat>::GetFloatPixel
          (GetImage(), imageX, imageY + imageZ * GetImageHeight());

        if (imageX + 1 < GetImageWidth())
        {
          f01 = Orthanc::ImageTraits<InputFormat>::GetFloatPixel
            (GetImage(), imageX + 1, imageY + imageZ * GetImageHeight());
        }
        else
        {
          f01 = f00;
        }

        if (imageY + 1 < GetImageWidth())
        {
          f10 = Orthanc::ImageTraits<InputFormat>::GetFloatPixel
            (GetImage(), imageX, imageY + 1 + imageZ * GetImageHeight());
        }
        else
        {
          f10 = f00;
        }

        if (imageX + 1 < GetImageWidth() &&
            imageY + 1 < GetImageHeight())
        {
          f11 = Orthanc::ImageTraits<InputFormat>::GetFloatPixel
            (GetImage(), imageX + 1, imageY + 1 + imageZ * GetImageHeight());
        }
        else
        {
          f11 = f00;
        }
      }

      float GetOutOfVolume() const
      {
        return outOfVolume_;
      }
      
      float GetFloatValue(float volumeX,
                          float volumeY,
                          float volumeZ) const
      {
        unsigned int imageX, imageY, imageZ;
        float fractionalX, fractionalY, fractionalZ;

        if (GetNearestCoordinates(imageX, imageY, imageZ,
                                  fractionalX, fractionalY, fractionalZ,
                                  volumeX, volumeY, volumeZ))
        {
          float f00, f01, f10, f11;
          SampleVoxels(f00, f01, f10, f11, imageX, imageY, imageZ);
          return GeometryToolbox::ComputeBilinearInterpolationUnitSquare
            (fractionalX, fractionalY, f00, f01, f10, f11);
        }
        else
        {
          return outOfVolume_;
        }
      }
    };


    template <Orthanc::PixelFormat InputFormat>
    class VoxelReader<InputFormat, ImageInterpolation_Trilinear> :
      public VoxelReaderBase
    {
    private:
      typedef VoxelReader<InputFormat, ImageInterpolation_Bilinear>  Bilinear;

      Bilinear      bilinear_;
      unsigned int  imageDepth_;

    public:
      VoxelReader(const ImageBuffer3D& image) :
        VoxelReaderBase(image),
        bilinear_(image),
        imageDepth_(image.GetDepth())
      {
      }

      float GetFloatValue(float volumeX,
                          float volumeY,
                          float volumeZ) const
      {
        unsigned int imageX, imageY, imageZ;
        float fractionalX, fractionalY, fractionalZ;

        if (GetNearestCoordinates(imageX, imageY, imageZ,
                                  fractionalX, fractionalY, fractionalZ,
                                  volumeX, volumeY, volumeZ))
        {
          float f000, f001, f010, f011;
          bilinear_.SampleVoxels(f000, f001, f010, f011, imageX, imageY, imageZ);

          if (imageZ + 1 < imageDepth_)
          {
            float f100, f101, f110, f111;
            bilinear_.SampleVoxels(f100, f101, f110, f111, imageX, imageY, imageZ + 1);
            return GeometryToolbox::ComputeTrilinearInterpolationUnitSquare
              (fractionalX, fractionalY, fractionalZ,
               f000, f001, f010, f011, f100, f101, f110, f111);
          }
          else
          {
            return GeometryToolbox::ComputeBilinearInterpolationUnitSquare
              (fractionalX, fractionalY, f000, f001, f010, f011);
          }
        }
        else
        {
          return bilinear_.GetOutOfVolume();
        }
      }
    };


    template <typename VoxelReader,
              typename PixelWriter,
              TransferFunction Function>
    class PixelShader;

    
    template <typename VoxelReader,
              typename PixelWriter>
    class PixelShader<VoxelReader, PixelWriter, TransferFunction_Copy>
    {
    private:
      VoxelReader  reader_;
      PixelWriter  writer_;
      
    public:
      PixelShader(const ImageBuffer3D& image,
                  float /* scaling */,
                  float /* offset */) :
        reader_(image)
      {
      }
      
      ORTHANC_STONE_FORCE_INLINE
      void Apply(typename PixelWriter::OutputPixelType* pixel,
                 float volumeX,
                 float volumeY,
                 float volumeZ)
      {
        typename VoxelReader::InputPixelType image;
        reader_.GetValue(image, volumeX, volumeY, volumeZ);
        writer_.SetValue(pixel, image);
      }        
    };

    
    template <typename VoxelReader,
              typename PixelWriter>
    class PixelShader<VoxelReader, PixelWriter, TransferFunction_Float>
    {
    private:
      VoxelReader  reader_;
      PixelWriter  writer_;
      
    public:
      PixelShader(const ImageBuffer3D& image,
                  float /* scaling */,
                  float /* offset */) :
        reader_(image)
      {
      }
      
      ORTHANC_STONE_FORCE_INLINE
      void Apply(typename PixelWriter::OutputPixelType* pixel,
                 float volumeX,
                 float volumeY,
                 float volumeZ)
      {
        writer_.SetFloatValue(pixel, reader_.GetFloatValue(volumeX, volumeY, volumeZ));
      }        
    };

    
    template <typename VoxelReader,
              typename PixelWriter>
    class PixelShader<VoxelReader, PixelWriter, TransferFunction_Linear>
    {
    private:
      VoxelReader  reader_;
      PixelWriter  writer_;
      float        scaling_;
      float        offset_;
      
    public:
      PixelShader(const ImageBuffer3D& image,
                  float scaling,
                  float offset) :
        reader_(image),
        scaling_(scaling),
        offset_(offset)
      {
      }
      
      ORTHANC_STONE_FORCE_INLINE
      void Apply(typename PixelWriter::OutputPixelType* pixel,
                 float volumeX,
                 float volumeY,
                 float volumeZ)
      {
        writer_.SetFloatValue(pixel, scaling_ * reader_.GetFloatValue(volumeX, volumeY, volumeZ) + offset_);
      }        
    };



    class FastRowIterator : public boost::noncopyable
    {
    private:
      float  position_[3];
      float  offset_[3];
      
    public:
      FastRowIterator(const Orthanc::ImageAccessor& slice,
                      const Extent2D& extent,
                      const CoordinateSystem3D& plane,
                      const OrientedBoundingBox& box,
                      unsigned int y)
      {
        const double width = static_cast<double>(slice.GetWidth());
        const double height = static_cast<double>(slice.GetHeight());
        assert(y < height);

        Vector q1 = plane.MapSliceToWorldCoordinates
          (extent.GetX1() + extent.GetWidth() * static_cast<double>(0) / static_cast<double>(width + 1),
           extent.GetY1() + extent.GetHeight() * static_cast<double>(y) / static_cast<double>(height + 1));

        Vector q2 = plane.MapSliceToWorldCoordinates
          (extent.GetX1() + extent.GetWidth() * static_cast<double>(width - 1) / static_cast<double>(width + 1),
           extent.GetY1() + extent.GetHeight() * static_cast<double>(y) / static_cast<double>(height + 1));

        Vector r1, r2;
        box.ToInternalCoordinates(r1, q1);
        box.ToInternalCoordinates(r2, q2);

        position_[0] = static_cast<float>(r1[0]);
        position_[1] = static_cast<float>(r1[1]);
        position_[2] = static_cast<float>(r1[2]);
        
        Vector tmp = (r2 - r1) / static_cast<double>(width - 1);
        offset_[0] = static_cast<float>(tmp[0]);
        offset_[1] = static_cast<float>(tmp[1]);
        offset_[2] = static_cast<float>(tmp[2]);
      }

      ORTHANC_STONE_FORCE_INLINE
      void Next()
      {
        position_[0] += offset_[0];
        position_[1] += offset_[1];
        position_[2] += offset_[2];
      }

      ORTHANC_STONE_FORCE_INLINE
      void GetVolumeCoordinates(float& x,
                                float& y,
                                float& z) const
      {
        x = position_[0];
        y = position_[1];
        z = position_[2];
      }
    };


    class SlowRowIterator : public boost::noncopyable
    {
    private:
      const Orthanc::ImageAccessor&  slice_;
      const Extent2D&                extent_;
      const CoordinateSystem3D&      plane_;
      const OrientedBoundingBox&     box_;
      unsigned int                   x_;
      unsigned int                   y_;
      
    public:
      SlowRowIterator(const Orthanc::ImageAccessor& slice,
                      const Extent2D& extent,
                      const CoordinateSystem3D& plane,
                      const OrientedBoundingBox& box,
                      unsigned int y) :
        slice_(slice),
        extent_(extent),
        plane_(plane),
        box_(box),
        x_(0),
        y_(y)
      {
        assert(y_ < slice_.GetHeight());
      }

      void Next()
      {
        x_++;
      }

      void GetVolumeCoordinates(float& x,
                                float& y,
                                float& z) const
      {
        assert(x_ < slice_.GetWidth());
        
        const double width = static_cast<double>(slice_.GetWidth());
        const double height = static_cast<double>(slice_.GetHeight());
        
        Vector q = plane_.MapSliceToWorldCoordinates
          (extent_.GetX1() + extent_.GetWidth() * static_cast<double>(x_) / (width + 1.0),
           extent_.GetY1() + extent_.GetHeight() * static_cast<double>(y_) / (height + 1.0));

        Vector r;
        box_.ToInternalCoordinates(r, q);

        x = static_cast<float>(r[0]);
        y = static_cast<float>(r[1]);
        z = static_cast<float>(r[2]);
      }
    };


    template <typename RowIterator,
              Orthanc::PixelFormat InputFormat,
              Orthanc::PixelFormat OutputFormat,
              ImageInterpolation Interpolation,
              TransferFunction Function>
    static void ProcessImage(Orthanc::ImageAccessor& slice,
                             const Extent2D& extent,
                             const ImageBuffer3D& source,
                             const CoordinateSystem3D& plane,
                             const OrientedBoundingBox& box,
                             float scaling,
                             float offset)
    {
      typedef VoxelReader<InputFormat, Interpolation>   Reader;
      typedef PixelWriter<InputFormat, OutputFormat>    Writer;
      typedef PixelShader<Reader, Writer, Function>     Shader;

      const unsigned int outputWidth = slice.GetWidth();
      const unsigned int outputHeight = slice.GetHeight();

      Shader shader(source, scaling, offset);

      for (unsigned int y = 0; y < outputHeight; y++)
      {
        typename Writer::OutputPixelType* p = 
          reinterpret_cast<typename Writer::OutputPixelType*>(slice.GetRow(y));

        RowIterator it(slice, extent, plane, box, y);

        for (unsigned int x = 0; x < outputWidth; x++, p++)
        {
          float volumeX, volumeY, volumeZ;
          it.GetVolumeCoordinates(volumeX, volumeY, volumeZ);
          shader.Apply(p, volumeX, volumeY, volumeZ);
          it.Next();
        }
      }
    }


    template <typename RowIterator,
              Orthanc::PixelFormat InputFormat,
              Orthanc::PixelFormat OutputFormat>
    static void ProcessImage(Orthanc::ImageAccessor& slice,
                             const Extent2D& extent,
                             const ImageBuffer3D& source,
                             const CoordinateSystem3D& plane,
                             const OrientedBoundingBox& box,
                             ImageInterpolation interpolation,
                             bool hasLinearFunction,
                             float scaling,
                             float offset)
    {
      if (hasLinearFunction)
      {
        switch (interpolation)
        {
          case ImageInterpolation_Nearest:
            ProcessImage<RowIterator, InputFormat, OutputFormat,
                         ImageInterpolation_Nearest, TransferFunction_Linear>
              (slice, extent, source, plane, box, scaling, offset);
            break;

          case ImageInterpolation_Bilinear:
            ProcessImage<RowIterator, InputFormat, OutputFormat,
                         ImageInterpolation_Bilinear, TransferFunction_Linear>
              (slice, extent, source, plane, box, scaling, offset);
            break;

          case ImageInterpolation_Trilinear:
            ProcessImage<RowIterator, InputFormat, OutputFormat,
                         ImageInterpolation_Trilinear, TransferFunction_Linear>
              (slice, extent, source, plane, box, scaling, offset);
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }        
      }
      else
      {
        switch (interpolation)
        {
          case ImageInterpolation_Nearest:
            ProcessImage<RowIterator, InputFormat, OutputFormat,
                         ImageInterpolation_Nearest, TransferFunction_Copy>
              (slice, extent, source, plane, box, 0, 0);
            break;

          case ImageInterpolation_Bilinear:
            ProcessImage<RowIterator, InputFormat, OutputFormat,
                         ImageInterpolation_Bilinear, TransferFunction_Float>
              (slice, extent, source, plane, box, 0, 0);
            break;

          case ImageInterpolation_Trilinear:
            ProcessImage<RowIterator, InputFormat, OutputFormat,
                         ImageInterpolation_Trilinear, TransferFunction_Float>
              (slice, extent, source, plane, box, 0, 0);
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }        
      }
    }
    
    
    template <typename RowIterator>
    static void ProcessImage(Orthanc::ImageAccessor& slice,
                             const Extent2D& extent,
                             const ImageBuffer3D& source,
                             const CoordinateSystem3D& plane,
                             const OrientedBoundingBox& box,
                             ImageInterpolation interpolation,
                             bool hasLinearFunction,
                             float scaling,
                             float offset)
    {
      if (source.GetFormat() == Orthanc::PixelFormat_Grayscale16 &&
          slice.GetFormat() == Orthanc::PixelFormat_Grayscale8)
      {
        ProcessImage<RowIterator,
                     Orthanc::PixelFormat_Grayscale16,
                     Orthanc::PixelFormat_Grayscale8>
          (slice, extent, source, plane, box, interpolation, hasLinearFunction, scaling, offset);
      }
      else if (source.GetFormat() == Orthanc::PixelFormat_Grayscale16 &&
               slice.GetFormat() == Orthanc::PixelFormat_Grayscale16)
      {
        ProcessImage<RowIterator,
                     Orthanc::PixelFormat_Grayscale16,
                     Orthanc::PixelFormat_Grayscale16>
          (slice, extent, source, plane, box, interpolation, hasLinearFunction, scaling, offset);
      }
      else if (source.GetFormat() == Orthanc::PixelFormat_SignedGrayscale16 &&
               slice.GetFormat() == Orthanc::PixelFormat_BGRA32)
      {
        ProcessImage<RowIterator,
                     Orthanc::PixelFormat_SignedGrayscale16,
                     Orthanc::PixelFormat_BGRA32>
          (slice, extent, source, plane, box, interpolation, hasLinearFunction, scaling, offset);
      }
      else if (source.GetFormat() == Orthanc::PixelFormat_Grayscale16 &&
               slice.GetFormat() == Orthanc::PixelFormat_BGRA32)
      {
        ProcessImage<RowIterator,
                     Orthanc::PixelFormat_Grayscale16,
                     Orthanc::PixelFormat_BGRA32>
          (slice, extent, source, plane, box, interpolation, hasLinearFunction, scaling, offset);
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }
  }
    
    

  void VolumeReslicer::CheckIterators(const ImageBuffer3D& source,
                                      const CoordinateSystem3D& plane,
                                      const OrientedBoundingBox& box) const
  {
    for (unsigned int y = 0; y < slice_->GetHeight(); y++)
    {
      FastRowIterator fast(*slice_, extent_, plane, box, y);
      SlowRowIterator slow(*slice_, extent_, plane, box, y);

      for (unsigned int x = 0; x < slice_->GetWidth(); x++)
      {
        float px, py, pz;
        fast.GetVolumeCoordinates(px, py, pz);

        float qx, qy, qz;
        slow.GetVolumeCoordinates(qx, qy, qz);

        Vector d;
        LinearAlgebra::AssignVector(d, px - qx, py - qy, pz - qz);
        double norm = boost::numeric::ublas::norm_2(d);
        if (norm > 0.0001)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
          
        fast.Next();
        slow.Next();
      }
    }
  }

    
  void VolumeReslicer::Reset()
  {
    success_ = false;
    extent_.Reset();
    slice_.reset(NULL);
  }


  float VolumeReslicer::GetMinOutputValue() const
  {
    switch (outputFormat_)
    {
      case Orthanc::PixelFormat_Grayscale8:
      case Orthanc::PixelFormat_Grayscale16:
      case Orthanc::PixelFormat_BGRA32:
        return 0.0f;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }

    
  float VolumeReslicer::GetMaxOutputValue() const
  {
    switch (outputFormat_)
    {
      case Orthanc::PixelFormat_Grayscale8:
      case Orthanc::PixelFormat_BGRA32:
        return static_cast<float>(std::numeric_limits<uint8_t>::max());
        break;

      case Orthanc::PixelFormat_Grayscale16:
        return static_cast<float>(std::numeric_limits<uint16_t>::max());
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }
    

  VolumeReslicer::VolumeReslicer() :
    outputFormat_(Orthanc::PixelFormat_Grayscale8),
    interpolation_(ImageInterpolation_Nearest),
    fastMode_(true),
    success_(false)
  {
    ResetLinearFunction();
  }


  void VolumeReslicer::GetLinearFunction(float& scaling,
                                         float& offset) const
  {
    if (hasLinearFunction_)
    {
      scaling = scaling_;
      offset = offset_;
    }
    else
    {
      scaling = 1.0f;
      offset = 0.0f;
    }
  }

  
  void VolumeReslicer::ResetLinearFunction()
  {
    Reset();
    hasLinearFunction_ = false;
    scaling_ = 1.0f;
    offset_ = 0.0f;
  }
    

  void VolumeReslicer::SetLinearFunction(float scaling,
                                         float offset)
  {
    Reset();
    hasLinearFunction_ = true;
    scaling_ = scaling;
    offset_ = offset;
  }

  
  void VolumeReslicer::SetWindow(float low,
                                 float high)
  {
    //printf("Range in pixel values: %f->%f\n", low, high);
    float scaling = (GetMaxOutputValue() - GetMinOutputValue()) / (high - low);
    float offset = GetMinOutputValue() - scaling * low;
    
    SetLinearFunction(scaling, offset);

    /*float x = scaling_ * low + offset_;
    float y = scaling_ * high + offset_;
    printf("%f %f (should be %f->%f)\n", x, y, GetMinOutputValue(), GetMaxOutputValue());*/
  }
  

  void VolumeReslicer::FitRange(const ImageBuffer3D& image)
  {
    float minInputValue, maxInputValue;

    if (!image.GetRange(minInputValue, maxInputValue) ||
        maxInputValue < 1)
    {
      ResetLinearFunction();
    }
    else
    {
      SetWindow(minInputValue, maxInputValue);
    }
  }  

  
  void VolumeReslicer::SetWindowing(ImageWindowing windowing,
                                    const ImageBuffer3D& image,
                                    float rescaleSlope,
                                    float rescaleIntercept)
  {
    if (windowing == ImageWindowing_Custom ||
        windowing == ImageWindowing_Default)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    float center, width;
    ComputeWindowing(center, width, windowing, 0, 0);

    float a = (center - width / 2.0f - rescaleIntercept) / rescaleSlope;
    float b = (center + width / 2.0f - rescaleIntercept) / rescaleSlope;
    SetWindow(a, b);
  }


  void VolumeReslicer::SetOutputFormat(Orthanc::PixelFormat format)
  {
    if (format != Orthanc::PixelFormat_Grayscale8 &&
        format != Orthanc::PixelFormat_Grayscale16 &&
        format != Orthanc::PixelFormat_BGRA32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (hasLinearFunction_)
    {
      LOG(WARNING) << "Calls to VolumeReslicer::SetOutputFormat() should be done before VolumeReslicer::FitRange()";
    }
    
    outputFormat_ = format;
    Reset();
  }


  void VolumeReslicer::SetInterpolation(ImageInterpolation interpolation)
  {
    if (interpolation != ImageInterpolation_Nearest &&
        interpolation != ImageInterpolation_Bilinear &&
        interpolation != ImageInterpolation_Trilinear)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    interpolation_ = interpolation;
    Reset();
  }


  const Extent2D& VolumeReslicer::GetOutputExtent() const
  {
    if (success_)
    {
      return extent_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  
  const Orthanc::ImageAccessor& VolumeReslicer::GetOutputSlice() const
  {
    if (success_)
    {
      assert(slice_.get() != NULL);
      return *slice_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  
  Orthanc::ImageAccessor* VolumeReslicer::ReleaseOutputSlice()
  {
    if (success_)
    {
      assert(slice_.get() != NULL);
      success_ = false;
      return slice_.release();
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  
  void VolumeReslicer::Apply(const ImageBuffer3D& source,
                             const CoordinateSystem3D& plane)
  {
    // Choose the default voxel size as the finest voxel dimension
    // of the source volumetric image
    const OrthancStone::Vector dim = source.GetVoxelDimensions(OrthancStone::VolumeProjection_Axial);
    double voxelSize = dim[0];
    
    if (dim[1] < voxelSize)
    {
      voxelSize = dim[1];
    }
    
    if (dim[2] < voxelSize)
    {
      voxelSize = dim[2];
    }

    if (voxelSize <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    Apply(source, plane, voxelSize);
  }

  
  void VolumeReslicer::Apply(const ImageBuffer3D& source,
                             const CoordinateSystem3D& plane,
                             double voxelSize)
  {
    Reset();

    // Firstly, compute the intersection of the source volumetric
    // image with the reslicing plane. This leads to a polygon with 3
    // to 6 vertices. We compute the extent of the intersection
    // polygon, with respect to the coordinate system of the reslicing
    // plane.
    OrientedBoundingBox box(source);

    if (!box.ComputeExtent(extent_, plane))
    {
      // The plane does not intersect with the bounding box of the volume
      slice_.reset(new Orthanc::Image(outputFormat_, 0, 0, false));
      success_ = true;
      return;
    }

    // Secondly, the extent together with the voxel size gives the
    // size of the output image
    unsigned int width = boost::math::iround(extent_.GetWidth() / voxelSize);
    unsigned int height = boost::math::iround(extent_.GetHeight() / voxelSize);

    slice_.reset(new Orthanc::Image(outputFormat_, width, height, false));

    //CheckIterators(source, plane, box);
      
    if (fastMode_)
    {
      ProcessImage<FastRowIterator>(*slice_, extent_, source, plane, box,
                                    interpolation_, hasLinearFunction_, scaling_, offset_);
    }
    else
    {
      ProcessImage<SlowRowIterator>(*slice_, extent_, source, plane, box,
                                    interpolation_, hasLinearFunction_, scaling_, offset_);
    }

    success_ = true;
  }
}
