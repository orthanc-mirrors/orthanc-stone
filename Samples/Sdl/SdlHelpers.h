#pragma once

#if ORTHANC_ENABLE_SDL != 1
# error This file cannot be used if ORTHANC_ENABLE_SDL != 1
#endif

#include <SDL.h>

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
}

