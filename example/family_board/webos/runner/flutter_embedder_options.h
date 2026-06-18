// Copyright (c) 2025 LG Electronics, Inc. All rights reserved.
// Copyright 2021 Sony Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_EMBEDDER_OPTIONS_
#define FLUTTER_EMBEDDER_OPTIONS_

#include <flutter/flutter_view_controller.h>

#include <limits.h>
#include <string>
#include <unistd.h>

#include "command_options.h"
#include "logger.h"

using namespace flutter;

class FlutterEmbedderOptions {
 public:
  FlutterEmbedderOptions() {
    options_.AddString("display", "g",
                       "Display backend [wayland|starfish|gbm|eglstream|x11]",
                       "wayland", false);
    options_.AddString("bundle", "b", "Path to Flutter project bundle",
                       "", false);
    options_.AddWithoutValue("cursor", "c", "show mouse cursor/pointer",
                             false);
    options_.AddInt("rotation", "r",
                    "Window rotation(degree) [0(default)|90|180|270]", 0,
                    false);
    options_.AddDouble("force-scale-factor", "s",
                       "Force a scale factor instead using default value", 1.0,
                       false);

    options_.AddString("title", "t", "Window title", "Flutter", false);
    options_.AddWithoutValue("onscreen-keyboard", "k",
                             "Enable on-screen keyboard", false);
    options_.AddWithoutValue("window-decoration", "d",
                             "Enable window decorations", false);
    options_.AddWithoutValue("fullscreen", "f", "Always full-screen display",
                             false);
    options_.AddInt("width", "w", "Window width", 0, false);
    options_.AddInt("height", "h", "Window height", 0, false);

    options_.AddString("appId", "i", "set AppId", "com.webos.app.example.flutter", false);
    options_.AddString("parameters", "p", "set launch params", "{}", false);
    options_.AddString("appDesc", "a", "set Application Descriptions", "{}", false);
    options_.AddString("preload", "l", "Use preload", "", false);
    options_.AddString("keepAlive", "e", "keepAlive", "false", false);
    options_.AddString("main", "m", "Redirect Path to Flutter project bundle", "", false);

  }

  ~FlutterEmbedderOptions() = default;

  std::string getExecutableDirectory() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count == -1) {
      return std::string();
    }
    std::string execPath =  std::string(result, count);
    size_t found = execPath.find_last_of("/");
    return execPath.substr(0, found);
  }

  bool Parse(int argc, char** argv) {
    if (!options_.Parse(argc, argv)) {
      LOG_ERROR("%s", options_.GetError().c_str());
      std::cout << options_.ShowHelp();
      return false;
    }

    bundle_path_ = options_.GetValue<std::string>("bundle");
    if (bundle_path_.empty()) bundle_path_ = getExecutableDirectory();

    use_mouse_cursor_ = options_.Exist("cursor");
    if (options_.Exist("rotation")) {
      switch (options_.GetValue<int>("rotation")) {
        case 90:
          window_view_rotation_ =
              flutter::FlutterViewController::ViewRotation::kRotation_90;
          break;
        case 180:
          window_view_rotation_ =
              flutter::FlutterViewController::ViewRotation::kRotation_180;
          break;
        case 270:
          window_view_rotation_ =
              flutter::FlutterViewController::ViewRotation::kRotation_270;
          break;
        default:
          window_view_rotation_ =
              flutter::FlutterViewController::ViewRotation::kRotation_0;
          break;
      }
    }

    if (options_.Exist("force-scale-factor")) {
      is_force_scale_factor_ = true;
      scale_factor_ = options_.GetValue<double>("force-scale-factor");
    } else {
      is_force_scale_factor_ = false;
      scale_factor_ = 1.0;
    }

    display_backend_ = options_.GetValue<std::string>("display");

    if (display_backend_ == "wayland")
    {
      window_title_ = options_.GetValue<std::string>("title");
      use_onscreen_keyboard_ = options_.Exist("onscreen-keyboard");
      use_window_decoration_ = options_.Exist("window-decoration");
      window_view_mode_ =
          options_.Exist("fullscreen")
              ? flutter::FlutterViewController::ViewMode::kFullscreen
              : flutter::FlutterViewController::ViewMode::kNormal;
      window_width_ = options_.GetValue<int>("width");
      window_height_ = options_.GetValue<int>("height");
    } else if (display_backend_ == "x11") {
      use_onscreen_keyboard_ = false;
      use_window_decoration_ = false;
      window_title_ = options_.GetValue<std::string>("title");
      window_view_mode_ =
          options_.Exist("fullscreen")
              ? flutter::FlutterViewController::ViewMode::kFullscreen
              : flutter::FlutterViewController::ViewMode::kNormal;
      window_width_ = options_.GetValue<int>("width");
      window_height_ = options_.GetValue<int>("height");
    } else {
      // gbm, eglstream, starfish
      use_onscreen_keyboard_ = false;
      use_window_decoration_ = false;
      window_view_mode_ = flutter::FlutterViewController::ViewMode::kFullscreen;
    }

    app_id_ = options_.GetValue<std::string>("appId");
    params_ = options_.GetValue<std::string>("parameters");
    app_desc_ = options_.GetValue<std::string>("appDesc");
    preload_ = options_.GetValue<std::string>("preload");
    keep_alive_ = options_.GetValue<std::string>("keepAlive");
    redirect_path_ = options_.GetValue<std::string>("main");

    return true;
  }

  std::string BundlePath() const { return bundle_path_; }
  std::string WindowTitle() const { return window_title_; }
  bool IsUseMouseCursor() const { return use_mouse_cursor_; }
  bool IsUseOnscreenKeyboard() const { return use_onscreen_keyboard_; }
  bool IsUseWindowDecoraation() const { return use_window_decoration_; }
  flutter::FlutterViewController::ViewMode WindowViewMode() const {
    return window_view_mode_;
  }
  int WindowWidth() const { return window_width_; }
  int WindowHeight() const { return window_height_; }
  flutter::FlutterViewController::ViewRotation WindowRotation() const {
    return window_view_rotation_;
  }
  bool IsForceScaleFactor() const { return is_force_scale_factor_; }
  double ScaleFactor() const { return scale_factor_; }

  std::string AppId() const { return app_id_; }
  std::string Params() const { return params_; }
  std::string AppDesc() const { return app_desc_; }
  std::string DisplayBackend() const { return display_backend_; }
  bool IsPreload() const { return preload_.empty()? false : true; }
  bool IsKeepAlive() const {return keep_alive_ == "true"? true: false;}
  std::string RedirectPath() const { return redirect_path_; }

 private:
  commandline::CommandOptions options_;

  std::string bundle_path_;
  std::string window_title_;
  bool use_mouse_cursor_ = true;
  bool use_onscreen_keyboard_ = false;
  bool use_window_decoration_ = false;
  flutter::FlutterViewController::ViewMode window_view_mode_ =
      flutter::FlutterViewController::ViewMode::kNormal;
  int window_width_ = 0;
  int window_height_ = 0;
  flutter::FlutterViewController::ViewRotation window_view_rotation_ =
      flutter::FlutterViewController::ViewRotation::kRotation_0;
  bool is_force_scale_factor_ = false;
  double scale_factor_ = 1.0;
  std::string app_id_;
  std::string params_;
  std::string app_desc_;
  std::string preload_;
  std::string keep_alive_;
  std::string display_backend_;
  std::string redirect_path_;
};

#endif  // FLUTTER_EMBEDDER_OPTIONS_
