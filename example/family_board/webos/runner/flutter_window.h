// Copyright (c) 2025 LG Electronics, Inc. All rights reserved.
// Copyright 2021 Sony Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_WINDOW_
#define FLUTTER_WINDOW_

#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>
#include "flutter_application_description.h"
#include "flutter_embedder_loader.h"

#include <memory>

class FlutterWindow {
 public:
  explicit FlutterWindow(
      const flutter::FlutterViewController::ViewProperties view_properties,
      const flutter::DartProject project);
  ~FlutterWindow() = default;

  // Prevent copying.
  FlutterWindow(FlutterWindow const&) = delete;
  FlutterWindow& operator=(FlutterWindow const&) = delete;

  bool OnCreate(std::shared_ptr<FlutterApplicationDescription> appDesc);
  void OnDestroy();
  void Run();

 private:
  flutter::FlutterViewController::ViewProperties view_properties_;
  flutter::DartProject project_;
  std::unique_ptr<flutter::FlutterViewController> flutter_view_controller_;
  std::unique_ptr<WebosInterfaceLoader> webos_plugin_interface_;
  std::unique_ptr<EmbedderLoader> embedder_;
};

#endif  // FLUTTER_WINDOW_
