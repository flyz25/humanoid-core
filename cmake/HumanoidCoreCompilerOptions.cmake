add_library(humanoid_core_options INTERFACE)
add_library(humanoid::compiler_options ALIAS humanoid_core_options)
set_target_properties(humanoid_core_options PROPERTIES EXPORT_NAME compiler_options)

target_compile_features(humanoid_core_options INTERFACE cxx_std_17)

function(humanoid_core_configure_target target_name)
  target_link_libraries(${target_name} PUBLIC humanoid::compiler_options)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
      ${target_name}
      PRIVATE -Wall
              -Wextra
              -Wpedantic
              -Wconversion
              -Wsign-conversion)

    if(HUMANOID_CORE_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  elseif(MSVC)
    target_compile_options(${target_name} PRIVATE /W4)

    if(HUMANOID_CORE_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE /WX)
    endif()
  endif()
endfunction()
