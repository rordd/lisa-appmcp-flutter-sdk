// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "logger.h"

PmLogContext GetPmLogContext()
{
  static PmLogContext ctx = PmLogGetContextInline(FLUTTER_LOG_CONTEXT);
  return ctx;
}
