################################################################################
#
# VOLK - Meta-loader for Vulkan
#
################################################################################

option(VOLK_TEST_ENABLE "Disable" OFF)
include(FetchContent)
FetchContent_Declare(
  volk
  GIT_REPOSITORY https://github.com/zeux/volk.git
  GIT_TAG        68b453c  # 2022-06-01
  SOURCE_DIR     ${FETCHCONTENT_BASE_DIR}/volk/volk
)
FetchContent_MakeAvailable(volk)

add_compile_definitions(VK_NO_PROTOTYPES)
include_directories(SYSTEM ${volk_SOURCE_DIR}/..)
