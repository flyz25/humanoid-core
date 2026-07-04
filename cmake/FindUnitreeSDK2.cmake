# Finds Unitree SDK2 from an explicit root or the repository submodule.
#
# Supported inputs:
#   UNITREE_SDK2_ROOT - Unitree SDK2 source or install root.
#   ENV{UNITREE_SDK2_ROOT} - Environment override.
#
# Output:
#   UnitreeSDK2_FOUND
#   UnitreeSDK2_ROOT
#   UnitreeSDK2_VERSION
#   unitree_sdk2 imported target

include(FindPackageHandleStandardArgs)

set(_unitree_sdk2_roots)

if(UNITREE_SDK2_ROOT)
  list(APPEND _unitree_sdk2_roots "${UNITREE_SDK2_ROOT}")
endif()

if(DEFINED ENV{UNITREE_SDK2_ROOT})
  list(APPEND _unitree_sdk2_roots "$ENV{UNITREE_SDK2_ROOT}")
endif()

if(PROJECT_SOURCE_DIR)
  list(APPEND _unitree_sdk2_roots "${PROJECT_SOURCE_DIR}/third_party/unitree_sdk2")
endif()

if(CMAKE_CURRENT_LIST_DIR)
  get_filename_component(_humanoid_find_module_dir "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
  get_filename_component(_humanoid_repo_root "${_humanoid_find_module_dir}/.." ABSOLUTE)
  list(APPEND _unitree_sdk2_roots "${_humanoid_repo_root}/third_party/unitree_sdk2")
endif()

list(REMOVE_DUPLICATES _unitree_sdk2_roots)

set(_unitree_sdk2_arch "${CMAKE_SYSTEM_PROCESSOR}")
set(_unitree_sdk2_candidate_roots)

foreach(_root IN LISTS _unitree_sdk2_roots)
  if(_root AND EXISTS "${_root}")
    get_filename_component(_abs_root "${_root}" ABSOLUTE)
    list(APPEND _unitree_sdk2_candidate_roots "${_abs_root}")
  endif()
endforeach()

foreach(_root IN LISTS _unitree_sdk2_candidate_roots)
  if(NOT UnitreeSDK2_ROOT)
    set(_include_dir "${_root}/include")
    set(_unitree_library "${_root}/lib/${_unitree_sdk2_arch}/libunitree_sdk2.a")
    set(_ddsc_library "${_root}/thirdparty/lib/${_unitree_sdk2_arch}/libddsc.so")
    set(_ddscxx_library "${_root}/thirdparty/lib/${_unitree_sdk2_arch}/libddscxx.so")
    set(_dds_include_dir "${_root}/thirdparty/include")
    set(_ddsxx_include_dir "${_root}/thirdparty/include/ddscxx")

    if(NOT EXISTS "${_unitree_library}")
      set(_unitree_library "${_root}/lib/libunitree_sdk2.a")
    endif()

    if(NOT EXISTS "${_ddsc_library}")
      set(_ddsc_library "${_root}/lib/libddsc.so")
    endif()

    if(NOT EXISTS "${_ddscxx_library}")
      set(_ddscxx_library "${_root}/lib/libddscxx.so")
    endif()

    if(NOT EXISTS "${_dds_include_dir}")
      set(_dds_include_dir "${_root}/include")
    endif()

    if(NOT EXISTS "${_ddsxx_include_dir}")
      set(_ddsxx_include_dir "${_root}/include/ddscxx")
    endif()

    if(EXISTS "${_include_dir}/unitree/robot/g1/loco/g1_loco_client.hpp"
       AND EXISTS "${_unitree_library}"
       AND EXISTS "${_ddsc_library}"
       AND EXISTS "${_ddscxx_library}")
      set(UnitreeSDK2_ROOT "${_root}" CACHE PATH "Unitree SDK2 root" FORCE)
      set(UnitreeSDK2_INCLUDE_DIR "${_include_dir}" CACHE PATH "Unitree SDK2 include directory" FORCE)
      set(UnitreeSDK2_LIBRARY "${_unitree_library}" CACHE FILEPATH "Unitree SDK2 library" FORCE)
      set(UnitreeSDK2_DDSC_LIBRARY "${_ddsc_library}" CACHE FILEPATH "Unitree SDK2 Cyclone DDS C library" FORCE)
      set(UnitreeSDK2_DDSCXX_LIBRARY "${_ddscxx_library}" CACHE FILEPATH "Unitree SDK2 Cyclone DDS C++ library" FORCE)
      set(UnitreeSDK2_DDS_INCLUDE_DIR "${_dds_include_dir}" CACHE PATH "Unitree SDK2 DDS include directory" FORCE)
      set(UnitreeSDK2_DDSXX_INCLUDE_DIR "${_ddsxx_include_dir}" CACHE PATH "Unitree SDK2 DDS C++ include directory" FORCE)
    endif()
  endif()
endforeach()

if(EXISTS "${UnitreeSDK2_ROOT}/.git")
  execute_process(
    COMMAND git -C "${UnitreeSDK2_ROOT}" describe --tags --exact-match
    OUTPUT_VARIABLE _unitree_sdk2_tag
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(_unitree_sdk2_tag)
  set(UnitreeSDK2_VERSION "${_unitree_sdk2_tag}")
else()
  set(UnitreeSDK2_VERSION "unknown")
endif()

set(_UnitreeSDK2_THREADS_OK TRUE)
if(CMAKE_C_COMPILER_LOADED OR CMAKE_CXX_COMPILER_LOADED)
  find_package(Threads QUIET)
  set(_UnitreeSDK2_THREADS_OK "${Threads_FOUND}")
endif()

find_package_handle_standard_args(
  UnitreeSDK2
  REQUIRED_VARS
    UnitreeSDK2_ROOT
    UnitreeSDK2_INCLUDE_DIR
    UnitreeSDK2_LIBRARY
    UnitreeSDK2_DDSC_LIBRARY
    UnitreeSDK2_DDSCXX_LIBRARY
    _UnitreeSDK2_THREADS_OK
  VERSION_VAR UnitreeSDK2_VERSION)

if(UnitreeSDK2_FOUND)
  if(NOT TARGET ddsc)
    add_library(ddsc SHARED IMPORTED GLOBAL)
    set_target_properties(
      ddsc
      PROPERTIES IMPORTED_LOCATION "${UnitreeSDK2_DDSC_LIBRARY}"
                 IMPORTED_NO_SONAME TRUE
                 INTERFACE_INCLUDE_DIRECTORIES "${UnitreeSDK2_DDS_INCLUDE_DIR}")
    if(TARGET Threads::Threads)
      target_link_libraries(ddsc INTERFACE Threads::Threads)
    endif()
  endif()

  if(NOT TARGET ddscxx)
    add_library(ddscxx SHARED IMPORTED GLOBAL)
    set_target_properties(
      ddscxx
      PROPERTIES IMPORTED_LOCATION "${UnitreeSDK2_DDSCXX_LIBRARY}"
                 IMPORTED_NO_SONAME TRUE
                 INTERFACE_INCLUDE_DIRECTORIES "${UnitreeSDK2_DDS_INCLUDE_DIR};${UnitreeSDK2_DDSXX_INCLUDE_DIR}")
    if(TARGET Threads::Threads)
      target_link_libraries(ddscxx INTERFACE Threads::Threads)
    endif()
  endif()

  if(NOT TARGET unitree_sdk2)
    add_library(unitree_sdk2 STATIC IMPORTED GLOBAL)
    set_target_properties(
      unitree_sdk2
      PROPERTIES IMPORTED_LOCATION "${UnitreeSDK2_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${UnitreeSDK2_INCLUDE_DIR}")
    target_link_libraries(unitree_sdk2 INTERFACE ddsc ddscxx)
    if(TARGET Threads::Threads)
      target_link_libraries(unitree_sdk2 INTERFACE Threads::Threads)
    endif()
  endif()
endif()

mark_as_advanced(
  UnitreeSDK2_INCLUDE_DIR
  UnitreeSDK2_LIBRARY
  UnitreeSDK2_DDSC_LIBRARY
  UnitreeSDK2_DDSCXX_LIBRARY
  UnitreeSDK2_DDS_INCLUDE_DIR
  UnitreeSDK2_DDSXX_INCLUDE_DIR)
