#ifndef _FLUTTER_DYNAMIC_LOADER_
#define _FLUTTER_DYNAMIC_LOADER_

#include <flutter/dart_project.h>
#include <flutter/flutter_view_controller.h>

#include <string>
#include <dlfcn.h>
#include <stdexcept>

#include "logger.h"

class DynamicLoader
{
public:
  static constexpr int ERROR_BUF_SIZE = 256;

  DynamicLoader():m_handle(nullptr), m_flags(0) {};
  DynamicLoader(std::string filename, int flags);
  DynamicLoader(const DynamicLoader& src) = delete;
  virtual ~DynamicLoader();

  void Open();
  void Open(std::string filename, int flags);
  void Close();

  void* Lookup(const char* symbolStr);

  void ThrowException(const char* errorStr);

  bool IsLoaded() { return (m_handle != nullptr); };
private:
  std::string m_fullPath;
  int         m_flags;
  void*       m_handle;

};

class EmbedderLoader : public DynamicLoader
{
public:
  EmbedderLoader();

  std::string GetFlutterRuntimePath();
  flutter::FlutterViewController* CreateViewController(
                                    const flutter::FlutterViewController::ViewProperties& view_properties,
                                    const flutter::DartProject& project);
};

class WebosInterfaceLoader : public DynamicLoader
{
public:
  WebosInterfaceLoader();

  void RegisterPlugins(flutter::PluginRegistry* registry);
};

#endif  // _FLUTTER_DYNAMIC_LOADER_
