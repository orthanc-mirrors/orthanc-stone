/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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
#include "../Messages/ObserverBase.h"
#include "../Deprecated/Toolbox/DicomFrameConverter.h"
#include "../Deprecated/Toolbox/OrthancApiClient.h"
#include "../StoneEnumerations.h"
#include "../Scene2D/Scene2D.h"

#include <Images/Image.h>
#include <Images/ImageProcessing.h>


namespace OrthancStone
{
  class RadiographyDicomLayer;

  class RadiographyScene :
    public ObserverBase<RadiographyScene>,
    public IObservable
  {
    friend class RadiographySceneGeometryReader;
  public:
    class GeometryChangedMessage : public OriginMessage<RadiographyScene>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

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

    class ContentChangedMessage : public OriginMessage<RadiographyScene>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

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

    class LayerEditedMessage : public OriginMessage<RadiographyScene>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

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

    class LayerRemovedMessage : public OriginMessage<RadiographyScene>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

    private:
      size_t&        layerIndex_;

    public:
      LayerRemovedMessage(const RadiographyScene& origin,
                          size_t& layerIndex) :
        OriginMessage(origin),
        layerIndex_(layerIndex)
      {
      }

      size_t& GetLayerIndex() const
      {
        return layerIndex_;
      }
    };


    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, WindowingChangedMessage, RadiographyScene);

    
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

    size_t  nextLayerIndex_;
    bool    hasWindowing_;
    float   windowingCenter_;
    float   windowingWidth_;
    Layers  layers_;

  public:
    RadiographyLayer& RegisterLayer(RadiographyLayer* layer);

  protected:
    virtual void _RegisterLayer(RadiographyLayer* layer);
    virtual void _OnLayerRemoved() {}

    void SetLayerIndex(RadiographyLayer* layer, size_t index)
    {
      layer->SetIndex(index);
    }

    virtual void OnTagsReceived(const Deprecated::OrthancApiClient::BinaryResponseReadyMessage& message);

    virtual void OnFrameReceived(const Deprecated::OrthancApiClient::BinaryResponseReadyMessage& message);
    
    void OnDicomExported(const Deprecated::OrthancApiClient::JsonResponseReadyMessage& message);

    void OnDicomWebReceived(const Deprecated::IWebService::HttpRequestSuccessMessage& message);

    virtual void OnLayerEdited(const RadiographyLayer::LayerEditedMessage& message);

  public:
    RadiographyScene();
    
    virtual ~RadiographyScene();

    virtual size_t GetApproximateMemoryUsage() const;

    bool GetWindowing(float& center,
                      float& width) const;

    void GetWindowingWithDefault(float& center,
                                 float& width) const;

    virtual void SetWindowing(float center,
                              float width);

    RadiographyPhotometricDisplayMode GetPreferredPhotomotricDisplayMode() const;

    RadiographyLayer& LoadText(const std::string& utf8,
                               const std::string& font,
                               unsigned int fontSize,
                               uint8_t foreground,
                               RadiographyLayer::Geometry* geometry,
                               bool isCenterGeometry);

    RadiographyLayer& UpdateText(size_t layerIndex,
                                 const std::string& font,
                                 const std::string& utf8,
                                 unsigned int fontSize,
                                 uint8_t foreground);

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
                                             Deprecated::DicomFrameConverter* converter,  // takes ownership
                                             RadiographyPhotometricDisplayMode preferredPhotometricDisplayMode,
                                             RadiographyLayer::Geometry* geometry);

    virtual RadiographyLayer& LoadDicomFrame(Deprecated::OrthancApiClient& orthanc,
                                             const std::string& instance,
                                             unsigned int frame,
                                             bool httpCompression,
                                             RadiographyLayer::Geometry* geometry); // pass NULL if you want default geometry

    RadiographyLayer& LoadDicomWebFrame(Deprecated::IWebService& web);

    void RemoveLayer(size_t layerIndex);

    RadiographyLayer& GetLayer(size_t layerIndex);

    const RadiographyLayer& GetLayer(size_t layerIndex) const;

    template <typename TypeLayer>
    TypeLayer* GetTypedLayer(size_t indexOfType = 0)
    {
      std::vector<size_t> layerIndexes;
      GetLayersIndexes(layerIndexes);

      size_t count = 0;

      for (size_t i = 0; i < layerIndexes.size(); ++i)
      {
        TypeLayer* typedLayer = dynamic_cast<TypeLayer*>(layers_[layerIndexes[i]]);
        if (typedLayer != NULL)
        {
          if (count == indexOfType)
          {
            return typedLayer;
          }
          count++;
        }
      }

      return NULL;
    }

    void GetLayersIndexes(std::vector<size_t>& output) const;

    virtual Extent2D GetSceneExtent(bool minimal) const;

    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation,
                        bool applyWindowing) const;

    bool LookupLayer(size_t& index /* out */,
                     double x,
                     double y) const;
    
    void DrawBorder(CairoContext& context,
                    unsigned int layer,
                    double zoom);

    void GetRange(float& minValue,
                  float& maxValue) const;

    void ExportToScene2D(Scene2D& output) const;

    // Export using PAM is faster than using PNG, but requires Orthanc
    // core >= 1.4.3
    void ExportDicom(Deprecated::OrthancApiClient& orthanc,
                     const Orthanc::DicomMap& dicom,
                     const std::string& parentOrthancId,
                     double pixelSpacingX,
                     double pixelSpacingY,
                     bool invert,
                     bool autoCrop,
                     ImageInterpolation interpolation,
                     bool usePam);

    void ExportDicom(Deprecated::OrthancApiClient& orthanc,
                     const Json::Value& dicomTags,
                     const std::string& parentOrthancId,
                     double pixelSpacingX,
                     double pixelSpacingY,
                     bool invert,
                     bool autoCrop,
                     ImageInterpolation interpolation,
                     bool usePam);

    void ExportToCreateDicomRequest(Json::Value& createDicomRequestContent,
                                    const Json::Value& dicomTags,
                                    const std::string& parentOrthancId,
                                    double pixelSpacingX,
                                    double pixelSpacingY,
                                    bool invert,
                                    bool autoCrop,
                                    ImageInterpolation interpolation,
                                    bool usePam);

    Orthanc::Image* ExportToCreateDicomRequestAndImage(Json::Value& createDicomRequestContent,
                                                       const Json::Value& dicomTags,
                                                       const std::string& parentOrthancId,
                                                       double pixelSpacingX,
                                                       double pixelSpacingY,
                                                       bool invert,
                                                       bool autoCrop,
                                                       ImageInterpolation interpolation);

    Orthanc::Image* ExportToImage(double pixelSpacingX,
                                  double pixelSpacingY,
                                  ImageInterpolation interpolation,
                                  bool autoCrop,
                                  bool applyWindowing)
    {
      return ExportToImage(pixelSpacingX, pixelSpacingY, interpolation, false, 0, autoCrop, applyWindowing);
    }

    Orthanc::Image* ExportToImage(double pixelSpacingX,
                                  double pixelSpacingY,
                                  ImageInterpolation interpolation,
                                  bool invert,
                                  int64_t maxValue /* for inversion */,
                                  bool autoCrop,
                                  bool applyWindowing);

    void ExtractLayerFromRenderedScene(Orthanc::ImageAccessor& layer,
                                       const Orthanc::ImageAccessor& renderedScene,
                                       size_t layerIndex,
                                       bool isCropped,
                                       ImageInterpolation interpolation);
  };
}