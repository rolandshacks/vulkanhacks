################################################################################
#
# GLM - OpenGL Mathematics
#
################################################################################

option(STB_TEST_ENABLE "Disable" OFF)
include(FetchContent)
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        5ba0baaa269b3fd681828e0e3b3ac0f1472eaf40 # 2021-07-26
  SOURCE_DIR     ${FETCHCONTENT_BASE_DIR}/stb/stb
)
FetchContent_MakeAvailable(stb)

include_directories(SYSTEM ${stb_SOURCE_DIR}/..)
