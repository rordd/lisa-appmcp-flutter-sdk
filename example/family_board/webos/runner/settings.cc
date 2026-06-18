// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <unistd.h>
#include <string>
#include <cstdlib>

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

#include "flutter_embedder_loader.h"
#include "logger.h"
#include "settings.h"
#include "settings_conf.h"   //generated from cmake

using namespace rapidjson;

void Settings::init()
{
  load(kSettingsFile, true);
}

void Settings::setEnv(std::string const & app_id,
                      std::string const & bundle_path,
                      std::string const & assets_path)
{
  if (!app_id.empty())
  {
    setenv(FLUTTER_APP_ID, app_id.c_str(), 0);
    m_settings[FLUTTER_APP_ID] = app_id;
  }

  if (!bundle_path.empty())
  {
    setenv(FLUTTER_BUNDLE_PATH, bundle_path.c_str(),0);
    m_settings[FLUTTER_BUNDLE_PATH] = bundle_path;

    std::string local_conf_file = bundle_path + "/" + FLUTTER_CONF_FILE;
    load(local_conf_file.c_str(), true);
  }

  if (!assets_path.empty())
  {
    setenv(FLUTTER_ASSETS_PATH, assets_path.c_str(),0);
    m_settings[FLUTTER_ASSETS_PATH] = assets_path;
  }


  m_settings[FLUTTER_RUNTIME_MODE] = "release";
  m_settings[FLUTTER_FRAMEWORK_VERSION] = "latest";
}

bool Settings::load(std::string const & conf_path, bool isExport)
{
  if (conf_path.empty()) return false;

  LOG_INFO("LoadConf : %s", conf_path.c_str());
  FILE* fp = fopen(conf_path.c_str(), "r");

  if (fp == NULL) {
    LOG_WARNING("Failed to load conf file");
    return false;
  }

  char readBuffer[65536];
  FileReadStream is(fp, readBuffer, sizeof(readBuffer));

  Document doc;
  doc.ParseStream(is);

  if (doc.HasParseError()) {
    LOG_ERROR("Invalid JSON-format parse error");
    return false;
  }

  if (!doc.IsObject()) {
    LOG_ERROR("Invalid JSON-format parse error");
    return false;
  }

  std::string param;
  std::string value;
  bool has_value;

  for (auto&m : doc.GetObject()) {
    param = m.name.GetString();
    has_value = !m.value.IsNull();
    if (!param.empty() && has_value) {
      Value& v = m.value;
      if (v.IsString()) {
        value = v.GetString();
      } else if (v.IsBool()) {
        value = v.GetBool() ? "true" : "false";
      } else if (v.IsInt()) {
        value = std::to_string(v.GetInt());
      } else if (v.IsObject()) {
        StringBuffer strbuf(0, 1024); //allocate 1024byte, default 512bytes
        Writer<StringBuffer> writer(strbuf);
        v.Accept(writer);
        value = strbuf.GetString();
      }

      if (isExport)
        setenv(param.c_str(),value.c_str(), 0);

      m_settings[param] = value;
    }
  }

  fclose(fp);

  return true;
}

std::string& Settings::get(std::string key)
{
  static std::string emptyString = "";

  if (m_settings.find(key) == m_settings.end()) {
    return emptyString;
  }

  return m_settings[key];
}

bool Settings::getBool(std::string key)
{
  if (m_settings.find(key) == m_settings.end()) {
    return false;
  }

  if (m_settings[key] ==  "true") return true;
  return false;
}
