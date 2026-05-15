//
// input.cpp
// by Naomi Peori <naomi@peori.ca>
//

#include "phosphene/input.h"

void Input::init() {
  m_gamepad = nullptr;
}

void Input::handle_event(const SDL_Event &event) {
  if (event.type == SDL_EVENT_GAMEPAD_ADDED && !m_gamepad) {
    m_gamepad = SDL_OpenGamepad(event.gdevice.which);
  }

  if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
    if (m_gamepad && SDL_GetGamepadID(m_gamepad) == event.gdevice.which) {
      SDL_CloseGamepad(m_gamepad);
      m_gamepad = nullptr;
    }
  }
}

InputState Input::read() const {
  const bool *keys = SDL_GetKeyboardState(nullptr);
  InputState s     = {};

  s.up     = keys[SDL_SCANCODE_UP];
  s.down   = keys[SDL_SCANCODE_DOWN];
  s.left   = keys[SDL_SCANCODE_LEFT];
  s.right  = keys[SDL_SCANCODE_RIGHT];
  s.a      = keys[SDL_SCANCODE_Z];
  s.b      = keys[SDL_SCANCODE_X];
  s.select = keys[SDL_SCANCODE_RSHIFT];
  s.start  = keys[SDL_SCANCODE_RETURN];

  if (m_gamepad) {
    s.up |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
    s.down |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    s.left |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    s.right |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    s.a |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
    s.b |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_EAST);
    s.select |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_BACK);
    s.start |= SDL_GetGamepadButton(m_gamepad, SDL_GAMEPAD_BUTTON_START);
  }

  return s;
}

void Input::shutdown() {
  if (m_gamepad) {
    SDL_CloseGamepad(m_gamepad);
  }
  m_gamepad = nullptr;
}
