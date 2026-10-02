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

#include "../../../OrthancStone/Sources/Loaders/DicomResourcesLoader.h"
#include "../../../OrthancStone/Sources/Loaders/SeriesFramesLoader.h"
#include "../../../OrthancStone/Sources/Scene2D/AnnotationsSceneLayer.h"
#include "../../../OrthancStone/Sources/Scene2DViewport/ViewportController.h"
#include "../../../OrthancStone/Sources/Viewport/DefaultViewportInteractor.h"
#include "ISingleViewportApplicationCore.h"


namespace OrthancStone
{
  class SimpleViewerCore :
    public ISingleViewportApplicationCore,
    public boost::enable_shared_from_this<SimpleViewerCore>,
    public New::TypedObserver<FrameDecodedMessage>,
    public New::TypedObserver<DicomResourcesLoadedMessage>
  {
  protected:
    boost::shared_ptr<StoneApplication::Context>  context_;
    boost::shared_ptr<IViewport>                  viewport_;

  private:
    std::string                                   instanceId_;
    unsigned int                                  frameIndex_;
    AnnotationsSceneLayer                         annotations_;
    DefaultViewportInteractor                     interactor_;
    boost::shared_ptr<DicomResourcesLoader>       dicomLoader_;
    boost::shared_ptr<SeriesFramesLoader>         framesLoader_;
    Units                                         units_;

  public:
    virtual void Handle(const New::IObservable& observable,
                        const FrameDecodedMessage& message) ORTHANC_OVERRIDE;

    virtual void Handle(const New::IObservable& observable,
                        const DicomResourcesLoadedMessage& message) ORTHANC_OVERRIDE;

  protected:
    virtual void CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                  const boost::shared_ptr<IViewport>& viewport) ORTHANC_OVERRIDE;

    virtual bool HandleKeyDown(const IEnvironment::ILock& environmentLock,
                               char key) ORTHANC_OVERRIDE;

    virtual void HandleMouseDown(const IEnvironment::ILock& environmentLock,
                                 IViewport::ILock& viewportLock,
                                 const PointerEvent& p) ORTHANC_OVERRIDE;

    virtual bool HandleMouseMove(const IEnvironment::ILock& environmentLock,
                                 IViewport::ILock& viewportLock,
                                 const PointerEvent& p) ORTHANC_OVERRIDE;

    virtual void Render(const IEnvironment::ILock& environmentLock,
                        IViewport::ILock& viewportLock) ORTHANC_OVERRIDE;

  public:
    SimpleViewerCore(const std::string& instanceId,
                     unsigned int frameIndex);
  };
}
