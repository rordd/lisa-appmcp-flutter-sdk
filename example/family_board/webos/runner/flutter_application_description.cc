// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_application_description.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "rapidjson/document.h"

#include "logger.h"

bool FlutterApplicationDescription::CheckTrustLevel(std::string trust_level) {
  if (trust_level.empty())
    return false;
  if (trust_level.compare("default") == 0)
    return true;
  if (trust_level.compare("trusted") == 0)
    return true;
  return false;
}

std::vector<std::string> SplitString(const std::string& str, char delimiter) {
  std::vector<std::string> resList;
  std::stringstream ss(str);
  std::string s;

  while (std::getline(ss, s, delimiter)) {
    resList.push_back(s);
  }

  return resList;
}

const std::string WindowTypeFromString(const std::string& str) {
  if (str == "overlay")
    return "_WEBOS_WINDOW_TYPE_OVERLAY";
  if (str == "popup")
    return "_WEBOS_WINDOW_TYPE_POPUP";
  if (str == "minimal")
    return "_WEBOS_WINDOW_TYPE_RESTRICTED";
  if (str == "floating")
    return "_WEBOS_WINDOW_TYPE_FLOATING";
  if (str == "system_ui")
    return "_WEBOS_WINDOW_TYPE_SYSTEM_UI";
  if (str == "screenSaver")
    return "_WEBOS_WINDOW_TYPE_SCREENSAVER";
  if (str == "home")
    return "_WEBOS_WINDOW_TYPE_HOME";
  return "_WEBOS_WINDOW_TYPE_CARD";
}

FlutterApplicationDescription::FlutterApplicationDescription()
    : transparency_(false),
      window_class_value_(kWindowClassNormal),
      native_lifecycle_interface_version_(1),
      back_history_api_disabled_(false),
      width_override_(0),
      height_override_(0),
      handle_exit_key_(false),
      support_portrait_mode_(false),
      supports_audio_guidance_(false),
      display_affinity_(0),
      use_virtual_keyboard_(true),
      window_group_defined_(false),
      window_group_owner_(false),
      window_group_allow_anon_(false),
      handles_relaunch_(false),
      cloudgame_active_(false),
      is_privileged_(false) {}


std::unique_ptr<FlutterApplicationDescription> FlutterApplicationDescription::FromAppInfo(const char* bundlePath)
{
    std::string pathAppInfo = std::string(bundlePath) + "/appinfo.json";

    std::ifstream fileAppInfo(pathAppInfo, std::ios::in);
    std::string appInfo = "{}";

    if (fileAppInfo) {
        fileAppInfo.seekg(0, std::ios::end);
        size_t size = fileAppInfo.tellg();
        fileAppInfo.seekg(0, std::ios::beg);

        appInfo.resize(size);
        fileAppInfo.read(&appInfo[0], size);

        if (appInfo.empty()) {
          appInfo = "{}";
        }
    }

    return FlutterApplicationDescription::FromJsonString(appInfo.c_str());
}

std::unique_ptr<FlutterApplicationDescription> FlutterApplicationDescription::FromJsonString(
    const char* json_str) {

  LOG_DEBUG("Parsing JSON-format: %s", json_str);

  rapidjson::Document doc;
  doc.Parse(json_str);

  if (doc.HasParseError()) {
    LOG_ERROR("Invalid JSON-format parse error: %s", json_str);
    return nullptr;
  }

  if (!doc.IsObject()) {
    LOG_ERROR("Invalid JSON-format parse error: %s", json_str);
    return nullptr;
  }

  auto app_desc =
      std::unique_ptr<FlutterApplicationDescription>(new FlutterApplicationDescription());

  app_desc->id_ = (doc.FindMember("id") != doc.MemberEnd())? doc["id"].GetString() : std::string();
  app_desc->type_ = (doc.FindMember("type") != doc.MemberEnd())? doc["type"].GetString() : std::string("flutter");
  app_desc->transparency_ = (doc.FindMember("transparent") != doc.MemberEnd())? doc["transparent"].GetBool() : false;
  app_desc->trust_level_ = (doc.FindMember("trustLevel") != doc.MemberEnd())? doc["trustLevel"].GetString() : std::string();
  std::string default_window_type = (doc.FindMember("defaultWindowType") != doc.MemberEnd())? doc["defaultWindowType"].GetString() : std::string();
  app_desc->default_window_type_ = WindowTypeFromString(default_window_type);
  app_desc->native_lifecycle_interface_version_ = (doc.FindMember("nativeLifeCycleInterfaceVersion") != doc.MemberEnd())? doc["nativeLifeCycleInterfaceVersion"].GetInt() : 1;
  app_desc->version_ = (doc.FindMember("version") != doc.MemberEnd())? doc["version"].GetString() : std::string();
  app_desc->back_history_api_disabled_ = (doc.FindMember("disableBackHistoryAPI") != doc.MemberEnd())? doc["disableBackHistoryAPI"].GetBool(): false;
  app_desc->icon_ = (doc.FindMember("icon") != doc.MemberEnd())? doc["icon"].GetString() : std::string();
  app_desc->folder_path_ = (doc.FindMember("folderPath") != doc.MemberEnd())? doc["folderPath"].GetString() : std::string();
  app_desc->title_ = (doc.FindMember("title") != doc.MemberEnd())? doc["title"].GetString() : std::string();
  app_desc->handle_exit_key_ = (doc.FindMember("handleExitKey") != doc.MemberEnd())? doc["handleExitKey"].GetBool() : false;
  app_desc->support_portrait_mode_ = (doc.FindMember("supportPortraitMode") != doc.MemberEnd())? doc["supportPortraitMode"].GetBool() : false;
  app_desc->use_virtual_keyboard_ = (doc.FindMember("enableKeyboard") != doc.MemberEnd())? doc["enableKeyboard"].GetBool() : true;
  app_desc->location_hint_ = (doc.FindMember("locationHint") != doc.MemberEnd())? doc["locationHint"].GetString() : std::string();
  app_desc->handles_relaunch_ = (doc.FindMember("handlesRelaunch") != doc.MemberEnd())? doc["handlesRelaunch"].GetBool() : false;
  app_desc->cloudgame_active_ = (doc.FindMember("cloudgame_active") != doc.MemberEnd())? doc["cloudgame_active"].GetBool() : false;

  // Handle accessibility, supportsAudioGuidance
  if (doc.FindMember("accessibility") != doc.MemberEnd()) {
    rapidjson::Value& accessibility =  doc["accessibility"];
    if (accessibility.IsObject()) {
      if (accessibility.FindMember("supportsAudioGuidance") != accessibility.MemberEnd()) {
        rapidjson::Value& audio_guidance = accessibility["supportsAudioGuidance"];
        app_desc->supports_audio_guidance_ =
          audio_guidance.IsBool() && audio_guidance.GetBool();
      }
    }
  }

  // Handle resolution
  if (doc.FindMember("resolution") != doc.MemberEnd()) {
    rapidjson::Value& resolution = doc["resolution"];
    if (resolution.IsString()) {
      std::string override_resolution = doc["resolution"].GetString();
      auto res_list = SplitString(override_resolution, 'x');
      if (res_list.size() == 2) {
        app_desc->width_override_ = std::stoi(res_list.at(0));
        app_desc->height_override_ = std::stoi(res_list.at(1));
      }
      if (app_desc->width_override_ < 0 || app_desc->height_override_ < 0) {
        app_desc->width_override_ = 0;
        app_desc->height_override_ = 0;
      }
    }
  }

  // Handle keyFilterTable
  // Key code is changed only for facebooklogin WebApp
  if (doc.FindMember("keyFilterTable") != doc.MemberEnd()) {
    rapidjson::Value& key_filter_table = doc["keyFilterTable"];
    if (key_filter_table.IsArray()) {
      for (auto& k : key_filter_table.GetArray()) {
        if (!k.IsObject())
          continue;
        int from = k["from"].GetInt();
        int to = k["to"].GetInt();
        int modifier = k["modifier"].GetInt();
        app_desc->key_filter_table_[from] = std::make_pair(to, modifier);
      }
    }
  }

  // Handle trustLevel
  if (!app_desc->CheckTrustLevel(app_desc->trust_level_))
    app_desc->trust_level_ = std::string("default");

  // Handle hidden property of window class
  // Convert a json object for window class to an enum value
  // and store it instead of the json object.
  // The format of window class in the appinfo.json is as below.
  //
  // class : { "hidden" : boolean }
  //
  WindowClass class_value = kWindowClassNormal;
  if (doc.FindMember("class") != doc.MemberEnd()) {
    rapidjson::Value& clazz = doc["class"];
    if (clazz.IsObject()) {
      if (clazz["hidden"].IsBool() && clazz["hidden"].GetBool())
        class_value = kWindowClassHidden;
    }
  }
  app_desc->window_class_value_ = class_value;

  if (doc.FindMember("windowGroup") != doc.MemberEnd()) {
    app_desc->window_group_defined_ = true;
    rapidjson::Value& window_group_info = doc["windowGroup"];

    if (window_group_info.IsObject()) {
      if (window_group_info.FindMember("name") != window_group_info.MemberEnd() &&
          window_group_info.FindMember("owner") != window_group_info.MemberEnd())
      {
        app_desc->window_group_name_ = window_group_info["name"].GetString();
        app_desc->window_group_owner_ = window_group_info["owner"].GetBool();
      }
      app_desc->window_group_allow_anon_ = false;

      if (app_desc->window_group_owner_) {
        if (window_group_info.FindMember("ownerInfo") != window_group_info.MemberEnd()) {
          rapidjson::Value& owner_info =  window_group_info["ownerInfo"];
          if (owner_info.IsObject()) {
            if (owner_info.FindMember("allowAnonymous") != owner_info.MemberEnd()) {
              app_desc->window_group_allow_anon_ = owner_info["allowAnonymous"].GetBool();
            }

            if (owner_info.FindMember("layers") != owner_info.MemberEnd()) {
              rapidjson::Value& layers =  owner_info["layers"];
              if (layers.IsArray()) {
                for (auto& layer : layers.GetArray()) {
                  if (!layer.IsObject())
                    continue;
                  app_desc->window_group_layers_[layer["name"].GetString()] = layer["z"].GetInt();
                }
              }
            }
          }
        }
      } else {
        if (window_group_info.FindMember("clientInfo") != window_group_info.MemberEnd()) {
          rapidjson::Value& client_info =  window_group_info["clientInfo"];
          if (client_info.FindMember("layer") != client_info.MemberEnd()) {
            rapidjson::Value& layer =  client_info["layer"];
            app_desc->window_group_layers_[layer.GetString()] = 0;
          }
        }
      }
    }
  }

  // Handle bundlePath
  if (!app_desc->folder_path_.empty()) {
    struct stat stat_ent_pt;
    std::string temp_path = app_desc->folder_path_ + "/" + app_desc->icon_;
    if (!stat(temp_path.c_str(), &stat_ent_pt)) {
      app_desc->icon_ = temp_path;
    }
  }

  if (app_desc->id_.find("com.palm.") == 0
      || app_desc->id_.find("com.webos.") == 0
      || app_desc->id_.find("com.lge.") == 0 ) {
    app_desc->is_privileged_ = true;
  }

  app_desc->main_path_ = (doc.FindMember("main") != doc.MemberEnd())? doc["main"].GetString() : std::string();

  return app_desc;
}

void FlutterApplicationDescription::SetDefaultWindowType(const std::string windowType) {
  default_window_type_ = WindowTypeFromString(windowType);
}
