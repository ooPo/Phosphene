//
// input.h
// by Naomi Peori <naomi@peori.ca>
//

#pragma once
#include <SDL3/SDL.h>

/// @brief Holds the current state of gamepad buttons.
struct InputState {
  bool up, down, left, right; ///< D-pad directions
  bool a, b, select, start;   ///< Face buttons
};

/// @brief Manages gamepad input with keyboard fallback.
///
/// Input processes SDL events to detect connected gamepads and tracks
/// their button state. Keyboard keys (arrow keys, ZXRS) are mapped as
/// a fallback if no gamepad is connected.
class Input {
public:
  /// Initialize input system.
  void init();

  /// Process an SDL event (typically called in the main event loop).
  void handle_event(const SDL_Event &event);

  /// Get current input state (buttons that are pressed).
  InputState read() const;

  /// Clean up gamepad resources.
  void shutdown();

private:
  SDL_Gamepad *m_gamepad = nullptr;
};
