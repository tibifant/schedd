solution "schedd"
  
  editorintegration "On"
  platforms { "x64", "ARM64" }

  if (_ACTION == "gmake" or _ACTION == "gmake2") then
    configurations { "Release", "Debug", "ReleaseClang", "DebugClang" }
    linkgroups "On"
    filter { "configurations:*Clang" }
    toolset "clang"
    filter { }
  elseif os.target() == "macosx" then
    configurations { "Release", "Debug" }
    toolset "clang"
  else
    configurations { "Release", "Debug", "ReleaseClang", "DebugClang" }
  end

dofile "backend/project.lua"
