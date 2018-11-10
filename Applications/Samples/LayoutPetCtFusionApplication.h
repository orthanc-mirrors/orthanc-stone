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

#include "SampleInteractor.h"

#include "../../Framework/Layers/ReferenceLineFactory.h"
#include "../../Framework/Layers/DicomStructureSetSlicer.h"
#include "../../Framework/Widgets/LayoutWidget.h"

#include <Core/Logging.h>

namespace OrthancStone
{
  namespace Samples
  {
    class LayoutPetCtFusionApplication : 
      public SampleApplicationBase,
      public LayeredSceneWidget::ISliceObserver,
      public WorldSceneWidget::IWorldObserver
    {
    private:
      class Interactor : public SampleInteractor
      {
      private:
        LayoutPetCtFusionApplication& that_;

      public:
        Interactor(LayoutPetCtFusionApplication& that,
                   VolumeImage& volume,
                   VolumeProjection projection, 
                   bool reverse) :
          SampleInteractor(volume, projection, reverse),
          that_(that)
        {
        }

        virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                            const SliceGeometry& slice,
                                                            const ViewportGeometry& view,
                                                            MouseButton button,
                                                            double x,
                                                            double y,
                                                            IStatusBar* statusBar)
        {
          if (button == MouseButton_Left)
          {
            // Center the sibling views over the clicked point
            Vector p = slice.MapSliceToWorldCoordinates(x, y);

            if (statusBar != NULL)
            {
              char buf[64];
              sprintf(buf, "Click on coordinates (%.02f,%.02f,%.02f) in cm", p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
              statusBar->SetMessage(buf);
            }

            that_.interactorAxial_->LookupSliceContainingPoint(*that_.ctAxial_, p);
            that_.interactorCoronal_->LookupSliceContainingPoint(*that_.ctCoronal_, p);
            that_.interactorSagittal_->LookupSliceContainingPoint(*that_.ctSagittal_, p);
          }

          return NULL;
        }

        virtual void KeyPressed(WorldSceneWidget& widget,
                                char key,
                                KeyboardModifiers modifiers,
                                IStatusBar* statusBar)
        {
          if (key == 's')
          {
            that_.FitContent();
          }
        }
      };

      bool                 processingEvent_;
      Interactor*          interactorAxial_;
      Interactor*          interactorCoronal_;
      Interactor*          interactorSagittal_;
      LayeredSceneWidget*  ctAxial_;
      LayeredSceneWidget*  ctCoronal_;
      LayeredSceneWidget*  ctSagittal_;
      LayeredSceneWidget*  petAxial_;
      LayeredSceneWidget*  petCoronal_;
      LayeredSceneWidget*  petSagittal_;
      LayeredSceneWidget*  fusionAxial_;
      LayeredSceneWidget*  fusionCoronal_;
      LayeredSceneWidget*  fusionSagittal_;


      void FitContent()
      {
        petAxial_->FitContent();
        petCoronal_->FitContent();
        petSagittal_->FitContent();
      }


      void AddLayer(LayeredSceneWidget& widget,
                    VolumeImage& volume,
                    bool isCt)
      {
        size_t layer;
        widget.AddLayer(layer, new VolumeImage::LayerFactory(volume));

        if (isCt)
        {
          RenderStyle style; 
          style.windowing_ = ImageWindowing_Bone;
          widget.SetLayerStyle(layer, style);
        }
        else
        {
          RenderStyle style; 
          style.applyLut_ = true;
          style.alpha_ = (layer == 0 ? 1.0f : 0.5f);
          widget.SetLayerStyle(layer, style);
        }
      }


      void ConnectSiblingLocations(LayeredSceneWidget& axial,
                                   LayeredSceneWidget& coronal,
                                   LayeredSceneWidget& sagittal)
      {
        ReferenceLineFactory::Configure(axial, coronal);
        ReferenceLineFactory::Configure(axial, sagittal);
        ReferenceLineFactory::Configure(coronal, sagittal);
      }


      void SynchronizeView(const WorldSceneWidget& source,
                           const ViewportGeometry& view,
                           LayeredSceneWidget& widget1,
                           LayeredSceneWidget& widget2,
                           LayeredSceneWidget& widget3)
      {
        if (&source == &widget1 ||
            &source == &widget2 ||
            &source == &widget3)
        {
          if (&source != &widget1)
          {
            widget1.SetView(view);
          }

          if (&source != &widget2)
          {
            widget2.SetView(view);
          }

          if (&source != &widget3)
          {
            widget3.SetView(view);
          }
        }
      }


      void SynchronizeSlice(const LayeredSceneWidget& source,
                            const SliceGeometry& slice,
                            LayeredSceneWidget& widget1,
                            LayeredSceneWidget& widget2,
                            LayeredSceneWidget& widget3)
      {
        if (&source == &widget1 ||
            &source == &widget2 ||
            &source == &widget3)
        {
          if (&source != &widget1)
          {
            widget1.SetSlice(slice);
          }

          if (&source != &widget2)
          {
            widget2.SetSlice(slice);
          }

          if (&source != &widget3)
          {
            widget3.SetSlice(slice);
          }
        }
      }


      LayeredSceneWidget* CreateWidget()
      {
        std::auto_ptr<LayeredSceneWidget> widget(new LayeredSceneWidget);
        widget->Register(dynamic_cast<WorldSceneWidget::IWorldObserver&>(*this));
        widget->Register(dynamic_cast<LayeredSceneWidget::ISliceObserver&>(*this));
        return widget.release();
      }


      void CreateLayout(BasicApplicationContext& context)
      {
        std::auto_ptr<OrthancStone::LayoutWidget> layout(new OrthancStone::LayoutWidget);
        layout->SetBackgroundCleared(true);
        //layout->SetBackgroundColor(255,0,0);
        layout->SetPadding(5);

        OrthancStone::LayoutWidget& layoutA = dynamic_cast<OrthancStone::LayoutWidget&>
          (layout->AddWidget(new OrthancStone::LayoutWidget));
        layoutA.SetPadding(0, 0, 0, 0, 5);
        layoutA.SetVertical();
        petAxial_ = &dynamic_cast<LayeredSceneWidget&>(layoutA.AddWidget(CreateWidget()));
        OrthancStone::LayoutWidget& layoutA2 = dynamic_cast<OrthancStone::LayoutWidget&>
          (layoutA.AddWidget(new OrthancStone::LayoutWidget));
        layoutA2.SetPadding(0, 0, 0, 0, 5);
        petSagittal_ = &dynamic_cast<LayeredSceneWidget&>(layoutA2.AddWidget(CreateWidget()));
        petCoronal_ = &dynamic_cast<LayeredSceneWidget&>(layoutA2.AddWidget(CreateWidget()));

        OrthancStone::LayoutWidget& layoutB = dynamic_cast<OrthancStone::LayoutWidget&>
          (layout->AddWidget(new OrthancStone::LayoutWidget));
        layoutB.SetPadding(0, 0, 0, 0, 5);
        layoutB.SetVertical();
        ctAxial_ = &dynamic_cast<LayeredSceneWidget&>(layoutB.AddWidget(CreateWidget()));
        OrthancStone::LayoutWidget& layoutB2 = dynamic_cast<OrthancStone::LayoutWidget&>
          (layoutB.AddWidget(new OrthancStone::LayoutWidget));
        layoutB2.SetPadding(0, 0, 0, 0, 5);
        ctSagittal_ = &dynamic_cast<LayeredSceneWidget&>(layoutB2.AddWidget(CreateWidget()));
        ctCoronal_ = &dynamic_cast<LayeredSceneWidget&>(layoutB2.AddWidget(CreateWidget()));

        OrthancStone::LayoutWidget& layoutC = dynamic_cast<OrthancStone::LayoutWidget&>
          (layout->AddWidget(new OrthancStone::LayoutWidget));
        layoutC.SetPadding(0, 0, 0, 0, 5);
        layoutC.SetVertical();
        fusionAxial_ = &dynamic_cast<LayeredSceneWidget&>(layoutC.AddWidget(CreateWidget()));
        OrthancStone::LayoutWidget& layoutC2 = dynamic_cast<OrthancStone::LayoutWidget&>
          (layoutC.AddWidget(new OrthancStone::LayoutWidget));
        layoutC2.SetPadding(0, 0, 0, 0, 5);
        fusionSagittal_ = &dynamic_cast<LayeredSceneWidget&>(layoutC2.AddWidget(CreateWidget()));
        fusionCoronal_ = &dynamic_cast<LayeredSceneWidget&>(layoutC2.AddWidget(CreateWidget()));
  
        context.SetCentralWidget(layout.release());
      }


    public:
      virtual void DeclareCommandLineOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("ct", boost::program_options::value<std::string>(), 
           "Orthanc ID of the CT series")
          ("pet", boost::program_options::value<std::string>(), 
           "Orthanc ID of the PET series")
          ("rt", boost::program_options::value<std::string>(), 
           "Orthanc ID of the DICOM RT-STRUCT series (optional)")
          ("threads", boost::program_options::value<unsigned int>()->default_value(3), 
           "Number of download threads for the CT series")
          ;

        options.add(generic);    
      }

      virtual void Initialize(BasicApplicationContext& context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        processingEvent_ = true;

        if (parameters.count("ct") != 1 ||
            parameters.count("pet") != 1)
        {
          LOG(ERROR) << "The series ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string ct = parameters["ct"].as<std::string>();
        std::string pet = parameters["pet"].as<std::string>();
        unsigned int threads = parameters["threads"].as<unsigned int>();

        VolumeImage& ctVolume = context.AddSeriesVolume(ct, true /* progressive download */, threads);
        VolumeImage& petVolume = context.AddSeriesVolume(pet, true /* progressive download */, 1);

        // Take the PET volume as the reference for the slices
        interactorAxial_ = &dynamic_cast<Interactor&>
          (context.AddInteractor(new Interactor(*this, petVolume, VolumeProjection_Axial, false)));
        interactorCoronal_ = &dynamic_cast<Interactor&>
          (context.AddInteractor(new Interactor(*this, petVolume, VolumeProjection_Coronal, false)));
        interactorSagittal_ = &dynamic_cast<Interactor&>
          (context.AddInteractor(new Interactor(*this, petVolume, VolumeProjection_Sagittal, true)));

        CreateLayout(context);

        AddLayer(*ctAxial_, ctVolume, true);
        AddLayer(*ctCoronal_, ctVolume, true);
        AddLayer(*ctSagittal_, ctVolume, true);

        AddLayer(*petAxial_, petVolume, false);
        AddLayer(*petCoronal_, petVolume, false);
        AddLayer(*petSagittal_, petVolume, false);

        AddLayer(*fusionAxial_, ctVolume, true);
        AddLayer(*fusionAxial_, petVolume, false);
        AddLayer(*fusionCoronal_, ctVolume, true);
        AddLayer(*fusionCoronal_, petVolume, false);
        AddLayer(*fusionSagittal_, ctVolume, true);
        AddLayer(*fusionSagittal_, petVolume, false);

        if (parameters.count("rt") == 1)
        {
          DicomStructureSet& rtStruct = context.AddStructureSet(parameters["rt"].as<std::string>());

          Vector p = rtStruct.GetStructureCenter(0);
          interactorAxial_->GetCursor().LookupSliceContainingPoint(p);

          ctAxial_->AddLayer(new DicomStructureSetSlicer(rtStruct));
          petAxial_->AddLayer(new DicomStructureSetSlicer(rtStruct));
          fusionAxial_->AddLayer(new DicomStructureSetSlicer(rtStruct));
        }        

        ConnectSiblingLocations(*ctAxial_, *ctCoronal_, *ctSagittal_); 
        ConnectSiblingLocations(*petAxial_, *petCoronal_, *petSagittal_); 
        ConnectSiblingLocations(*fusionAxial_, *fusionCoronal_, *fusionSagittal_); 

        interactorAxial_->AddWidget(*ctAxial_);
        interactorAxial_->AddWidget(*petAxial_);
        interactorAxial_->AddWidget(*fusionAxial_);
        
        interactorCoronal_->AddWidget(*ctCoronal_);
        interactorCoronal_->AddWidget(*petCoronal_);
        interactorCoronal_->AddWidget(*fusionCoronal_);
        
        interactorSagittal_->AddWidget(*ctSagittal_);
        interactorSagittal_->AddWidget(*petSagittal_);
        interactorSagittal_->AddWidget(*fusionSagittal_);

        processingEvent_ = false;

        statusBar.SetMessage("Use the key \"t\" to toggle the fullscreen mode");
        statusBar.SetMessage("Use the key \"s\" to reinitialize the layout");
      }

      virtual void NotifySizeChange(const WorldSceneWidget& source,
                                    ViewportGeometry& view)
      {
        view.FitContent();
      }

      virtual void NotifyViewChange(const WorldSceneWidget& source,
                                    const ViewportGeometry& view)
      {
        if (!processingEvent_)  // Avoid reentrant calls
        {
          processingEvent_ = true;

          SynchronizeView(source, view, *ctAxial_, *petAxial_, *fusionAxial_);
          SynchronizeView(source, view, *ctCoronal_, *petCoronal_, *fusionCoronal_);
          SynchronizeView(source, view, *ctSagittal_, *petSagittal_, *fusionSagittal_);
          
          processingEvent_ = false;
        }
      }

      virtual void NotifySliceChange(const LayeredSceneWidget& source,
                                     const SliceGeometry& slice)
      {
        if (!processingEvent_)  // Avoid reentrant calls
        {
          processingEvent_ = true;

          SynchronizeSlice(source, slice, *ctAxial_, *petAxial_, *fusionAxial_);
          SynchronizeSlice(source, slice, *ctCoronal_, *petCoronal_, *fusionCoronal_);
          SynchronizeSlice(source, slice, *ctSagittal_, *petSagittal_, *fusionSagittal_);
          
          processingEvent_ = false;
        }
      }
    };
  }
}
