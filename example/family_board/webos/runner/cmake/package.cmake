cmake_minimum_required(VERSION 3.10)

find_package(PkgConfig)

# common libraries.
#pkg_check_modules(EGL REQUIRED egl)


#pkg_check_modules(GLIB REQUIRED glib-2.0)
#pkg_check_modules(LS2++ REQUIRED luna-service2++>=3)

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
#pkg_check_modules(NYX REQUIRED nyx)
pkg_check_modules(PMLOG REQUIRED PmLogLib)

# requires for supporting external texture plugin.
# OpenGL ES3 are included in glesv2.
#pkg_check_modules(GLES REQUIRED glesv2)
#pkg_check_modules(LIBSYSTEMD REQUIRED libsystemd)
