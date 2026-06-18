// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_WAYLAND_CLIENT_LOGGER_H_
#define FLUTTER_WAYLAND_CLIENT_LOGGER_H_

#include <string.h>

#include <cstdio>
#include <iostream>
#include <sstream>

#include <PmLogLib.h>

#define FLUTTER_LOG_CONTEXT "flutter"
#define FLUTTER_MSGID "runner"

extern PmLogContext GetPmLogContext();

#define LOG_CRITICAL(...) \
          PmLogCritical(GetPmLogContext(), FLUTTER_MSGID, 0, ##__VA_ARGS__)
#define LOG_ERROR(...) \
        PmLogError(GetPmLogContext(), FLUTTER_MSGID, 0,##__VA_ARGS__)
#define LOG_WARNING(...) \
        PmLogWarning(GetPmLogContext(), FLUTTER_MSGID, 0, ##__VA_ARGS__)
#define LOG_INFO(...) \
        PmLogInfo(GetPmLogContext(), FLUTTER_MSGID, 0, ##__VA_ARGS__)
#define LOG_DEBUG(...) \
        PmLogDebug(GetPmLogContext(), ##__VA_ARGS__)

#endif  // FLUTTER_WAYLAND_CLIENT_LOGGER_H_
