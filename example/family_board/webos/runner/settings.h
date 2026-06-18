// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_RUNNER_SETTINGS_H_
#define FLUTTER_RUNNER_SETTINGS_H_

#include <string>
#include <map>

#include "settings_conf.h"   //generated from cmake

class Settings {
 private:
  Settings() {};
  virtual ~Settings() {}

  std::map<std::string, std::string> m_settings;

 public:
  static Settings& getInstance()
  {
    static Settings instance;
    return instance;
  }

  void init();

  void setEnv(std::string const & app_id = std::string(),
              std::string const & bundle_path = std::string(),
              std::string const & assets_path = std::string());

  bool load(std::string const & conf_path, bool isExport);

  void set(std::string key, std::string value) { m_settings[key] = value; }
  std::string& get(std::string key);
  bool getBool(std::string key);

  inline std::string& getBundlePath() { return get(FLUTTER_BUNDLE_PATH); }
  inline std::string& getRuntimeMode() { return get(FLUTTER_RUNTIME_MODE); }
  inline std::string& getFrameworkVersion() { return get(FLUTTER_FRAMEWORK_VERSION); }
  inline std::string& getDisplayBackend() { return get(FLUTTER_DISPLAY_BACKEND); }
};

#endif // FLUTTER_RUNNER_SETTINGS_H_

