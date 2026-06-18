// Copyright (c) 2025 LG Electronics, Inc. All rights reserved.
// Copyright 2021 Sony Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>

#include <iostream>
#include <memory>
#include <string>

#include "flutter_embedder_options.h"
#include "flutter_embedder_loader.h"
#include "flutter_window.h"
#include "flutter_webos_window_properties.h"
#include "flutter_launch_params.h"
#include "settings.h"
#include "logger.h"

int main(int argc, char** argv) {
  FlutterEmbedderOptions options;
  try {
    if (!options.Parse(argc, argv)) {
      return 0;
    }
  } catch(commandline::Exception& e){
     LOG_ERROR("e: %s", e.what());
     return 0;
  }

  std::shared_ptr<FlutterApplicationDescription> app_desc = nullptr;
  std::shared_ptr<FlutterLaunchParams> launch_params = nullptr;
  if (options.AppDesc() != "{}") {
    app_desc = FlutterApplicationDescription::FromJsonString(options.AppDesc().c_str());
  } else if (!options.BundlePath().empty()) {
    app_desc = FlutterApplicationDescription::FromAppInfo(options.BundlePath().c_str());
  }
  if (options.Params() != "{}") {
    launch_params = FlutterLaunchParams::FromJsonString(options.Params().c_str());
  }
  if (launch_params && app_desc) {
    app_desc->SetDisplayAffinity(launch_params->GetDisplayAffinity());
    if (!launch_params->GetWindowType().empty()) {
      app_desc->SetDefaultWindowType(launch_params->GetWindowType());
    }
  }

  Settings& settings = Settings::getInstance();
  settings.init();

  // Creates the Flutter project.
  std::string bundle_path;
  if (app_desc && !app_desc->BundlePath().empty()) {
    bundle_path = app_desc->BundlePath();
  } else {
    bundle_path = options.BundlePath();
  }

  if (app_desc &&
      app_desc->isPrivileged()) {
    std::string main_path;
    std::string file_scheme = "file://";
    if (!options.RedirectPath().empty()) {
      LOG_DEBUG("redirectPath: %s", options.RedirectPath().c_str());
      main_path = options.RedirectPath();
    } else {
      LOG_DEBUG("mainPath: %s", app_desc->MainPath().c_str());
      main_path = app_desc->MainPath();
    }
    std::string::size_type i = main_path.find(file_scheme);
    if (!main_path.empty() && (i == 0)) {
      main_path.erase(i, file_scheme.length());
      bundle_path = main_path;
    }
  }

  const std::wstring fl_path(bundle_path.begin(), bundle_path.end());
  flutter::DartProject project(fl_path);
  auto command_line_arguments = std::vector<std::string>();

  if (settings.getBool(FLUTTER_ALLOW_TAS)) {
    command_line_arguments.push_back("tas");
  } else {
    command_line_arguments.push_back("normal");
  }
  if (launch_params) {
    command_line_arguments.push_back(options.Params());
  }
  project.set_dart_entrypoint_arguments(std::move(command_line_arguments));

  flutter::FlutterViewController::ViewProperties view_properties = {};

  view_properties.width = options.WindowWidth();
  view_properties.height = options.WindowHeight();
  view_properties.view_mode = options.WindowViewMode();
  view_properties.view_rotation = options.WindowRotation();
  view_properties.title = options.WindowTitle();
  view_properties.use_mouse_cursor = options.IsUseMouseCursor();
  view_properties.use_onscreen_keyboard = options.IsUseOnscreenKeyboard();
  view_properties.use_window_decoration = options.IsUseWindowDecoraation();
  view_properties.force_scale_factor = options.IsForceScaleFactor();
  view_properties.scale_factor = options.ScaleFactor();

  std::wstring ws = project.assets_path();
  std::string assets_path(ws.begin(),ws.end());
  view_properties.assets_path = assets_path.c_str();

  if (app_desc) {
    if(!app_desc->Id().empty()) {
      view_properties.app_id = app_desc->Id();
    }

    if (!app_desc->DefaultWindowType().empty()) {
      view_properties.default_window_type = app_desc->DefaultWindowType();
    }

    if (app_desc->WidthOverride() > 0) {
      view_properties.width = app_desc->WidthOverride();
    }

    if (app_desc->HeightOverride() > 0) {
      view_properties.height = app_desc->HeightOverride();
    }

    view_properties.use_onscreen_keyboard = app_desc->UseVirtualKeyboard();
    view_properties.opaque = !app_desc->IsTransparent();
    view_properties.support_portrait_mode = app_desc->SupportPortraitMode();
    view_properties.handles_relaunch = app_desc->HandlesRelaunch();
    view_properties.native_interface_version = app_desc->NativeLifeCycleInterfaceVersion();

    WebosOptionalProperties* webos_options = new WebosOptionalProperties();
    view_properties.webos_properties = webos_options;

    webos_options->native_type = (app_desc->Type() != "flutter");
    if (app_desc->HasWindowGroupInfo()) {
      LOG_DEBUG("Set WindowGroup Info");
      auto window_group_info = std::make_unique<WindowGroupInfo>(
                                          app_desc->WindowGroupName(),
                                          app_desc->IsWindowGroupOwner(),
                                          app_desc->IsWindowGroupAllowAnon()
                               );

      for (const auto& [name, zorder] : app_desc->WindowGroupLayers())
      {
        window_group_info->AddLayer(name, zorder);
      }
      webos_options->window_group_info_ = std::move(window_group_info);
    }
  }

  //if width or height is not set, create window in fullscreen size
  if (view_properties.width == 0 || view_properties.height == 0)
    view_properties.view_mode = flutter::FlutterViewController::ViewMode::kFullscreen;

  view_properties.launched_hidden = options.IsPreload();
  if (launch_params) {
    view_properties.launched_hidden |= launch_params->IsLaunchedHidden();
    view_properties.display_affinity = launch_params->GetDisplayAffinity();

    if (!launch_params->GetRouteTarget().empty())
      view_properties.route_target = launch_params->GetRouteTarget();
  }

  view_properties.keep_alive = options.IsKeepAlive();

  settings.setEnv(*view_properties.app_id, bundle_path, assets_path);
  settings.load(assets_path + "/version.json", false);
  settings.set(FLUTTER_DISPLAY_BACKEND, options.DisplayBackend());

  // The Flutter instance hosted by this window.
  FlutterWindow window(view_properties, project);
  if (!window.OnCreate(app_desc)) {
    return 0;
  }

  window.Run();
  window.OnDestroy();

  delete view_properties.webos_properties;
  return 0;
}
