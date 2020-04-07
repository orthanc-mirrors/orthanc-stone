Executable versions
================
Generic options
----------------------
```
("help", "Display this help and exit")
("verbose", "Be verbose in logs")
("orthanc", boost::program_options::value<std::string>()
  ->default_value("http://localhost:8042/"),
  "URL to the Orthanc server")
("username", "Username for the Orthanc server")
("password", "Password for the Orthanc server")
("https-verify", boost::program_options::value<bool>()
  ->default_value(true), "Check HTTPS certificates")
```
OrthancStoneSimpleViewer
-------------------------------------
- Options:
    ```
    - "studyId", std::string, "Orthanc ID of the study"
    ```
- study loading works OK
- Invert does not work:
```
void SimpleViewerApplication::ExecuteAction(SimpleViewerApplication::Actions action)
  {
    // TODO
  }
```

OrthancStoneSimpleViewerSingleFile
-------------------------------------
- Options:
    ```
    - "studyId", std::string, "Orthanc ID of the study"
    ```

Study loading works.

The `line` and `circle` buttons work and call this:
```
virtual void OnTool1Clicked()
{
  currentTool_ = Tools_LineMeasure;
}

virtual void OnTool2Clicked()
{
  currentTool_ = Tools_CircleMeasure;
}
```
The `action1` and `action2` buttons are not connected

The following is displayed in the console at launch time:
```
W0313 12:20:12.790449 NativeStoneApplicationRunner.cpp:55] Use the key "s" to reinitialize the layout
W0313 12:20:12.790449 NativeStoneApplicationRunner.cpp:55] Use the key "n" to go to next image in the main viewport
```
However, when looking at `MainWidgetInteractor::KeyPressed` (`SimpleViewerApplicationSingleFile.h:169`), only the following is processed:
- 's': reset layout
- 'l': select line tool
- 'c': select circle tool

OrthancStoneSingleFrame
-------------------------------------
```
generic.add_options()
("instance", boost::program_options::value<std::string>(), 
"Orthanc ID of the instance")
("frame", boost::program_options::value<unsigned int>()
  ->default_value(0),
"Number of the frame, for multi-frame DICOM instances")
("smooth", boost::program_options::value<bool>()
  ->default_value(true), 
"Enable bilinear interpolation to smooth the image");
```
only key handled in `KeyPressed` is `s` to call `widget.FitContent()`


OrthancStoneSingleFrameEditor
-------------------------------------
```
generic.add_options()
("instance", boost::program_options::value<std::string>(),
"Orthanc ID of the instance")
("frame", boost::program_options::value<unsigned int>()
  ->default_value(0),
"Number of the frame, for multi-frame DICOM instances");
```
Available commands in `KeyPressed` (`SingleFrameEditorApplication.h:280`): 
- 'a' widget.FitContent()
- 'c' Crop tool
- 'm' Mask tool
- 'd' dump to json and diplay result (?)
- 'e' export current view to Dicom with dummy tags (?)
- 'i' wdiget.SwitchInvert
- 't' Move tool
- 'n' switch between nearest and bilinear interpolation
- 'r' Rotate tool
- 's' Resize tool
- 'w' Windowing tool
- 'ctrl+y' redo
- 'ctrl+z' undo
