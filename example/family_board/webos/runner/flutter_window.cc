// Copyright (c) 2025 LG Electronics, Inc. All rights reserved.
// Copyright 2021 Sony Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_window.h"

#include "logger.h"

#include <chrono>
#include <cmath>
#include <dlfcn.h>
#include <iostream>
#include <thread>

using namespace flutter;

FlutterWindow::FlutterWindow(
    const flutter::FlutterViewController::ViewProperties view_properties,
    const flutter::DartProject project)
    : view_properties_(view_properties), project_(project) {}

bool FlutterWindow::OnCreate(std::shared_ptr<FlutterApplicationDescription>appDesc) {

  embedder_ = std::make_unique<EmbedderLoader>();

  flutter::FlutterViewController* vc = embedder_->CreateViewController(view_properties_, project_);

  flutter_view_controller_ = std::unique_ptr<flutter::FlutterViewController>(vc);

  // Ensure that basic setup of the controller was successful.
  if (!flutter_view_controller_->engine() ||
      !flutter_view_controller_->view()) {
    return false;
  }

  // Set additional window Property
  if (appDesc) {
    LOG_DEBUG("id: %s", appDesc->Id().c_str());

    // Register Flutter plugins.
    webos_plugin_interface_ = std::make_unique<WebosInterfaceLoader>();
    webos_plugin_interface_->RegisterPlugins(flutter_view_controller_->engine());


    flutter::FlutterView* view = flutter_view_controller_->view();

    view->SetWindowProperty("appId", appDesc->Id());
    view->SetWindowProperty("title", appDesc->Title());
    view->SetWindowProperty("icon", appDesc->Icon());
    view->SetWindowProperty("subtitle", std::string());
    view->SetWindowProperty("_WEBOS_WINDOW_CLASS",
                    std::to_string(static_cast<int>(appDesc->WindowClassValue())));

    // specify window type and key policy
    view->SetWindowProperty("_WEBOS_WINDOW_TYPE", appDesc->DefaultWindowType());
    view->SetWindowProperty("_WEBOS_ACCESS_POLICY_KEYS_BACK",
                    appDesc->BackHistoryAPIDisabled() ? "true" : "false");
    view->SetWindowProperty("_WEBOS_ACCESS_POLICY_KEYS_EXIT",
                    appDesc->HandleExitKey() ? "true" : "false");

    view->SetWindowProperty("displayAffinity", std::to_string(static_cast<int>(appDesc->GetDisplayAffinity())));
    view->SetWindowProperty("locationHint", appDesc->LocationHint());
    view->SetWindowProperty("cloudgame_active",
                    appDesc->CloudgameActive() ? "true" : "false");
  }

  return true;
}

void FlutterWindow::OnDestroy() {
  if (flutter_view_controller_) {
    flutter_view_controller_ = nullptr;
  }
}

void FlutterWindow::Run() {
  // Main loop.
  auto next_flutter_event_time =
      std::chrono::steady_clock::time_point::clock::now();
  while (flutter_view_controller_->view()->DispatchEvent()) {
    // Wait until the next event.
    {
      auto wait_duration =
          std::max(std::chrono::nanoseconds(0),
                   next_flutter_event_time -
                       std::chrono::steady_clock::time_point::clock::now());
      std::this_thread::sleep_for(
          std::chrono::duration_cast<std::chrono::milliseconds>(wait_duration));
    }

    // Processes any pending events in the Flutter engine, and returns the
    // number of nanoseconds until the next scheduled event (or max, if none).
    auto wait_duration = flutter_view_controller_->engine()->ProcessMessages();
    {
      auto next_event_time = std::chrono::steady_clock::time_point::max();
      if (wait_duration != std::chrono::nanoseconds::max()) {
        next_event_time =
            std::min(next_event_time,
                     std::chrono::steady_clock::time_point::clock::now() +
                         wait_duration);
      } else {
        // Wait for the next frame if no events.
        auto frame_rate = flutter_view_controller_->view()->GetFrameRate();
        next_event_time = std::min(
            next_event_time,
            std::chrono::steady_clock::time_point::clock::now() +
                std::chrono::milliseconds(
                    static_cast<int>(std::trunc(1000000.0 / frame_rate))));
      }
      next_flutter_event_time =
          std::max(next_flutter_event_time, next_event_time);
    }
  }
}
