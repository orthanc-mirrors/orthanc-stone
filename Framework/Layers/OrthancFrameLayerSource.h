/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "LayerSourceBase.h"
#include "../Toolbox/IWebService.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/FullOrthancDataset.h"

namespace OrthancStone
{
  class OrthancFrameLayerSource :
    public LayerSourceBase,
    public IWebService::ICallback
  {
  private:
    enum Content
    {
      Content_Tags,
      Content_Frame
    };

    class Operation;
    
    IWebService&                                      orthanc_;
    std::string                                       instanceId_;
    unsigned int                                      frame_;
    std::auto_ptr<OrthancPlugins::FullOrthancDataset> dataset_;
    Orthanc::PixelFormat                              format_;

  public:
    OrthancFrameLayerSource(IWebService& orthanc,
                            const std::string& instanceId,
                            unsigned int frame);

    virtual void SetObserver(IObserver& observer);

    virtual void NotifyError(const std::string& uri,
                             Orthanc::IDynamicObject* payload);

    virtual void NotifySuccess(const std::string& uri,
                               const void* answer,
                               size_t answerSize,
                               Orthanc::IDynamicObject* payload);

    virtual bool GetExtent(double& x1,
                           double& y1,
                           double& x2,
                           double& y2,
                           const SliceGeometry& viewportSlice /* ignored */);

    virtual void ScheduleLayerCreation(const SliceGeometry& viewportSlice);
  };
}
