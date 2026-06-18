// Copyright (c) 2023 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_launch_params.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "rapidjson/document.h"

FlutterLaunchParams::FlutterLaunchParams()
    : display_affinity_(0),
      launched_hidden_(false) {}

std::unique_ptr<FlutterLaunchParams> FlutterLaunchParams::FromJsonString(
    const char* json_str) {

  std::cout << "Parsing JSON-format: " << json_str << std::endl;

  rapidjson::Document doc;
  doc.Parse(json_str);

  if (doc.HasParseError()) {
    std::cout << "Invalid JSON-format parse error: " << json_str << std::endl;
    return nullptr;
  }

  if (!doc.IsObject()) {
    std::cout << "Invalid JSON-format parse error: " << json_str << std::endl;
    return nullptr;
  }

  auto params =
      std::unique_ptr<FlutterLaunchParams>(new FlutterLaunchParams());

  params->launched_hidden_ =  (doc.FindMember("launchedHidden") != doc.MemberEnd())? doc["launchedHidden"].GetBool() : false;
  params->display_affinity_ = (doc.FindMember("displayAffinity") != doc.MemberEnd())? doc["displayAffinity"].GetInt() : 0;
  params->route_target_ = (doc.FindMember("target") != doc.MemberEnd())? doc["target"].GetString() : "";
  params->window_type_ = (doc.FindMember("windowType") != doc.MemberEnd())? doc["windowType"].GetString() : "";
  return params;
}
