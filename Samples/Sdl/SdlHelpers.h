#pragma once

#include <map>
#include <string>

namespace OrthancStoneHelpers
{
  inline OrthancStone::KeyboardModifiers GetKeyboardModifiers(const uint8_t* keyboardState,
                                                              const int scancodeCount)
  {
    using namespace OrthancStone;
    int result = KeyboardModifiers_None;

    if (keyboardState != NULL)
    {
      if (SDL_SCANCODE_LSHIFT < scancodeCount &&
          keyboardState[SDL_SCANCODE_LSHIFT])
      {
        result |= KeyboardModifiers_Shift;
      }

      if (SDL_SCANCODE_RSHIFT < scancodeCount &&
          keyboardState[SDL_SCANCODE_RSHIFT])
      {
        result |= KeyboardModifiers_Shift;
      }

      if (SDL_SCANCODE_LCTRL < scancodeCount &&
          keyboardState[SDL_SCANCODE_LCTRL])
      {
        result |= KeyboardModifiers_Control;
      }

      if (SDL_SCANCODE_RCTRL < scancodeCount &&
          keyboardState[SDL_SCANCODE_RCTRL])
      {
        result |= KeyboardModifiers_Control;
      }

      if (SDL_SCANCODE_LALT < scancodeCount &&
          keyboardState[SDL_SCANCODE_LALT])
      {
        result |= KeyboardModifiers_Alt;
      }

      if (SDL_SCANCODE_RALT < scancodeCount &&
          keyboardState[SDL_SCANCODE_RALT])
      {
        result |= KeyboardModifiers_Alt;
      }
    }

    return static_cast<KeyboardModifiers>(result);
  }


  inline void GetPointerEvent(OrthancStone::PointerEvent& p,
                              const OrthancStone::ICompositor& compositor,
                              SDL_Event event,
                              const uint8_t* keyboardState,
                              const int scancodeCount)
  {
    using namespace OrthancStone;
    KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

    switch (event.button.button)
    {
    case SDL_BUTTON_LEFT:
      p.SetMouseButton(OrthancStone::MouseButton_Left);
      break;

    case SDL_BUTTON_RIGHT:
      p.SetMouseButton(OrthancStone::MouseButton_Right);
      break;

    case SDL_BUTTON_MIDDLE:
      p.SetMouseButton(OrthancStone::MouseButton_Middle);
      break;

    default:
      p.SetMouseButton(OrthancStone::MouseButton_None);
      break;
    }

    p.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));
    p.SetAltModifier(modifiers & KeyboardModifiers_Alt);
    p.SetControlModifier(modifiers & KeyboardModifiers_Control);
    p.SetShiftModifier(modifiers & KeyboardModifiers_Shift);
  }

  //inline void ParseCommonCommandLineArguments()
  //{

  //  po::options_description description("Usage:");

  //  description.add_options()
  //    ("trace", "Enable TRACE logging mode (default: false)")
  //    ("info", "Enable INFO logging mode (default: false)")
  //    ("orthanc", po::value<string>()->default_value("http://localhost:8042"), "Base URL of the Orthanc instance")

  //  po::variables_map vm;
  //  po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
  //  po::notify(vm);

  //  if (vm.count("help"))
  //  {

  //  }

  //  if (vm.count("compression"))
  //  {
  //    cout << "Compression level " << vm["compression"].as<int>() << endl;
  //  }

  //  return 0;

  //}

  //inline void DeclareOptionAndDefault(boost::program_options::options_description& options,
  //                             const std::string& flag, const std::string& defaultValue,
  //                             std::string help = "")
  //{
  //  description.add_options()
  //    (flag, po::value<string>()->default_value(defaultValue), help);
  //}

  //inline void ProcessOptions

}

