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

#include "SampleApplicationBase.h"

#include "../../Framework/Radiography/RadiographyLayerCropTracker.h"
#include "../../Framework/Radiography/RadiographyLayerMaskTracker.h"
#include "../../Framework/Radiography/RadiographyLayerMoveTracker.h"
#include "../../Framework/Radiography/RadiographyLayerResizeTracker.h"
#include "../../Framework/Radiography/RadiographyLayerRotateTracker.h"
#include "../../Framework/Radiography/RadiographyMaskLayer.h"
#include "../../Framework/Radiography/RadiographyScene.h"
#include "../../Framework/Radiography/RadiographySceneCommand.h"
#include "../../Framework/Radiography/RadiographySceneReader.h"
#include "../../Framework/Radiography/RadiographySceneWriter.h"
#include "../../Framework/Radiography/RadiographyWidget.h"
#include "../../Framework/Radiography/RadiographyWindowingTracker.h"
#include "../../Framework/Toolbox/TextRenderer.h"

#include <Core/HttpClient.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/PngWriter.h>
#include <Core/Images/PngReader.h>


// Export using PAM is faster than using PNG, but requires Orthanc
// core >= 1.4.3
#define EXPORT_USING_PAM  1


namespace OrthancStone
{
  namespace Samples
  {
    class RadiographyEditorInteractor :
        public Deprecated::IWorldSceneInteractor,
        public ObserverBase<RadiographyEditorInteractor>
    {
    private:
      enum Tool
      {
        Tool_Move,
        Tool_Rotate,
        Tool_Crop,
        Tool_Resize,
        Tool_Mask,
        Tool_Windowing
      };


      StoneApplicationContext*  context_;
      UndoRedoStack             undoRedoStack_;
      Tool                      tool_;
      RadiographyMaskLayer*     maskLayer_;


      static double GetHandleSize()
      {
        return 10.0;
      }


    public:
      RadiographyEditorInteractor() :
        context_(NULL),
        tool_(Tool_Move),
        maskLayer_(NULL)
      {
      }

      void SetContext(StoneApplicationContext& context)
      {
        context_ = &context;
      }

      void SetMaskLayer(RadiographyMaskLayer* maskLayer)
      {
        maskLayer_ = maskLayer;
      }
      virtual Deprecated::IWorldSceneMouseTracker* CreateMouseTracker(Deprecated::WorldSceneWidget& worldWidget,
                                                                      const Deprecated::ViewportGeometry& view,
                                                                      MouseButton button,
                                                                      KeyboardModifiers modifiers,
                                                                      int viewportX,
                                                                      int viewportY,
                                                                      double x,
                                                                      double y,
                                                                      Deprecated::IStatusBar* statusBar,
                                                                      const std::vector<Deprecated::Touch>& displayTouches)
      {
        RadiographyWidget& widget = dynamic_cast<RadiographyWidget&>(worldWidget);

        if (button == MouseButton_Left)
        {
          size_t selected;

          if (tool_ == Tool_Windowing)
          {
            return new RadiographyWindowingTracker(
                  undoRedoStack_,
                  widget.GetScene(),
                  widget,
                  OrthancStone::ImageInterpolation_Nearest,
                  viewportX, viewportY,
                  RadiographyWindowingTracker::Action_DecreaseWidth,
                  RadiographyWindowingTracker::Action_IncreaseWidth,
                  RadiographyWindowingTracker::Action_DecreaseCenter,
                  RadiographyWindowingTracker::Action_IncreaseCenter);
          }
          else if (!widget.LookupSelectedLayer(selected))
          {
            // No layer is currently selected
            size_t layer;
            if (widget.GetScene().LookupLayer(layer, x, y))
            {
              widget.Select(layer);
            }

            return NULL;
          }
          else if (tool_ == Tool_Crop ||
                   tool_ == Tool_Resize ||
                   tool_ == Tool_Mask)
          {
            RadiographyScene::LayerAccessor accessor(widget.GetScene(), selected);
            
            ControlPoint controlPoint;
            if (accessor.GetLayer().LookupControlPoint(controlPoint, x, y, view.GetZoom(), GetHandleSize()))
            {
              switch (tool_)
              {
              case Tool_Crop:
                return new RadiographyLayerCropTracker
                    (undoRedoStack_, widget.GetScene(), view, selected, controlPoint);

              case Tool_Mask:
                return new RadiographyLayerMaskTracker
                    (undoRedoStack_, widget.GetScene(), view, selected, controlPoint);

              case Tool_Resize:
                return new RadiographyLayerResizeTracker
                    (undoRedoStack_, widget.GetScene(), selected, controlPoint,
                     (modifiers & KeyboardModifiers_Shift));

              default:
                throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
              }
            }
            else
            {
              size_t layer;

              if (widget.GetScene().LookupLayer(layer, x, y))
              {
                widget.Select(layer);
              }
              else
              {
                widget.Unselect();
              }

              return NULL;
            }
          }
          else
          {
            size_t layer;

            if (widget.GetScene().LookupLayer(layer, x, y))
            {
              if (layer == selected)
              {
                switch (tool_)
                {
                case Tool_Move:
                  return new RadiographyLayerMoveTracker
                      (undoRedoStack_, widget.GetScene(), layer, x, y,
                       (modifiers & KeyboardModifiers_Shift));

                case Tool_Rotate:
                  return new RadiographyLayerRotateTracker
                      (undoRedoStack_, widget.GetScene(), view, layer, x, y,
                       (modifiers & KeyboardModifiers_Shift));

                default:
                  break;
                }

                return NULL;
              }
              else
              {
                widget.Select(layer);
                return NULL;
              }
            }
            else
            {
              widget.Unselect();
              return NULL;
            }
          }
        }
        else
        {
          return NULL;
        }
        return NULL;
      }

      virtual void MouseOver(CairoContext& context,
                             Deprecated::WorldSceneWidget& worldWidget,
                             const Deprecated::ViewportGeometry& view,
                             double x,
                             double y,
                             Deprecated::IStatusBar* statusBar)
      {
        RadiographyWidget& widget = dynamic_cast<RadiographyWidget&>(worldWidget);

#if 0
        if (statusBar != NULL)
        {
          char buf[64];
          sprintf(buf, "X = %.02f Y = %.02f (in cm)", x / 10.0, y / 10.0);
          statusBar->SetMessage(buf);
        }
#endif

        size_t selected;

        if (widget.LookupSelectedLayer(selected) &&
            (tool_ == Tool_Crop ||
             tool_ == Tool_Resize ||
             tool_ == Tool_Mask))
        {
          RadiographyScene::LayerAccessor accessor(widget.GetScene(), selected);

          ControlPoint controlPoint;
          if (accessor.GetLayer().LookupControlPoint(controlPoint, x, y, view.GetZoom(), GetHandleSize()))
          {
            double z = 1.0 / view.GetZoom();

            context.SetSourceColor(255, 0, 0);
            cairo_t* cr = context.GetObject();
            cairo_set_line_width(cr, 2.0 * z);
            cairo_move_to(cr, controlPoint.x - GetHandleSize() * z, controlPoint.y - GetHandleSize() * z);
            cairo_line_to(cr, controlPoint.x + GetHandleSize() * z, controlPoint.y - GetHandleSize() * z);
            cairo_line_to(cr, controlPoint.x + GetHandleSize() * z, controlPoint.y + GetHandleSize() * z);
            cairo_line_to(cr, controlPoint.x - GetHandleSize() * z, controlPoint.y + GetHandleSize() * z);
            cairo_line_to(cr, controlPoint.x - GetHandleSize() * z, controlPoint.y - GetHandleSize() * z);
            cairo_stroke(cr);
          }
        }
      }

      virtual void MouseWheel(Deprecated::WorldSceneWidget& widget,
                              MouseWheelDirection direction,
                              KeyboardModifiers modifiers,
                              Deprecated::IStatusBar* statusBar)
      {
      }

      virtual void KeyPressed(Deprecated::WorldSceneWidget& worldWidget,
                              KeyboardKeys key,
                              char keyChar,
                              KeyboardModifiers modifiers,
                              Deprecated::IStatusBar* statusBar)
      {
        RadiographyWidget& widget = dynamic_cast<RadiographyWidget&>(worldWidget);

        switch (keyChar)
        {
        case 'a':
          widget.FitContent();
          break;

        case 'c':
          tool_ = Tool_Crop;
          break;

        case 'm':
          tool_ = Tool_Mask;
          widget.Select(1);
          break;

        case 'd':
        {
          // dump to json and reload
          Json::Value snapshot;
          RadiographySceneWriter writer;
          writer.Write(snapshot, widget.GetScene());

          LOG(INFO) << "JSON export was successful: "
                    << snapshot.toStyledString();

          boost::shared_ptr<RadiographyScene> scene(new RadiographyScene);
          RadiographySceneReader reader(*scene, *context_->GetOrthancApiClient());
          reader.Read(snapshot);

          widget.SetScene(scene);
        };break;

        case 'e':
        {
          Orthanc::DicomMap tags;

          // Minimal set of tags to generate a valid CR image
          tags.SetValue(Orthanc::DICOM_TAG_ACCESSION_NUMBER, "NOPE", false);
          tags.SetValue(Orthanc::DICOM_TAG_BODY_PART_EXAMINED, "PELVIS", false);
          tags.SetValue(Orthanc::DICOM_TAG_INSTANCE_NUMBER, "1", false);
          //tags.SetValue(Orthanc::DICOM_TAG_LATERALITY, "", false);
          tags.SetValue(Orthanc::DICOM_TAG_MANUFACTURER, "OSIMIS", false);
          tags.SetValue(Orthanc::DICOM_TAG_MODALITY, "CR", false);
          tags.SetValue(Orthanc::DICOM_TAG_PATIENT_BIRTH_DATE, "20000101", false);
          tags.SetValue(Orthanc::DICOM_TAG_PATIENT_ID, "hello", false);
          tags.SetValue(Orthanc::DICOM_TAG_PATIENT_NAME, "HELLO^WORLD", false);
          tags.SetValue(Orthanc::DICOM_TAG_PATIENT_ORIENTATION, "", false);
          tags.SetValue(Orthanc::DICOM_TAG_PATIENT_SEX, "M", false);
          tags.SetValue(Orthanc::DICOM_TAG_REFERRING_PHYSICIAN_NAME, "HOUSE^MD", false);
          tags.SetValue(Orthanc::DICOM_TAG_SERIES_NUMBER, "1", false);
          tags.SetValue(Orthanc::DICOM_TAG_SOP_CLASS_UID, "1.2.840.10008.5.1.4.1.1.1", false);
          tags.SetValue(Orthanc::DICOM_TAG_STUDY_ID, "STUDY", false);
          tags.SetValue(Orthanc::DICOM_TAG_VIEW_POSITION, "", false);

          if (context_ != NULL)
          {
            widget.GetScene().ExportDicom(*context_->GetOrthancApiClient(),
                                          tags, std::string(), 0.1, 0.1, widget.IsInverted(),
                                          widget.GetInterpolation(), EXPORT_USING_PAM);
          }

          break;
        }

        case 'i':
          widget.SwitchInvert();
          break;

        case 't':
          tool_ = Tool_Move;
          break;

        case 'n':
        {
          switch (widget.GetInterpolation())
          {
          case ImageInterpolation_Nearest:
            LOG(INFO) << "Switching to bilinear interpolation";
            widget.SetInterpolation(ImageInterpolation_Bilinear);
            break;

          case ImageInterpolation_Bilinear:
            LOG(INFO) << "Switching to nearest neighbor interpolation";
            widget.SetInterpolation(ImageInterpolation_Nearest);
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }
          
          break;
        }

        case 'r':
          tool_ = Tool_Rotate;
          break;

        case 's':
          tool_ = Tool_Resize;
          break;

        case 'w':
          tool_ = Tool_Windowing;
          break;

        case 'y':
          if (modifiers & KeyboardModifiers_Control)
          {
            undoRedoStack_.Redo();
            widget.NotifyContentChanged();
          }
          break;

        case 'z':
          if (modifiers & KeyboardModifiers_Control)
          {
            undoRedoStack_.Undo();
            widget.NotifyContentChanged();
          }
          break;

        default:
          break;
        }
      }
    };



    class SingleFrameEditorApplication :
        public SampleSingleCanvasApplicationBase,
        public IObserver
    {
    private:
      boost::shared_ptr<RadiographyScene>   scene_;
      RadiographyEditorInteractor           interactor_;
      RadiographyMaskLayer*                 maskLayer_;

    public:
      virtual ~SingleFrameEditorApplication()
      {
        LOG(WARNING) << "Destroying the application";
      }
      
      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
            ("instance", boost::program_options::value<std::string>(),
             "Orthanc ID of the instance")
            ("frame", boost::program_options::value<unsigned int>()->default_value(0),
             "Number of the frame, for multi-frame DICOM instances")
            ;

        options.add(generic);
      }

      virtual void Initialize(StoneApplicationContext* context,
                              Deprecated::IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        context_ = context;
        interactor_.SetContext(*context);

        statusBar.SetMessage("Use the key \"a\" to reinitialize the layout");
        statusBar.SetMessage("Use the key \"c\" to crop");
        statusBar.SetMessage("Use the key \"e\" to export DICOM to the Orthanc server");
        statusBar.SetMessage("Use the key \"f\" to switch full screen");
        statusBar.SetMessage("Use the key \"i\" to invert contrast");
        statusBar.SetMessage("Use the key \"m\" to modify the mask");
        statusBar.SetMessage("Use the key \"n\" to switch between nearest neighbor and bilinear interpolation");
        statusBar.SetMessage("Use the key \"r\" to rotate objects");
        statusBar.SetMessage("Use the key \"s\" to resize objects (not applicable to DICOM layers)");
        statusBar.SetMessage("Use the key \"t\" to move (translate) objects");
        statusBar.SetMessage("Use the key \"w\" to change windowing");
        
        statusBar.SetMessage("Use the key \"ctrl-z\" to undo action");
        statusBar.SetMessage("Use the key \"ctrl-y\" to redo action");

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        //int frame = parameters["frame"].as<unsigned int>();

        scene_.reset(new RadiographyScene);
        
        RadiographyLayer& dicomLayer = scene_->LoadDicomFrame(*context->GetOrthancApiClient(), instance, 0, false, NULL);
        //scene_->LoadDicomFrame(instance, frame, false); //.SetPan(200, 0);
        // = scene_->LoadDicomFrame(context->GetOrthancApiClient(), "61f3143e-96f34791-ad6bbb8d-62559e75-45943e1b", 0, false, NULL);

#if !defined(ORTHANC_ENABLE_WASM) || ORTHANC_ENABLE_WASM != 1
        Orthanc::HttpClient::ConfigureSsl(true, "/etc/ssl/certs/ca-certificates.crt");
#endif
        
        //scene_->LoadDicomWebFrame(context->GetWebService());
        
        std::vector<Orthanc::ImageProcessing::ImagePoint> mask;
        mask.push_back(Orthanc::ImageProcessing::ImagePoint(1100, 100));
        mask.push_back(Orthanc::ImageProcessing::ImagePoint(1100, 1000));
        mask.push_back(Orthanc::ImageProcessing::ImagePoint(2000, 1000));
        mask.push_back(Orthanc::ImageProcessing::ImagePoint(2200, 150));
        mask.push_back(Orthanc::ImageProcessing::ImagePoint(1500, 550));
        maskLayer_ = dynamic_cast<RadiographyMaskLayer*>(&(scene_->LoadMask(mask, dynamic_cast<RadiographyDicomLayer&>(dicomLayer), 128.0f, NULL)));
        interactor_.SetMaskLayer(maskLayer_);

        {
          std::auto_ptr<Orthanc::ImageAccessor> renderedTextAlpha(TextRenderer::Render(Orthanc::EmbeddedResources::UBUNTU_FONT, 100,
                                                                                    "%öÇaA&#"));
          RadiographyLayer& layer = scene_->LoadAlphaBitmap(renderedTextAlpha.release(), NULL);
          dynamic_cast<RadiographyAlphaLayer&>(layer).SetForegroundValue(200.0f * 256.0f);
        }

        {
          RadiographyTextLayer::RegisterFont("ubuntu", Orthanc::EmbeddedResources::UBUNTU_FONT);
          RadiographyLayer& layer = scene_->LoadText("Hello\nworld", "ubuntu", 20, 128, NULL, false);
          layer.SetResizeable(true);
        }
        
        {
          RadiographyLayer& layer = scene_->LoadTestBlock(100, 50, NULL);
          layer.SetResizeable(true);
          layer.SetPan(0, 200);
        }
        
        boost::shared_ptr<RadiographyWidget> widget(new RadiographyWidget(scene_, "main-widget"));
        widget->SetTransmitMouseOver(true);
        widget->SetInteractor(interactor_);
        SetCentralWidget(widget);

        //scene_->SetWindowing(128, 256);
      }
    };
  }
}
