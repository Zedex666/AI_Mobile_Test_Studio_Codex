# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "AI_Mobile_Test_Studio_Codex_autogen"
  "CMakeFiles\\AI_Mobile_Test_Studio_Codex_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\AI_Mobile_Test_Studio_Codex_autogen.dir\\ParseCache.txt"
  )
endif()
