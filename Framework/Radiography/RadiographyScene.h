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

#include "RadiographyLayer.h"
#include "../Toolbox/OrthancApiClient.h"
#include "Framework/StoneEnumerations.h"
#include "Core/Images/Image.h"
#include "Core/Images/ImageProcessing.h"

namespace OrthancStone
{
  class RadiographyDicomLayer;
  class DicomFrameConverter;

  class RadiographyScene :
      public IObserver,
      public IObservable
  {
  public:
    class GeometryChangedMessage :
        public OriginMessage<MessageType_RadiographyScene_GeometryChanged, RadiographyScene>
    {
    private:
      RadiographyLayer&        layer_;

    public:
      GeometryChangedMessage(const RadiographyScene& origin,
                             RadiographyLayer& layer) :
        OriginMessage(origin),
        layer_(layer)
      {
      }

      RadiographyLayer& GetLayer() const
      {
        return layer_;
      }
    };

    class ContentChangedMessage :
        public OriginMessage<MessageType_RadiographyScene_ContentChanged, RadiographyScene>
    {
    private:
      RadiographyLayer&        layer_;

    public:
      ContentChangedMessage(const RadiographyScene& origin,
                            RadiographyLayer& layer) :
        OriginMessage(origin),
        layer_(layer)
      {
      }

      RadiographyLayer& GetLayer() const
      {
        return layer_;
      }
    };

    class LayerEditedMessage :
        public OriginMessage<MessageType_RadiographyScene_LayerEdited, RadiographyScene>
    {
    private:
      const RadiographyLayer&        layer_;

    public:
      LayerEditedMessage(const RadiographyScene& origin,
                         const RadiographyLayer& layer) :
        OriginMessage(origin),
        layer_(layer)
      {
      }

      const RadiographyLayer& GetLayer() const
      {
        return layer_;
      }

    };

    class WindowingChangedMessage :
        public OriginMessage<MessageType_RadiographyScene_WindowingChanged, RadiographyScene>
    {

    public:
      WindowingChangedMessage(const RadiographyScene& origin) :
        OriginMessage(origin)
      {
      }
    };

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


  protected:
    typedef std::map<size_t, RadiographyLayer*>  Layers;

    size_t  countLayers_;
    bool    hasWindowing_;
    float   windowingCenter_;
    float   windowingWidth_;
    Layers  layers_;

  protected:
    RadiographyLayer& RegisterLayer(RadiographyLayer* layer);

    void OnTagsReceived(const OrthancApiClient::BinaryResponseReadyMessage& message);

    void OnFrameReceived(const OrthancApiClient::BinaryResponseReadyMessage& message);
    
    void OnDicomExported(const OrthancApiClient::JsonResponseReadyMessage& message);

    void OnDicomWebReceived(const IWebService::HttpRequestSuccessMessage& message);

    void OnLayerEdited(const RadiographyLayer::LayerEditedMessage& message);
  public:
    RadiographyScene(MessageBroker& broker);
    
    virtual ~RadiographyScene();

    bool GetWindowing(float& center,
                      float& width) const;

    void GetWindowingWithDefault(float& center,
                                 float& width) const;

    void SetWindowing(float center,
                      float width);

    PhotometricDisplayMode GetPreferredPhotomotricDisplayMode() const;

    RadiographyLayer& LoadText(const Orthanc::Font& font,
                               const std::string& utf8,
                               RadiographyLayer::Geometry* geometry);
    
    RadiographyLayer& LoadTestBlock(unsigned int width,
                                    unsigned int height,
                                    RadiographyLayer::Geometry* geometry);

    RadiographyLayer& LoadMask(const std::vector<Orthanc::ImageProcessing::ImagePoint>& corners,
                               const RadiographyDicomLayer& dicomLayer,
                               float foreground,
                               RadiographyLayer::Geometry* geometry);

    RadiographyLayer& LoadAlphaBitmap(Orthanc::ImageAccessor* bitmap,  // takes ownership
                                      RadiographyLayer::Geometry* geometry);

    virtual RadiographyLayer& LoadDicomImage(Orthanc::ImageAccessor* dicomImage, // takes ownership
                                             const std::string& instance,
                                             unsigned int frame,
                                             DicomFrameConverter* converter,  // takes ownership
                                             PhotometricDisplayMode preferredPhotometricDisplayMode,
                                             RadiographyLayer::Geometry* geometry);

    virtual RadiographyLayer& LoadDicomFrame(OrthancApiClient& orthanc,
                                             const std::string& instance,
                                             unsigned int frame,
                                             bool httpCompression,
                                             RadiographyLayer::Geometry* geometry); // pass NULL if you want default geometry

    RadiographyLayer& LoadDicomWebFrame(IWebService& web);

    void RemoveLayer(size_t layerIndex);

    const RadiographyLayer& GetLayer(size_t layerIndex) const;

    template <typename TypeLayer>
    TypeLayer* GetLayer(size_t index = 0)
    {
      std::vector<size_t> layerIndexes;
      GetLayersIndexes(layerIndexes);

      size_t count = 0;

      for (size_t i = 0; i < layerIndexes.size(); ++i)
      {
        TypeLayer* typedLayer = dynamic_cast<TypeLayer*>(layers_[layerIndexes[i]]);
        if (typedLayer != NULL)
        {
          if (count == index)
          {
            return typedLayer;
          }
          count++;
        }
      }

      return NULL;
    }

    template <typename TypeLayer>
    const TypeLayer* GetLayer(size_t index = 0) const
    {
      std::vector<size_t> layerIndexes;
      GetLayersIndexes(layerIndexes);

      size_t count = 0;

      for (size_t i = 0; i < layerIndexes.size(); ++i)
      {
        const TypeLayer* typedLayer = dynamic_cast<const TypeLayer*>(layers_.at(layerIndexes[i]));
        if (typedLayer != NULL)
        {
          if (count == index)
          {
            return typedLayer;
          }
          count++;
        }
      }

      return NULL;
    }

    void GetLayersIndexes(std::vector<size_t>& output) const;

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
    void ExportDicom(OrthancApiClient& orthanc,
                     const Orthanc::DicomMap& dicom,
                     const std::string& parentOrthancId,
                     double pixelSpacingX,
                     double pixelSpacingY,
                     bool invert,
                     ImageInterpolation interpolation,
                     bool usePam);

    void ExportDicom(OrthancApiClient& orthanc,
                     const Json::Value& dicomTags,
                     const std::string& parentOrthancId,
                     double pixelSpacingX,
                     double pixelSpacingY,
                     bool invert,
                     ImageInterpolation interpolation,
                     bool usePam);

    void ExportToCreateDicomRequest(Json::Value& createDicomRequestContent,
                                    const Json::Value& dicomTags,
                                    const std::string& parentOrthancId,
                                    double pixelSpacingX,
                                    double pixelSpacingY,
                                    bool invert,
                                    ImageInterpolation interpolation,
                                    bool usePam);

    Orthanc::Image* ExportToCreateDicomRequestAndImage(Json::Value& createDicomRequestContent,
                                                       const Json::Value& dicomTags,
                                                       const std::string& parentOrthancId,
                                                       double pixelSpacingX,
                                                       double pixelSpacingY,
                                                       bool invert,
                                                       ImageInterpolation interpolation);

    Orthanc::Image* ExportToImage(double pixelSpacingX,
                                  double pixelSpacingY,
                                  ImageInterpolation interpolation)
    {
      return ExportToImage(pixelSpacingX, pixelSpacingY, interpolation, false, 0);
    }

    Orthanc::Image* ExportToImage(double pixelSpacingX,
                                  double pixelSpacingY,
                                  ImageInterpolation interpolation,
                                  bool invert,
                                  int64_t maxValue /* for inversion */);
  };
}
