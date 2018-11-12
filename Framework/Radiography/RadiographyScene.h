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


#pragma once

#include "../Toolbox/AffineTransform2D.h"
#include "../Toolbox/Extent2D.h"
#include "../Toolbox/OrthancApiClient.h"
#include "../Viewport/CairoContext.h"


namespace OrthancStone
{
  class RadiographyScene :
    public IObserver,
    public IObservable
  {
  public:
    typedef OriginMessage<MessageType_Widget_GeometryChanged, RadiographyScene> GeometryChangedMessage;
    typedef OriginMessage<MessageType_Widget_ContentChanged, RadiographyScene> ContentChangedMessage;

    enum Corner
    {
      Corner_TopLeft,
      Corner_TopRight,
      Corner_BottomLeft,
      Corner_BottomRight
    };


    class Layer : public boost::noncopyable
    {
      friend class RadiographyScene;
      
    private:
      size_t             index_;
      bool               hasSize_;
      unsigned int       width_;
      unsigned int       height_;
      bool               hasCrop_;
      unsigned int       cropX_;
      unsigned int       cropY_;
      unsigned int       cropWidth_;
      unsigned int       cropHeight_;
      AffineTransform2D  transform_;
      AffineTransform2D  transformInverse_;
      double             pixelSpacingX_;
      double             pixelSpacingY_;
      double             panX_;
      double             panY_;
      double             angle_;
      bool               resizeable_;


    protected:
      const AffineTransform2D& GetTransform() const
      {
        return transform_;
      }


    private:
      void UpdateTransform();
      
      void AddToExtent(Extent2D& extent,
                       double x,
                       double y) const;

      void GetCornerInternal(double& x,
                             double& y,
                             Corner corner,
                             unsigned int cropX,
                             unsigned int cropY,
                             unsigned int cropWidth,
                             unsigned int cropHeight) const;

      void SetIndex(size_t index)
      {
        index_ = index;
      }
      
      bool Contains(double x,
                    double y) const;
      
      void DrawBorders(CairoContext& context,
                       double zoom);

    public:
      Layer();

      virtual ~Layer()
      {
      }

      size_t GetIndex() const
      {
        return index_;
      }

      void ResetCrop()
      {
        hasCrop_ = false;
      }

      void SetCrop(unsigned int x,
                   unsigned int y,
                   unsigned int width,
                   unsigned int height);

      void GetCrop(unsigned int& x,
                   unsigned int& y,
                   unsigned int& width,
                   unsigned int& height) const;

      void SetAngle(double angle);

      double GetAngle() const
      {
        return angle_;
      }

      void SetSize(unsigned int width,
                   unsigned int height);

      unsigned int GetWidth() const
      {
        return width_;
      }        

      unsigned int GetHeight() const
      {
        return height_;
      }       

      Extent2D GetExtent() const;

      bool GetPixel(unsigned int& imageX,
                    unsigned int& imageY,
                    double sceneX,
                    double sceneY) const;

      void SetPan(double x,
                  double y);

      void SetPixelSpacing(double x,
                           double y);

      double GetPixelSpacingX() const
      {
        return pixelSpacingX_;
      }   

      double GetPixelSpacingY() const
      {
        return pixelSpacingY_;
      }   

      double GetPanX() const
      {
        return panX_;
      }

      double GetPanY() const
      {
        return panY_;
      }

      void GetCenter(double& centerX,
                     double& centerY) const;

      void GetCorner(double& x /* out */,
                     double& y /* out */,
                     Corner corner) const;
      
      bool LookupCorner(Corner& corner /* out */,
                        double x,
                        double y,
                        double zoom,
                        double viewportDistance) const;

      bool IsResizeable() const
      {
        return resizeable_;
      }

      void SetResizeable(bool resizeable)
      {
        resizeable_ = resizeable;
      }

      virtual bool GetDefaultWindowing(float& center,
                                       float& width) const = 0;

      virtual void Render(Orthanc::ImageAccessor& buffer,
                          const AffineTransform2D& viewTransform,
                          ImageInterpolation interpolation) const = 0;

      virtual bool GetRange(float& minValue,
                            float& maxValue) const = 0;
    }; 


    class LayerAccessor : public boost::noncopyable
    {
    private:
      RadiographyScene&  scene_;
      size_t             index_;
      Layer*             layer_;

    public:
      LayerAccessor(RadiographyScene& scene,
                    size_t index);

      LayerAccessor(RadiographyScene& scene,
                    double x,
                    double y);

      void Invalidate()
      {
        layer_ = NULL;
      }

      bool IsValid() const
      {
        return layer_ != NULL;
      }

      RadiographyScene& GetScene() const;

      size_t GetIndex() const;

      Layer& GetLayer() const;
    };


  private:
    class AlphaLayer;    
    class DicomLayer;

    typedef std::map<size_t, Layer*>  Layers;
        
    OrthancApiClient&  orthanc_;
    size_t             countLayers_;
    bool               hasWindowing_;
    float              windowingCenter_;
    float              windowingWidth_;
    Layers             layers_;

    Layer& RegisterLayer(Layer* layer);

    void OnTagsReceived(const OrthancApiClient::BinaryResponseReadyMessage& message);

    void OnFrameReceived(const OrthancApiClient::BinaryResponseReadyMessage& message);
    
    void OnDicomExported(const OrthancApiClient::JsonResponseReadyMessage& message);

  public:
    RadiographyScene(MessageBroker& broker,
                     OrthancApiClient& orthanc);
    
    virtual ~RadiographyScene();

    bool GetWindowing(float& center,
                      float& width) const;

    void GetWindowingWithDefault(float& center,
                                 float& width) const;

    void SetWindowing(float center,
                      float width);

    Layer& LoadText(const Orthanc::Font& font,
                    const std::string& utf8);
    
    Layer& LoadTestBlock(unsigned int width,
                         unsigned int height);
    
    Layer& LoadDicomFrame(const std::string& instance,
                          unsigned int frame,
                          bool httpCompression);

    Extent2D GetSceneExtent() const;

    void Render(Orthanc::ImageAccessor& buffer,
                const AffineTransform2D& viewTransform,
                ImageInterpolation interpolation) const;

    bool LookupLayer(size_t& index /* out */,
                     double x,
                     double y) const;
    
    void DrawBorder(CairoContext& context,
                    unsigned int layer,
                    double zoom);

    void GetRange(float& minValue,
                  float& maxValue) const;

    // Export using PAM is faster than using PNG, but requires Orthanc
    // core >= 1.4.3
    void ExportDicom(const Orthanc::DicomMap& dicom,
                     double pixelSpacingX,
                     double pixelSpacingY,
                     bool invert,
                     ImageInterpolation interpolation,
                     bool usePam);
  };
}
