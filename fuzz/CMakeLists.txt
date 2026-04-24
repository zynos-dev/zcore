if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "ZCORE_BUILD_FUZZERS requires a Clang compiler toolchain.")
endif()

if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  message(FATAL_ERROR "ZCORE_BUILD_FUZZERS currently requires clang++ (non-clang-cl) so libFuzzer sanitizer runtime flags are linked correctly.")
endif()

if(WIN32)
  set(_zcore_default_fuzz_sanitizers "fuzzer,address")
else()
  set(_zcore_default_fuzz_sanitizers "fuzzer,address,undefined")
endif()

set(
  ZCORE_FUZZ_SANITIZERS
  "${_zcore_default_fuzz_sanitizers}"
  CACHE STRING
  "Comma-separated sanitizer set for local fuzz targets"
)

if(WIN32)
  get_filename_component(_zcore_clang_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(_zcore_clang_root_dir "${_zcore_clang_bin_dir}" DIRECTORY)
  set(_zcore_clang_runtime_dlls
    libc++.dll
    libc++abi.dll
    libunwind.dll
    libwinpthread-1.dll
    libgcc_s_seh-1.dll
  )
  file(GLOB _zcore_asan_runtime_dlls "${_zcore_clang_root_dir}/lib/clang/*/lib/windows/clang_rt.asan_dynamic-*.dll")
endif()

function(zcore_add_fuzz_target target_name source_file)
  add_executable(${target_name} ${source_file})
  target_link_libraries(${target_name} PRIVATE zcore::zcore)
  target_compile_options(
    ${target_name}
    PRIVATE
      "-fsanitize=${ZCORE_FUZZ_SANITIZERS}"
      -fno-omit-frame-pointer
      -g
  )
  target_link_options(
    ${target_name}
    PRIVATE
      "-fsanitize=${ZCORE_FUZZ_SANITIZERS}"
  )

  if(WIN32)
    set_property(TARGET ${target_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    target_compile_definitions(
      ${target_name}
      PRIVATE
        _ITERATOR_DEBUG_LEVEL=0
        _HAS_ITERATOR_DEBUGGING=0
    )

    foreach(_zcore_runtime_dll IN LISTS _zcore_clang_runtime_dlls)
      set(_zcore_runtime_src "${_zcore_clang_bin_dir}/${_zcore_runtime_dll}")
      if(EXISTS "${_zcore_runtime_src}")
        add_custom_command(
          TARGET ${target_name}
          POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_zcore_runtime_src}"
            "$<TARGET_FILE_DIR:${target_name}>"
        )
      endif()
    endforeach()

    foreach(_zcore_asan_dll IN LISTS _zcore_asan_runtime_dlls)
      add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_zcore_asan_dll}"
          "$<TARGET_FILE_DIR:${target_name}>"
      )
    endforeach()
  endif()
endfunction()

zcore_add_fuzz_target(zcore_fuzz_utf8 utf8_fuzz.cpp)
zcore_add_fuzz_target(zcore_fuzz_hash_map hash_map_fuzz.cpp)
zcore_add_fuzz_target(zcore_fuzz_buffered_io buffered_io_fuzz.cpp)
zcore_add_fuzz_target(zcore_fuzz_string string_fuzz.cpp)
