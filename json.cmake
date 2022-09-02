################################################################################
#
# JSON - NLohmann JSON Parser Library
#
################################################################################

include(FetchContent)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        bc889af  # v3.11.2 (2022-08-12)
  SOURCE_DIR     ${FETCHCONTENT_BASE_DIR}/json
)
FetchContent_MakeAvailable(json)

message("JSON SOURCE : ${json_SOURCE_DIR}")

include_directories(SYSTEM ${json_SOURCE_DIR}/include)
