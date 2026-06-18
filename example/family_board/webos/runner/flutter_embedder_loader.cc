#include <memory.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

#include "flutter_embedder_loader.h"
#include "settings.h"

using namespace flutter;

DynamicLoader::DynamicLoader(std::string filename, int flags)
  : m_fullPath(filename),
    m_flags(flags),
    m_handle(nullptr)
{
}

DynamicLoader::~DynamicLoader()
{
  Close();
}

void DynamicLoader::Open()
{
  LOG_INFO("Loading Shared Library: %s", m_fullPath.c_str());
  m_handle = dlopen(m_fullPath.c_str(), m_flags);


  if (0 == m_handle)
  {
    ThrowException(dlerror());
  }

  dlerror();
}

void DynamicLoader::Open(std::string filename, int flags)
{
  m_fullPath = filename;
  m_flags = flags;

  Open();
}

void DynamicLoader::Close()
{
  if (nullptr != m_handle)
  {
    dlclose(m_handle);
  }
}

void* DynamicLoader::Lookup(const char* symbolStr)
{
  void *targetFunc = nullptr;
  targetFunc = dlsym(m_handle, symbolStr);

  if (0 == targetFunc)
  {
    ThrowException(dlerror());
  }

  return targetFunc;
}

void DynamicLoader::ThrowException(const char* errorStr)
{
  char errorBuf[ERROR_BUF_SIZE];
  memset(errorBuf, '\0', sizeof(errorBuf));

  snprintf(errorBuf, sizeof(errorBuf), "%s\n", errorStr);
  throw std::runtime_error(errorBuf);

}

EmbedderLoader::EmbedderLoader()
    :DynamicLoader()
{
    try {
      std::string pathEmbedder = GetFlutterRuntimePath();
      Open(pathEmbedder, RTLD_LAZY|RTLD_GLOBAL|RTLD_NODELETE);
    } catch (std::runtime_error& e) {
      LOG_ERROR("%s", e.what());
    }
}

std::string EmbedderLoader::GetFlutterRuntimePath()
{
  Settings& settings = Settings::getInstance();

  // TARGET_SUFFIX is defined in CMakeLists.txt
  const std::string embedderName = "libflutter_elinux_" +
                                    settings.getDisplayBackend() + ".so";

  std::string bundlePath = settings.getBundlePath() + "/lib/" + embedderName;

  if (access((bundlePath).c_str(), F_OK) == 0)
    return bundlePath;


  const std::string runtimeBasePath = "/usr/lib/flutter/";
  std::string runtimeMode = settings.getRuntimeMode();
  std::string frameworkVersion = settings.getFrameworkVersion();

  std::string systemPath = runtimeBasePath +
                            frameworkVersion + "/" +
                            runtimeMode + "/" +
                            embedderName;

  // TODO: check systemPath existency
  return systemPath;
}

FlutterViewController* EmbedderLoader::CreateViewController(
                                const flutter::FlutterViewController::ViewProperties& view_properties,
                                const DartProject& project)
{
  if (!IsLoaded()) return nullptr;

  flutter::FlutterViewController* (*_CreateViewController)(
            const FlutterViewController::ViewProperties& view_properties,
            const DartProject& project) = nullptr;
  try {
    _CreateViewController = reinterpret_cast<FlutterViewController* (*)(
                                const flutter::FlutterViewController::ViewProperties& view_properties,
                                const flutter::DartProject& project)>
                                    (Lookup("WrapperCreateFlutterController"));
  } catch (std::runtime_error& e) {
    LOG_ERROR("%s", e.what());
  }

  if (!_CreateViewController) {
    LOG_ERROR("empty _CreateViewController");
    return nullptr;
  }

  return _CreateViewController(view_properties, project);
}

WebosInterfaceLoader::WebosInterfaceLoader()
    :DynamicLoader()
{
  const std::string interfaceLib = "/lib/libwebos_plugin_interface.so";
  Settings& settings = Settings::getInstance();
  std::string bundlePath = settings.getBundlePath() + interfaceLib;
    try {
      Open(bundlePath, RTLD_LAZY);
    } catch (std::runtime_error& e) {
      LOG_ERROR("%s", e.what());
    }
}

void WebosInterfaceLoader::RegisterPlugins(PluginRegistry* registry)
{
  if (!IsLoaded()) return;

  void (*_RegisterPlugins)(flutter::PluginRegistry* registry) = nullptr;

  try {
    _RegisterPlugins = reinterpret_cast<void (*)(PluginRegistry* registry)>
                                    (Lookup("RegisterPlugins"));
  } catch (std::runtime_error& e) {
    LOG_ERROR("%s", e.what());
  }

  if (!_RegisterPlugins) return;

  _RegisterPlugins(registry);

  return;
}

