// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LAUNCH_PARAMS_H_
#define FLUTTER_LAUNCH_PARAMS_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

class FlutterLaunchParams {
 public:
  FlutterLaunchParams();
  virtual ~FlutterLaunchParams() {}

  static std::unique_ptr<FlutterLaunchParams> FromJsonString(
      const char* json_str);

  int GetDisplayAffinity() { return display_affinity_; }
  bool IsLaunchedHidden() { return launched_hidden_; }
  std::string GetRouteTarget() { return route_target_; }
  std::string GetWindowType() { return window_type_; }

 private:
  int display_affinity_;
  bool launched_hidden_;
  std::string route_target_;
  std::string window_type_;
};

#endif  // FLUTTER_LAUNCH_PARAMS_H_
