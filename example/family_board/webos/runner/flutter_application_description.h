// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_APPLICATION_DESCRIPTION_H_
#define FLUTTER_APPLICATION_DESCRIPTION_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

typedef int32_t DisplayId;

class FlutterApplicationDescription {
 public:
  enum WindowClass { kWindowClassNormal = 0x00, kWindowClassHidden = 0x01 };

  FlutterApplicationDescription();
  virtual ~FlutterApplicationDescription() {}

  const std::string& Id() const { return id_; }
  const std::string& Title() const { return title_; }
  const std::string& Icon() const { return icon_; }
  const std::string& Type() const { return type_; }

  bool IsTransparent() const { return transparency_; }

  WindowClass WindowClassValue() const { return window_class_value_; }

  const std::string& TrustLevel() const { return trust_level_; }

  const std::string& BundlePath() const { return folder_path_; }

  const std::string& DefaultWindowType() const { return default_window_type_; }
  void SetDefaultWindowType(const std::string windowType);

  const std::string& Version() const { return version_; }

  static std::unique_ptr<FlutterApplicationDescription> FromAppInfo(
      const char* bundlePath);

  static std::unique_ptr<FlutterApplicationDescription> FromJsonString(
      const char* json_str);

  int NativeLifeCycleInterfaceVersion() const { return native_lifecycle_interface_version_; }

  bool BackHistoryAPIDisabled() const { return back_history_api_disabled_; }
  void SetBackHistoryAPIDisabled(bool disabled) {
    back_history_api_disabled_ = disabled;
  }

  bool UseVirtualKeyboard() const { return use_virtual_keyboard_; }

  const std::unordered_map<int, std::pair<int, int>>& KeyFilterTable() const {
    return key_filter_table_;
  }

  int WidthOverride() const { return width_override_; }
  int HeightOverride() const { return height_override_; }

  bool HandleExitKey() const { return handle_exit_key_; }
  bool SupportsAudioGuidance() const { return supports_audio_guidance_; }
  bool SupportPortraitMode() const { return support_portrait_mode_; }

  const std::string& LocationHint() const { return location_hint_; }

  // To support multi display
  DisplayId GetDisplayAffinity() { return display_affinity_; }
  void SetDisplayAffinity(DisplayId display) { display_affinity_ = display; }

  bool HandlesRelaunch() const { return handles_relaunch_; }

  // Getter of webOS windowGroup information
  bool HasWindowGroupInfo() const { return window_group_defined_; }
  const std::string& WindowGroupName() const { return window_group_name_; }
  bool IsWindowGroupOwner() const { return window_group_owner_; }
  bool IsWindowGroupAllowAnon() const { return window_group_allow_anon_; }
  const std::unordered_map<std::string, int>& WindowGroupLayers() const {
    return window_group_layers_;
  }

  // gamepad
  bool CloudgameActive() const { return cloudgame_active_; }

  bool isPrivileged() const {return is_privileged_; }
  const std::string& MainPath() const { return main_path_; }

 private:
  bool CheckTrustLevel(std::string trust_level);

  std::string id_;
  std::string title_;
  std::string icon_;
  std::string type_;

  bool transparency_;
  WindowClass window_class_value_;
  std::string trust_level_;
  std::string folder_path_;
  std::string default_window_type_;
  std::string version_;
  int native_lifecycle_interface_version_;
  bool back_history_api_disabled_;
  int width_override_;
  int height_override_;
  std::unordered_map<int, std::pair<int, int>> key_filter_table_;
  bool handle_exit_key_;
  bool support_portrait_mode_;
  bool supports_audio_guidance_;
  int display_affinity_;
  std::string location_hint_;
  bool use_virtual_keyboard_;
  bool is_home_;
  bool handles_relaunch_;

  // webOS windowGroup information
  bool window_group_defined_;
  std::string window_group_name_;
  bool window_group_owner_;
  bool window_group_allow_anon_;
  std::unordered_map<std::string, int> window_group_layers_;

  // gamepad
  bool cloudgame_active_;

  bool is_privileged_;
  std::string main_path_;
};

#endif  // FLUTTER_APPLICATION_DESCRIPTION_H_
