file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

add_library(anm STATIC ${SOURCES} ${HEADERS})

target_include_directories(
  anm PUBLIC $<INSTALL_INTERFACE:include>
             $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (OPENBLACK_CLANG_TIDY_CHECKS)
  # FIXME(bwrsandman) MSVC is throwing false errors about exceptions being disabled
  if (NOT MSVC AND CLANG_TIDY)
    set_target_properties(anm PROPERTIES CXX_CLANG_TIDY ${CLANG_TIDY})
  else ()
    message("Clang-tidy checks requested but unavailable")
  endif ()
endif ()

if (MSVC)
  target_compile_definitions(anm PRIVATE "_HAS_EXCEPTIONS=0")
  target_compile_options(anm PRIVATE /W4 /WX "/EHs-c-")
else ()
  target_compile_options(
    anm PRIVATE -Wall -Wextra -pedantic -Werror -fno-exceptions
  )
endif ()

set_property(TARGET anm PROPERTY FOLDER "components")
