/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:expandtab:shiftwidth=4:tabstop=4:
 */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __EmbedQtKeyUtils_h__
#define __EmbedQtKeyUtils_h__

#include "nscore.h"
#include <string.h>

class MozKey
{
public:
  static int QtModifierToDOMModifier(int aModifiers);
  static int QtKeyCodeToDOMKeyCode(int aKeysym, int aModifiers = 0);
  static int DOMKeyCodeToQtKeyCode(int aKeysym);

  static uint32_t* GetFlagWord32(uint32_t aKeyCode, uint32_t* aMask);
  static bool IsKeyDown(uint32_t aKeyCode);
  static void SetKeyDownFlag(uint32_t aKeyCode);
  static void ClearKeyDownFlag(uint32_t aKeyCode);
};

#endif /* __EmbedQtKeyUtils_h__ */
