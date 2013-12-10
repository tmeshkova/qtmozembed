/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <Qt>

#include "EmbedQtKeyUtils.h"

#include "nsIDOMKeyEvent.h"
#include "nsIDOMWindowUtils.h"

using namespace mozilla;

struct nsKeyConverter
{
    int vkCode; // Platform independent key code
    int keysym; // Qt key code
};

static struct nsKeyConverter nsKeycodes[] =
{
//  { nsIDOMKeyEvent::DOM_VK_CANCEL,        Qt::Key_Cancel },
    { nsIDOMKeyEvent::DOM_VK_BACK_SPACE,    Qt::Key_Backspace },
    { nsIDOMKeyEvent::DOM_VK_TAB,           Qt::Key_Tab },
    { nsIDOMKeyEvent::DOM_VK_TAB,           Qt::Key_Backtab },
//  { nsIDOMKeyEvent::DOM_VK_CLEAR,         Qt::Key_Clear },
    { nsIDOMKeyEvent::DOM_VK_RETURN,        Qt::Key_Return },
    { nsIDOMKeyEvent::DOM_VK_RETURN,        Qt::Key_Enter },
    { nsIDOMKeyEvent::DOM_VK_SHIFT,         Qt::Key_Shift },
    { nsIDOMKeyEvent::DOM_VK_CONTROL,       Qt::Key_Control },
    { nsIDOMKeyEvent::DOM_VK_ALT,           Qt::Key_Alt },
    { nsIDOMKeyEvent::DOM_VK_PAUSE,         Qt::Key_Pause },
    { nsIDOMKeyEvent::DOM_VK_CAPS_LOCK,     Qt::Key_CapsLock },
    { nsIDOMKeyEvent::DOM_VK_ESCAPE,        Qt::Key_Escape },
    { nsIDOMKeyEvent::DOM_VK_SPACE,         Qt::Key_Space },
    { nsIDOMKeyEvent::DOM_VK_PAGE_UP,       Qt::Key_PageUp },
    { nsIDOMKeyEvent::DOM_VK_PAGE_DOWN,     Qt::Key_PageDown },
    { nsIDOMKeyEvent::DOM_VK_END,           Qt::Key_End },
    { nsIDOMKeyEvent::DOM_VK_HOME,          Qt::Key_Home },
    { nsIDOMKeyEvent::DOM_VK_LEFT,          Qt::Key_Left },
    { nsIDOMKeyEvent::DOM_VK_UP,            Qt::Key_Up },
    { nsIDOMKeyEvent::DOM_VK_RIGHT,         Qt::Key_Right },
    { nsIDOMKeyEvent::DOM_VK_DOWN,          Qt::Key_Down },
    { nsIDOMKeyEvent::DOM_VK_PRINTSCREEN,   Qt::Key_Print },
    { nsIDOMKeyEvent::DOM_VK_INSERT,        Qt::Key_Insert },
    { nsIDOMKeyEvent::DOM_VK_DELETE,        Qt::Key_Delete },
    { nsIDOMKeyEvent::DOM_VK_HELP,          Qt::Key_Help },

    { nsIDOMKeyEvent::DOM_VK_0,             Qt::Key_0 },
    { nsIDOMKeyEvent::DOM_VK_1,             Qt::Key_1 },
    { nsIDOMKeyEvent::DOM_VK_2,             Qt::Key_2 },
    { nsIDOMKeyEvent::DOM_VK_3,             Qt::Key_3 },
    { nsIDOMKeyEvent::DOM_VK_4,             Qt::Key_4 },
    { nsIDOMKeyEvent::DOM_VK_5,             Qt::Key_5 },
    { nsIDOMKeyEvent::DOM_VK_6,             Qt::Key_6 },
    { nsIDOMKeyEvent::DOM_VK_7,             Qt::Key_7 },
    { nsIDOMKeyEvent::DOM_VK_8,             Qt::Key_8 },
    { nsIDOMKeyEvent::DOM_VK_9,             Qt::Key_9 },

    { nsIDOMKeyEvent::DOM_VK_SEMICOLON,     Qt::Key_Semicolon },
    { nsIDOMKeyEvent::DOM_VK_EQUALS,        Qt::Key_Equal },

    { nsIDOMKeyEvent::DOM_VK_A,             Qt::Key_A },
    { nsIDOMKeyEvent::DOM_VK_B,             Qt::Key_B },
    { nsIDOMKeyEvent::DOM_VK_C,             Qt::Key_C },
    { nsIDOMKeyEvent::DOM_VK_D,             Qt::Key_D },
    { nsIDOMKeyEvent::DOM_VK_E,             Qt::Key_E },
    { nsIDOMKeyEvent::DOM_VK_F,             Qt::Key_F },
    { nsIDOMKeyEvent::DOM_VK_G,             Qt::Key_G },
    { nsIDOMKeyEvent::DOM_VK_H,             Qt::Key_H },
    { nsIDOMKeyEvent::DOM_VK_I,             Qt::Key_I },
    { nsIDOMKeyEvent::DOM_VK_J,             Qt::Key_J },
    { nsIDOMKeyEvent::DOM_VK_K,             Qt::Key_K },
    { nsIDOMKeyEvent::DOM_VK_L,             Qt::Key_L },
    { nsIDOMKeyEvent::DOM_VK_M,             Qt::Key_M },
    { nsIDOMKeyEvent::DOM_VK_N,             Qt::Key_N },
    { nsIDOMKeyEvent::DOM_VK_O,             Qt::Key_O },
    { nsIDOMKeyEvent::DOM_VK_P,             Qt::Key_P },
    { nsIDOMKeyEvent::DOM_VK_Q,             Qt::Key_Q },
    { nsIDOMKeyEvent::DOM_VK_R,             Qt::Key_R },
    { nsIDOMKeyEvent::DOM_VK_S,             Qt::Key_S },
    { nsIDOMKeyEvent::DOM_VK_T,             Qt::Key_T },
    { nsIDOMKeyEvent::DOM_VK_U,             Qt::Key_U },
    { nsIDOMKeyEvent::DOM_VK_V,             Qt::Key_V },
    { nsIDOMKeyEvent::DOM_VK_W,             Qt::Key_W },
    { nsIDOMKeyEvent::DOM_VK_X,             Qt::Key_X },
    { nsIDOMKeyEvent::DOM_VK_Y,             Qt::Key_Y },
    { nsIDOMKeyEvent::DOM_VK_Z,             Qt::Key_Z },

    { nsIDOMKeyEvent::DOM_VK_NUMPAD0,       Qt::Key_0 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD1,       Qt::Key_1 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD2,       Qt::Key_2 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD3,       Qt::Key_3 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD4,       Qt::Key_4 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD5,       Qt::Key_5 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD6,       Qt::Key_6 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD7,       Qt::Key_7 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD8,       Qt::Key_8 },
    { nsIDOMKeyEvent::DOM_VK_NUMPAD9,       Qt::Key_9 },
    { nsIDOMKeyEvent::DOM_VK_MULTIPLY,      Qt::Key_Asterisk },
    { nsIDOMKeyEvent::DOM_VK_ADD,           Qt::Key_Plus },
//  { nsIDOMKeyEvent::DOM_VK_SEPARATOR,     Qt::Key_Separator },
    { nsIDOMKeyEvent::DOM_VK_SUBTRACT,      Qt::Key_Minus },
    { nsIDOMKeyEvent::DOM_VK_DECIMAL,       Qt::Key_Period },
    { nsIDOMKeyEvent::DOM_VK_DIVIDE,        Qt::Key_Slash },
    { nsIDOMKeyEvent::DOM_VK_F1,            Qt::Key_F1 },
    { nsIDOMKeyEvent::DOM_VK_F2,            Qt::Key_F2 },
    { nsIDOMKeyEvent::DOM_VK_F3,            Qt::Key_F3 },
    { nsIDOMKeyEvent::DOM_VK_F4,            Qt::Key_F4 },
    { nsIDOMKeyEvent::DOM_VK_F5,            Qt::Key_F5 },
    { nsIDOMKeyEvent::DOM_VK_F6,            Qt::Key_F6 },
    { nsIDOMKeyEvent::DOM_VK_F7,            Qt::Key_F7 },
    { nsIDOMKeyEvent::DOM_VK_F8,            Qt::Key_F8 },
    { nsIDOMKeyEvent::DOM_VK_F9,            Qt::Key_F9 },
    { nsIDOMKeyEvent::DOM_VK_F10,           Qt::Key_F10 },
    { nsIDOMKeyEvent::DOM_VK_F11,           Qt::Key_F11 },
    { nsIDOMKeyEvent::DOM_VK_F12,           Qt::Key_F12 },
    { nsIDOMKeyEvent::DOM_VK_F13,           Qt::Key_F13 },
    { nsIDOMKeyEvent::DOM_VK_F14,           Qt::Key_F14 },
    { nsIDOMKeyEvent::DOM_VK_F15,           Qt::Key_F15 },
    { nsIDOMKeyEvent::DOM_VK_F16,           Qt::Key_F16 },
    { nsIDOMKeyEvent::DOM_VK_F17,           Qt::Key_F17 },
    { nsIDOMKeyEvent::DOM_VK_F18,           Qt::Key_F18 },
    { nsIDOMKeyEvent::DOM_VK_F19,           Qt::Key_F19 },
    { nsIDOMKeyEvent::DOM_VK_F20,           Qt::Key_F20 },
    { nsIDOMKeyEvent::DOM_VK_F21,           Qt::Key_F21 },
    { nsIDOMKeyEvent::DOM_VK_F22,           Qt::Key_F22 },
    { nsIDOMKeyEvent::DOM_VK_F23,           Qt::Key_F23 },
    { nsIDOMKeyEvent::DOM_VK_F24,           Qt::Key_F24 },

    { nsIDOMKeyEvent::DOM_VK_NUM_LOCK,      Qt::Key_NumLock },
    { nsIDOMKeyEvent::DOM_VK_SCROLL_LOCK,   Qt::Key_ScrollLock },

    { nsIDOMKeyEvent::DOM_VK_COMMA,         Qt::Key_Comma },
    { nsIDOMKeyEvent::DOM_VK_PERIOD,        Qt::Key_Period },
    { nsIDOMKeyEvent::DOM_VK_SLASH,         Qt::Key_Slash },
    { nsIDOMKeyEvent::DOM_VK_BACK_QUOTE,    Qt::Key_QuoteLeft },
    { nsIDOMKeyEvent::DOM_VK_OPEN_BRACKET,  Qt::Key_ParenLeft },
    { nsIDOMKeyEvent::DOM_VK_CLOSE_BRACKET, Qt::Key_ParenRight },
    { nsIDOMKeyEvent::DOM_VK_QUOTE,         Qt::Key_QuoteDbl },

    { nsIDOMKeyEvent::DOM_VK_META,          Qt::Key_Meta }
};

int
MozKey::QtKeyCodeToDOMKeyCode(int aKeysym, int aModifier)
{
    unsigned int i;

    // First, try to handle alphanumeric input, not listed in nsKeycodes:
    // most likely, more letters will be getting typed in than things in
    // the key list, so we will look through these first.

    // since X has different key symbols for upper and lowercase letters and
    // mozilla does not, convert gdk's to mozilla's
    if (aKeysym >= Qt::Key_A && aKeysym <= Qt::Key_Z)
        return aKeysym - Qt::Key_A + nsIDOMKeyEvent::DOM_VK_A;

    // numbers
    if (aKeysym >= Qt::Key_0 && aKeysym <= Qt::Key_9)
        return aKeysym - Qt::Key_0 + nsIDOMKeyEvent::DOM_VK_0;

    // keypad numbers
    if (aKeysym >= Qt::Key_0 && aKeysym <= Qt::Key_9 && aModifier & Qt::KeypadModifier)
        return aKeysym - Qt::Key_0 + nsIDOMKeyEvent::DOM_VK_NUMPAD0;

    // misc other things
    for (i = 0; i < ArrayLength(nsKeycodes); i++) {
        if (nsKeycodes[i].keysym == aKeysym)
            return(nsKeycodes[i].vkCode);
    }

    // function keys
    if (aKeysym >= Qt::Key_F1 && aKeysym <= Qt::Key_F24)
        return aKeysym - Qt::Key_F1 + nsIDOMKeyEvent::DOM_VK_F1;

    return((int)0);
}

int
MozKey::DOMKeyCodeToQtKeyCode(int aKeysym)
{
    unsigned int i;

    // First, try to handle alphanumeric input, not listed in nsKeycodes:
    // most likely, more letters will be getting typed in than things in
    // the key list, so we will look through these first.

    if (aKeysym >= nsIDOMKeyEvent::DOM_VK_A && aKeysym <= nsIDOMKeyEvent::DOM_VK_Z)
      // gdk and DOM both use the ASCII codes for these keys.
      return aKeysym;

    // numbers
    if (aKeysym >= nsIDOMKeyEvent::DOM_VK_0 && aKeysym <= nsIDOMKeyEvent::DOM_VK_9)
      // gdk and DOM both use the ASCII codes for these keys.
      return aKeysym - Qt::Key_0 + nsIDOMKeyEvent::DOM_VK_0;

    // keypad numbers
    if (aKeysym >= nsIDOMKeyEvent::DOM_VK_NUMPAD0 && aKeysym <= nsIDOMKeyEvent::DOM_VK_NUMPAD9) {
      NS_ERROR("keypad numbers conversion not implemented");
      //return aKeysym - nsIDOMKeyEvent::DOM_VK_NUMPAD0 + Qt::Key_KP_0;
      return 0;
    }

    // misc other things
    for (i = 0; i < ArrayLength(nsKeycodes); ++i) {
      if (nsKeycodes[i].vkCode == aKeysym) {
        return nsKeycodes[i].keysym;
      }
    }

    // function keys
    if (aKeysym >= nsIDOMKeyEvent::DOM_VK_F1 && aKeysym <= nsIDOMKeyEvent::DOM_VK_F9)
      return aKeysym - nsIDOMKeyEvent::DOM_VK_F1 + Qt::Key_F1;

    return 0;
}

uint32_t*
MozKey::GetFlagWord32(uint32_t aKeyCode, uint32_t* aMask)
{
  /* Mozilla DOM Virtual Key Code is from 0 to 224. */
  // NS_ASSERTION((aKeyCode <= 0xFF), "Invalid DOM Key Code");
  static uint32_t mKeyDownFlags[8];
  memset(mKeyDownFlags, 0, sizeof(mKeyDownFlags));
  aKeyCode &= 0xFF;

  /* 32 = 2^5 = 0x20 */
  *aMask = uint32_t(1) << (aKeyCode & 0x1F);
  return &mKeyDownFlags[(aKeyCode >> 5)];
}

bool
MozKey::IsKeyDown(uint32_t aKeyCode)
{
  uint32_t mask;
  uint32_t* flag = GetFlagWord32(aKeyCode, &mask);
  return ((*flag) & mask) != 0;
}

void
MozKey::SetKeyDownFlag(uint32_t aKeyCode)
{
  uint32_t mask;
  uint32_t* flag = GetFlagWord32(aKeyCode, &mask);
  *flag |= mask;
}

void
MozKey::ClearKeyDownFlag(uint32_t aKeyCode)
{
  uint32_t mask;
  uint32_t* flag = GetFlagWord32(aKeyCode, &mask);
  *flag &= ~mask;
}

int
MozKey::QtModifierToDOMModifier(int aModifiers)
{
  int32_t modifiers = 0;
  if (aModifiers & Qt::ControlModifier) {
    modifiers |= nsIDOMWindowUtils::MODIFIER_CONTROL;
  }
  if (aModifiers & Qt::AltModifier) {
    modifiers |= nsIDOMWindowUtils::MODIFIER_ALT;
  }
  if (aModifiers & Qt::ShiftModifier) {
    modifiers |= nsIDOMWindowUtils::MODIFIER_SHIFT;
  }
  if (aModifiers & Qt::MetaModifier) {
    modifiers |= nsIDOMWindowUtils::MODIFIER_META;
  }
  return modifiers;
}
