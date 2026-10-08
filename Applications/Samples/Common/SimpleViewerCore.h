/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

// TODO Refactoring - Remove
#define TEST_LINE_MEASURE_TOOL   1
#define TEST_ANGLE_MEASURE_TOOL  1

#include "../../../OrthancStone/Sources/Loaders/DicomResourcesLoader.h"
#include "../../../OrthancStone/Sources/Loaders/SeriesFramesLoader.h"
#include "../../../OrthancStone/Sources/Scene2D/AnnotationsSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2DViewport/ViewportController.h"
#include "ISingleViewportApplicationCore.h"

#include "../../../OrthancStone/Sources/Scene2DViewport/LineMeasureTool.h"  // TODO Refactoring - Remove
#include "../../../OrthancStone/Sources/Scene2DViewport/AngleMeasureTool.h"  // TODO Refactoring - Remove


namespace OrthancStone
{
  class SimpleViewerCore :
    public ISingleViewportApplicationCore,
    //public boost::enable_shared_from_this<SimpleViewerCore>,  // TODO Refactoring - Revert
    public New::TypedObserver<FrameDecodedMessage>,
    public New::TypedObserver<DicomResourcesLoadedMessage>,
    public OrthancStone::ObserverBase<SimpleViewerCore>  // TODO Refactoring - Remove
  {
  protected:
    boost::shared_ptr<StoneApplication::Context>  context_;
    boost::shared_ptr<IViewport>                  viewport_;

  private:
    class Interactor;

    boost::shared_ptr<AnnotationsSceneLayer>      annotations_;
    boost::shared_ptr<DicomResourcesLoader>       dicomLoader_;
    boost::shared_ptr<SeriesFramesLoader>         framesLoader_;

#if TEST_LINE_MEASURE_TOOL == 1
    boost::shared_ptr<LineMeasureTool>  lineMeasureTool_;  // TODO Refactoring - Remove
#endif

#if TEST_ANGLE_MEASURE_TOOL == 1
    boost::shared_ptr<AngleMeasureTool>  angleMeasureTool_;  // TODO Refactoring - Remove
#endif

  public:
    virtual void Handle(const New::IObservable& observable,
                        const FrameDecodedMessage& message) ORTHANC_OVERRIDE;

    virtual void Handle(const New::IObservable& observable,
                        const DicomResourcesLoadedMessage& message) ORTHANC_OVERRIDE;

    void Handle(const AnnotationsSceneLayer::AnnotationRemovedMessage& message)
    {
      RenderAnnotations();
    }

    void RenderAnnotations();

  protected:
    virtual void CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                  const boost::shared_ptr<IViewport>& viewport) ORTHANC_OVERRIDE;

    virtual IViewportInteractor* CreateMouseInteractor() ORTHANC_OVERRIDE;

    virtual void HandleKeyDown(const IEnvironment::ILock& environmentLock,
                               char key) ORTHANC_OVERRIDE;

  public:
    void LoadFrameFromOrthanc(const std::string& instanceId,
                              unsigned int frameIndex);

    void LoadFrameFromDicomWeb(const std::string& server,
                               const std::string& studyInstanceUid,
                               const std::string& seriesInstanceUid,
                               const std::string& sopInstanceUid,
                               unsigned int frameIndex);
  };
}
