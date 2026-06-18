// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_RUNNER_SETTINGS_CONF_H_
#define FLUTTER_RUNNER_SETTINGS_CONF_H_

// default >> /etc/palm/flutter-conf.json
static const char* const kSettingsFile  = "/etc/palm/flutter-conf.json";

#define FLUTTER_CONF_FILE "flutter-conf.json"
#define FLUTTER_APP_ID "FLUTTER_APP_ID"
#define FLUTTER_BUNDLE_PATH "FLUTTER_BUNDLE_PATH"
#define FLUTTER_ASSETS_PATH "FLUTTER_ASSETS_PATH"

#define FLUTTER_ALLOW_TAS "FLUTTER_ALLOW_TAS"

#define FLUTTER_RUNTIME_MODE "runtime_mode"
#define FLUTTER_FRAMEWORK_VERSION "flutter_framework_version"
#define FLUTTER_DISPLAY_BACKEND "display_backend"

#endif  // FLUTTER_RUNNER_SETTINGS_CONFIG_H_
