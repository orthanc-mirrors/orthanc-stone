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

#include "RadiographyLayer.h"
#include "../Toolbox/OrthancApiClient.h"


namespace OrthancStone
{
  class RadiographyScene :
    public IObserver,
    public IObservable
  {
  public:
    typedef OriginMessage<MessageType_Widget_GeometryChanged, RadiographyScene> GeometryChangedMessage;
    typedef OriginMessage<MessageType_Widget_ContentChanged, RadiographyScene> ContentChangedMessage;

    class LayerAccessor : public boost::noncopyable
    {
    private:
      RadiographyScene&  scene_;
      size_t             index_;
      RadiographyLayer*  layer_;

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

      RadiographyLayer& GetLayer() const;
    };


  private:
    class AlphaLayer;    
    class DicomLayer;

    typedef std::map<size_t, RadiographyLayer*>  Layers;
        
    OrthancApiClient&  orthanc_;
    size_t             countLayers_;
    bool               hasWindowing_;
    float              windowingCenter_;
    float              windowingWidth_;
    Layers             layers_;

    RadiographyLayer& RegisterLayer(RadiographyLayer* layer);

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

    RadiographyLayer& LoadText(const Orthanc::Font& font,
                               const std::string& utf8);
    
    RadiographyLayer& LoadTestBlock(unsigned int width,
                                    unsigned int height);
    
    RadiographyLayer& LoadDicomFrame(const std::string& instance,
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