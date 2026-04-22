add_executable(
  zcore_string_utf8_bench
  string_utf8_benchmark.cpp
)

target_link_libraries(zcore_string_utf8_bench PRIVATE zcore::zcore)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  get_filename_component(_zcore_clang_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  set(_zcore_clang_runtime_dlls
    libc++.dll
    libc++abi.dll
    libunwind.dll
    libwinpthread-1.dll
    libgcc_s_seh-1.dll
  )

  foreach(_zcore_runtime_dll IN LISTS _zcore_clang_runtime_dlls)
    set(_zcore_runtime_src "${_zcore_clang_bin_dir}/${_zcore_runtime_dll}")
    if(EXISTS "${_zcore_runtime_src}")
      add_custom_command(
        TARGET zcore_string_utf8_bench
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_zcore_runtime_src}"
          "$<TARGET_FILE_DIR:zcore_string_utf8_bench>"
      )
    endif()
  endforeach()
endif()
