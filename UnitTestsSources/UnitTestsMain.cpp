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


#include "gtest/gtest.h"

#include "../Resources/Orthanc/Core/Logging.h"
#include "../Framework/Toolbox/OrthancWebService.h"
#include "../Framework/Layers/OrthancFrameLayerSource.h"
#include "../Framework/Widgets/LayerWidget.h"


namespace OrthancStone
{
  class Tata : public ILayerSource::IObserver
  {
  public:
    virtual void NotifySourceChange(ILayerSource& source)
    {
      printf("Source change\n");

      OrthancStone::SliceGeometry slice;
      double x1, y1, x2, y2;
      printf(">> %d: ", source.GetExtent(x1, y1, x2, y2, slice));
      printf("(%f,%f) (%f,%f)\n", x1, y1, x2, y2);
    }

    virtual void NotifySliceChange(ILayerSource& source,
                                   const SliceGeometry& slice)
    {
      printf("Slice change\n");
    }

    virtual void NotifyLayerReady(ILayerRenderer* layer,
                                  ILayerSource& source,
                                  const SliceGeometry& viewportSlice)
    {
      std::auto_ptr<ILayerRenderer> tmp(layer);
      
    }

    virtual void NotifyLayerError(ILayerSource& source,
                                  const SliceGeometry& viewportSlice)
    {
    }
  };



  /*class OrthancInstanceLoader : public IWebService::ICallback
  {
  public:
    class ICallback : public boost::noncopyable
    {
    public:
      virtual ~ICallback()
      {
      }
      
      virtual void NotifyInstanceLoaded(const std::string& instanceId,
                                        const OrthancPlugins::FullOrthancDataset& dicom) = 0;
      
      virtual void NotifyInstanceError(const std::string& instanceId) = 0;
    };

  private:
    class Operation : public Orthanc::IDynamicObject
    {
    private:
      ICallback&   callback_;
      std::string  instanceId_;

    public:
      Operation(ICallback& callback,
                const std::string& instanceId) :
        callback_(callback),
        instanceId_(instanceId)
      {
      }

      ICallback& GetCallback()
      {
        return callback_;
      }

      const std::string& GetInstanceId() const
      {
        return instanceId_;
      }
    };

    IWebService&  orthanc_;

  public:
    OrthancInstanceLoader(IWebService&  orthanc) :
      orthanc_(orthanc)
    {
    }

    void ScheduleLoadInstance(ICallback& callback,
                              const std::string& instanceId)
    {
      orthanc_.ScheduleGetRequest(*this,
                                  "/instances/" + instanceId + "/tags",
                                  new Operation(callback, instanceId));
    }
    
    void NotifySuccess(const std::string& uri,
                       const void* answer,
                       size_t answerSize,
                       Orthanc::IDynamicObject* payload)
    {
      std::auto_ptr<Operation> operation(reinterpret_cast<Operation*>(payload));

      try
      {
        OrthancPlugins::FullOrthancDataset dataset(answer, answerSize);
        operation->GetCallback().NotifyInstanceLoaded(operation->GetInstanceId(), dataset);
      }
      catch (Orthanc::OrthancException&)
      {
        operation->GetCallback().NotifyInstanceError(operation->GetInstanceId());
      }
    }
      
    void NotifyError(const std::string& uri,
                     Orthanc::IDynamicObject* payload)
    {
      std::auto_ptr<Operation> operation(reinterpret_cast<Operation*>(payload));

      LOG(ERROR) << "Cannot download " << uri;
      operation->GetCallback().NotifyInstanceError(operation->GetInstanceId());
    }
    };*/

  

}



TEST(Toto, Tutu)
{
  Orthanc::WebServiceParameters web;
  OrthancStone::OrthancWebService orthanc(web);
  OrthancStone::OrthancFrameLayerSource source(orthanc, "befb52a6-b4b04954-b5a019c3-fdada9d7-dddc9430", 0);

  OrthancStone::Tata tata;
  source.SetObserver(tata);

  OrthancStone::SliceGeometry slice;
  source.ScheduleLayerCreation(slice);


  OrthancStone::LayerWidget widget;
  printf(">> %d\n", widget.AddLayer(new OrthancStone::OrthancFrameLayerSource(orthanc, "befb52a6-b4b04954-b5a019c3-fdada9d7-dddc9430", 0)));
}



int main(int argc, char **argv)
{
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  return result;
}
