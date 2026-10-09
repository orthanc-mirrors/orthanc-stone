/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/

#pragma once

#include "../Scene2D/Scene2D.h"

#include <boost/noncopyable.hpp>


namespace OrthancStone
{
  class PolylineSceneLayer;
  class TextSceneLayer;

  /**
  This class holds the indices of a set a layer and supplies
  getters to the concrete layer objects. Sounds very ad hoc, and it is.
  */
  class LayerHolder : public boost::noncopyable
  {
  private:
    int textLayerCount_;
    int polylineLayerCount_;
    int infoTextCount_;
    int baseLayerIndex_;

    int GetPolylineLayerIndex(int index);

    int GetTextLayerIndex(int index);

    int GetInfoTextLayerIndex(int index);

  public:
    /**
    This ctor merely stores the scene and layer counts. No layer creation
    performed at this time
    */
    LayerHolder(int polylineLayerCount, int textLayerCount, int infoTextCount = 0);

    /**
    This actually creates the layers
    */
    void CreateLayers(Scene2D& scene);

    /**
    This creates the layers if they are not created yet. Can be useful in 
    some scenarios
    */
    void CreateLayersIfNeeded(Scene2D& scene);

    /**
    Whether the various text and polylines layers have all been created or 
    none at all
    */
    bool AreLayersCreated() const;

    /**
    This removes the layers from the scene
    */
    void DeleteLayers(Scene2D& scene);

    /**
    This removes the layers from the scene if they are already created
    */
    void DeleteLayersIfNeeded(Scene2D& scene);

    /**
    Please note that the returned pointer belongs to the scene.Don't you dare
    storing or deleting it, you fool!

    This throws if the index is not valid or if the layers are not created or
    have been deleted
    */
    PolylineSceneLayer* GetPolylineLayer(Scene2D& scene,
                                         int index = 0);

    /**
    Please note that the returned pointer belongs to the scene. Don't you dare
    storing or deleting it, you fool!

    This throws if the index is not valid or if the layers are not created or
    have been deleted
    */
    TextSceneLayer* GetTextLayer(Scene2D& scene,
                                 int index = 0);
  };
}
