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


#include "../../Framework/Loaders/DicomStructureSetLoader.h"
#include "../../Framework/Loaders/OrthancMultiframeVolumeLoader.h"
#include "../../Framework/Loaders/OrthancSeriesVolumeProgressiveLoader.h"
#include "../../Framework/Oracle/SleepOracleCommand.h"
#include "../../Framework/Oracle/ThreadedOracle.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/GrayscaleStyleConfigurator.h"
#include "../../Framework/Scene2D/LookupTableStyleConfigurator.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Volumes/VolumeSceneLayerSource.h"
#include "../../Framework/Volumes/DicomVolumeImageMPRSlicer.h"
#include "../../Framework/Volumes/DicomVolumeImageReslicer.h"

// From Orthanc framework
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/SystemToolbox.h>


namespace OrthancStone
{
  class NativeApplicationContext : public IMessageEmitter
  {
  private:
    boost::shared_mutex  mutex_;
    MessageBroker        broker_;
    IObservable          oracleObservable_;

  public:
    NativeApplicationContext() :
      oracleObservable_(broker_)
    {
    }


    virtual void EmitMessage(const IObserver& observer,
                             const IMessage& message) ORTHANC_OVERRIDE
    {
      try
      {
        boost::unique_lock<boost::shared_mutex>  lock(mutex_);
        oracleObservable_.EmitMessage(observer, message);
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Exception while emitting a message: " << e.What();
      }
    }


    class ReaderLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::shared_lock<boost::shared_mutex>  lock_;

    public:
      ReaderLock(NativeApplicationContext& that) : 
        that_(that),
        lock_(that.mutex_)
      {
      }
    };


    class WriterLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::unique_lock<boost::shared_mutex>  lock_;

    public:
      WriterLock(NativeApplicationContext& that) : 
        that_(that),
        lock_(that.mutex_)
      {
      }

      MessageBroker& GetBroker() 
      {
        return that_.broker_;
      }

      IObservable& GetOracleObservable()
      {
        return that_.oracleObservable_;
      }
    };
  };
}



class Toto : public OrthancStone::IObserver
{
private:
  OrthancStone::CoordinateSystem3D  plane_;
  OrthancStone::IOracle&            oracle_;
  OrthancStone::Scene2D             scene_;
  std::auto_ptr<OrthancStone::VolumeSceneLayerSource>  source1_, source2_, source3_;


  void Refresh()
  {
    if (source1_.get() != NULL)
    {
      source1_->Update(plane_);
    }
      
    if (source2_.get() != NULL)
    {
      source2_->Update(plane_);
    }

    if (source3_.get() != NULL)
    {
      source3_->Update(plane_);
    }

    scene_.FitContent(1024, 768);
      
    {
      OrthancStone::CairoCompositor compositor(scene_, 1024, 768);
      compositor.Refresh();
        
      Orthanc::ImageAccessor accessor;
      compositor.GetCanvas().GetReadOnlyAccessor(accessor);

      Orthanc::Image tmp(Orthanc::PixelFormat_RGB24, accessor.GetWidth(), accessor.GetHeight(), false);
      Orthanc::ImageProcessing::Convert(tmp, accessor);
        
      static unsigned int count = 0;
      char buf[64];
      sprintf(buf, "scene-%06d.png", count++);
        
      Orthanc::PngWriter writer;
      writer.WriteToFile(buf, tmp);
    }
  }

  
  void Handle(const OrthancStone::DicomVolumeImage::GeometryReadyMessage& message)
  {
    printf("Geometry ready\n");
    
    plane_ = message.GetOrigin().GetGeometry().GetSagittalGeometry();
    //plane_ = message.GetOrigin().GetGeometry().GetAxialGeometry();
    //plane_ = message.GetOrigin().GetGeometry().GetCoronalGeometry();
    plane_.SetOrigin(message.GetOrigin().GetGeometry().GetCoordinates(0.5f, 0.5f, 0.5f));

    Refresh();
  }
  
  
  void Handle(const OrthancStone::SleepOracleCommand::TimeoutMessage& message)
  {
    if (message.GetOrigin().HasPayload())
    {
      printf("TIMEOUT! %d\n", dynamic_cast<const Orthanc::SingleValueObject<unsigned int>& >(message.GetOrigin().GetPayload()).GetValue());
    }
    else
    {
      printf("TIMEOUT\n");

      Refresh();

      /**
       * The sleep() leads to a crash if the oracle is still running,
       * while this object is destroyed. Always stop the oracle before
       * destroying active objects.  (*)
       **/
      // boost::this_thread::sleep(boost::posix_time::seconds(2));

      oracle_.Schedule(*this, new OrthancStone::SleepOracleCommand(message.GetOrigin().GetDelay()));
    }
  }

  void Handle(const OrthancStone::OrthancRestApiCommand::SuccessMessage& message)
  {
    Json::Value v;
    message.ParseJsonBody(v);

    printf("ICI [%s]\n", v.toStyledString().c_str());
  }

  void Handle(const OrthancStone::GetOrthancImageCommand::SuccessMessage& message)
  {
    printf("IMAGE %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage& message)
  {
    printf("WebViewer %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const OrthancStone::OracleCommandExceptionMessage& message)
  {
    printf("EXCEPTION: [%s] on command type %d\n", message.GetException().What(), message.GetCommand().GetType());

    switch (message.GetCommand().GetType())
    {
      case OrthancStone::IOracleCommand::Type_GetOrthancWebViewerJpeg:
        printf("URI: [%s]\n", dynamic_cast<const OrthancStone::GetOrthancWebViewerJpegCommand&>
               (message.GetCommand()).GetUri().c_str());
        break;
      
      default:
        break;
    }
  }

public:
  Toto(OrthancStone::IOracle& oracle,
       OrthancStone::IObservable& oracleObservable) :
    IObserver(oracleObservable.GetBroker()),
    oracle_(oracle)
  {
    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::SleepOracleCommand::TimeoutMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OrthancRestApiCommand::SuccessMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancImageCommand::SuccessMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage>(*this, &Toto::Handle));

    oracleObservable.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OracleCommandExceptionMessage>(*this, &Toto::Handle));
  }

  void SetReferenceLoader(OrthancStone::IObservable& loader)
  {
    loader.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::DicomVolumeImage::GeometryReadyMessage>(*this, &Toto::Handle));
  }

  void SetVolume1(int depth,
                  const boost::shared_ptr<OrthancStone::IVolumeSlicer>& volume,
                  OrthancStone::ILayerStyleConfigurator* style)
  {
    source1_.reset(new OrthancStone::VolumeSceneLayerSource(scene_, depth, volume));

    if (style != NULL)
    {
      source1_->SetConfigurator(style);
    }
  }

  void SetVolume2(int depth,
                  const boost::shared_ptr<OrthancStone::IVolumeSlicer>& volume,
                  OrthancStone::ILayerStyleConfigurator* style)
  {
    source2_.reset(new OrthancStone::VolumeSceneLayerSource(scene_, depth, volume));

    if (style != NULL)
    {
      source2_->SetConfigurator(style);
    }
  }

  void SetStructureSet(int depth,
                       const boost::shared_ptr<OrthancStone::DicomStructureSetLoader>& volume)
  {
    source3_.reset(new OrthancStone::VolumeSceneLayerSource(scene_, depth, volume));
  }
                       
};


void Run(OrthancStone::NativeApplicationContext& context,
         OrthancStone::ThreadedOracle& oracle)
{
  // the oracle has been supplied with the context (as an IEmitter) upon
  // creation
  boost::shared_ptr<OrthancStone::DicomVolumeImage>  ct(new OrthancStone::DicomVolumeImage);
  boost::shared_ptr<OrthancStone::DicomVolumeImage>  dose(new OrthancStone::DicomVolumeImage);

  
  boost::shared_ptr<Toto> toto;
  boost::shared_ptr<OrthancStone::OrthancSeriesVolumeProgressiveLoader> ctLoader;
  boost::shared_ptr<OrthancStone::OrthancMultiframeVolumeLoader> doseLoader;
  boost::shared_ptr<OrthancStone::DicomStructureSetLoader>  rtstructLoader;

  {
    OrthancStone::NativeApplicationContext::WriterLock lock(context);
    toto.reset(new Toto(oracle, lock.GetOracleObservable()));

    // the oracle is used to schedule commands
    // the oracleObservable is used by the loaders to:
    // - request the broker (lifetime mgmt)
    // - register the loader callbacks (called indirectly by the oracle)
    ctLoader.reset(new OrthancStone::OrthancSeriesVolumeProgressiveLoader(ct, oracle, lock.GetOracleObservable()));
    doseLoader.reset(new OrthancStone::OrthancMultiframeVolumeLoader(dose, oracle, lock.GetOracleObservable()));
    rtstructLoader.reset(new OrthancStone::DicomStructureSetLoader(oracle, lock.GetOracleObservable()));
  }


  //toto->SetReferenceLoader(*ctLoader);
  toto->SetReferenceLoader(*doseLoader);


#if 1
  toto->SetVolume1(0, ctLoader, new OrthancStone::GrayscaleStyleConfigurator);
#else
  {
    boost::shared_ptr<OrthancStone::IVolumeSlicer> reslicer(new OrthancStone::DicomVolumeImageReslicer(ct));
    toto->SetVolume1(0, reslicer, new OrthancStone::GrayscaleStyleConfigurator);
  }
#endif  
  

  {
    std::auto_ptr<OrthancStone::LookupTableStyleConfigurator> config(new OrthancStone::LookupTableStyleConfigurator);
    config->SetLookupTable(Orthanc::EmbeddedResources::COLORMAP_HOT);

    boost::shared_ptr<OrthancStone::DicomVolumeImageMPRSlicer> tmp(new OrthancStone::DicomVolumeImageMPRSlicer(dose));
    toto->SetVolume2(1, tmp, config.release());
  }

  toto->SetStructureSet(2, rtstructLoader);
  
  oracle.Schedule(*toto, new OrthancStone::SleepOracleCommand(100));

  if (0)
  {
    Json::Value v = Json::objectValue;
    v["Level"] = "Series";
    v["Query"] = Json::objectValue;

    std::auto_ptr<OrthancStone::OrthancRestApiCommand>  command(new OrthancStone::OrthancRestApiCommand);
    command->SetMethod(Orthanc::HttpMethod_Post);
    command->SetUri("/tools/find");
    command->SetBody(v);

    oracle.Schedule(*toto, command.release());
  }
  
  if(0)
  {
    if (0)
    {
      std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
      command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Jpeg)));
      command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
      oracle.Schedule(*toto, command.release());
    }
    
    if (0)
    {
      std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
      command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
      command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
      oracle.Schedule(*toto, command.release());
    }
    
    if (0)
    {
      std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
      command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
      command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
      oracle.Schedule(*toto, command.release());
    }
    
    if (0)
    {
      std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
      command->SetHttpHeader("Accept-Encoding", "gzip");
      command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
      command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
      oracle.Schedule(*toto, command.release());
    }
    
    if (0)
    {
      std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
      command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
      command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
      oracle.Schedule(*toto, command.release());
    }

    if (0)
    {
      std::auto_ptr<OrthancStone::GetOrthancWebViewerJpegCommand>  command(new OrthancStone::GetOrthancWebViewerJpegCommand);
      command->SetHttpHeader("Accept-Encoding", "gzip");
      command->SetInstance("e6c7c20b-c9f65d7e-0d76f2e2-830186f2-3e3c600e");
      command->SetQuality(90);
      oracle.Schedule(*toto, command.release());
    }


    if (0)
    {
      for (unsigned int i = 0; i < 10; i++)
      {
        std::auto_ptr<OrthancStone::SleepOracleCommand> command(new OrthancStone::SleepOracleCommand(i * 1000));
        command->SetPayload(new Orthanc::SingleValueObject<unsigned int>(42 * i));
        oracle.Schedule(*toto, command.release());
      }
    }
  }
  
  // 2017-11-17-Anonymized
#if 0
  // BGO data
  ctLoader->LoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");  // CT
  doseLoader->LoadInstance("830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb");  // RT-DOSE
  //rtstructLoader->LoadInstance("54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9");  // RT-STRUCT
#else
  //ctLoader->LoadSeries("cb3ea4d1-d08f3856-ad7b6314-74d88d77-60b05618");  // CT
  //doseLoader->LoadInstance("41029085-71718346-811efac4-420e2c15-d39f99b6");  // RT-DOSE
  //rtstructLoader->LoadInstance("83d9c0c3-913a7fee-610097d7-cbf0522d-fd75bee6");  // RT-STRUCT

  // 2017-05-16
  ctLoader->LoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");  // CT
  doseLoader->LoadInstance("eac822ef-a395f94e-e8121fe0-8411fef8-1f7bffad");  // RT-DOSE
  rtstructLoader->LoadInstance("54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9");  // RT-STRUCT
#endif
  // 2015-01-28-Multiframe
  //doseLoader->LoadInstance("88f71e2a-5fad1c61-96ed14d6-5b3d3cf7-a5825279");  // Multiframe CT
  
  // Delphine
  //ctLoader->LoadSeries("5990e39c-51e5f201-fe87a54c-31a55943-e59ef80e");  // CT
  //ctLoader->LoadSeries("67f1b334-02c16752-45026e40-a5b60b6b-030ecab5");  // Lung 1/10mm


  {
    LOG(WARNING) << "...Waiting for Ctrl-C...";

    oracle.Start();

    Orthanc::SystemToolbox::ServerBarrier();

    /**
     * WARNING => The oracle must be stopped BEFORE the objects using
     * it are destroyed!!! This forces to wait for the completion of
     * the running callback methods. Otherwise, the callbacks methods
     * might still be running while their parent object is destroyed,
     * resulting in crashes. This is very visible if adding a sleep(),
     * as in (*).
     **/

    oracle.Stop();
  }
}



/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  OrthancStone::StoneInitialize();
  //Orthanc::Logging::EnableInfoLevel(true);

  try
  {
    OrthancStone::NativeApplicationContext context;

    OrthancStone::ThreadedOracle oracle(context);
    //oracle.SetThreadsCount(1);

    {
      Orthanc::WebServiceParameters p;
      //p.SetUrl("http://localhost:8043/");
      p.SetCredentials("orthanc", "orthanc");
      oracle.SetOrthancParameters(p);
    }

    //oracle.Start();

    Run(context, oracle);
    
    //oracle.Stop();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::StoneFinalize();

  return 0;
}
