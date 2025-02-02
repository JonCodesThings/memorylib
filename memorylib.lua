workspace "memorylib_workspace"
  configurations { "Debug", "Release" }
  platforms { "x86", "x86_64" }
  location "build"

project "memorylib"
  kind "StaticLib"
  language "C"
  includedirs { "." }
  files { "include/memorylib/*.h", "src/*.c" }