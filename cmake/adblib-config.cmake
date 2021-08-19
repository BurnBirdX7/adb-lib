# Package configuration file

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
if (DEFINED CMAKE_BUILD_TYPE)
    include(${SELF_DIR}/${CMAKE_BUILD_TYPE}/adblib.cmake)
else()
    include(${SELF_DIR}/Debug/adblib.cmake)
endif()