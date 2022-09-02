################################################################################
#
# GLM - OpenGL Mathematics
#
################################################################################

option(GLM_TEST_ENABLE "Disable" OFF)
include(FetchContent)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        bf71a834948186f4097caa076cd2663c69a10e1e # release-0.9.9.8
)
FetchContent_MakeAvailable(glm)

include_directories(SYSTEM ${glm_SOURCE_DIR})
