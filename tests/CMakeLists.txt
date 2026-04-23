include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  URL_HASH SHA256=f179ec217f9b3b3f3c6e8b02d3e7eda997b49e4ce26d6b235c9053bec9c0bf9f
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(CMAKE_CXX_CLANG_TIDY)
  foreach(_zcore_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
    if(TARGET ${_zcore_gtest_target})
      set_target_properties(${_zcore_gtest_target} PROPERTIES CXX_CLANG_TIDY "")
    endif()
  endforeach()
endif()

set(
  ZCORE_TEST_SOURCES
  cpu_features_test.cpp
  intrinsics_test.cpp
  foundation_aliases_test.cpp
  file_header_docs_test.cpp
  public_include_surface_test.cpp
  hash_test.cpp
  hash_customization_test.cpp
  flags_test.cpp
  array_test.cpp
  inline_storage_test.cpp
  inline_buffer_test.cpp
  fixed_bit_set_test.cpp
  fixed_vector_test.cpp
  vector_test.cpp
  deque_test.cpp
  btree_map_test.cpp
  btree_set_test.cpp
  hash_map_test.cpp
  hash_set_test.cpp
  math_test.cpp
  fixed_string_test.cpp
  string_test.cpp
  utf8_test.cpp
  duration_test.cpp
  instant_test.cpp
  timestamp_test.cpp
  sequence_number_test.cpp
  clock_test.cpp
  thread_id_test.cpp
  thread_test.cpp
  thread_local_test.cpp
  thread_bound_test.cpp
  synchronized_test.cpp
  spsc_ring_buffer_test.cpp
  mpsc_ring_buffer_test.cpp
  atomic_test.cpp
  mutex_test.cpp
  rw_lock_test.cpp
  spin_lock_test.cpp
  condition_variable_test.cpp
  endian_test.cpp
  byte_buffer_test.cpp
  buffered_reader_test.cpp
  buffered_writer_test.cpp
  io_interfaces_test.cpp
  strong_typedef_test.cpp
  id_test.cpp
  handle_test.cpp
  type_id_test.cpp
  type_info_test.cpp
  version_test.cpp
  abi_version_test.cpp
  interface_id_test.cpp
  plugin_descriptor_test.cpp
  load_result_test.cpp
  allocator_test.cpp
  arena_test.cpp
  owned_test.cpp
  shared_test.cpp
  borrow_test.cpp
  borrow_mut_test.cpp
  non_null_test.cpp
  slice_test.cpp
  string_view_test.cpp
  log_level_test.cpp
  log_sink_test.cpp
  trace_event_test.cpp
  trace_sink_test.cpp
  scope_guard_test.cpp
  defer_test.cpp
  function_test.cpp
  any_test.cpp
  error_info_test.cpp
  extension_policy_test.cpp
  option_test.cpp
  error_result_status_test.cpp
  error_handler_test.cpp
)

add_executable(
  zcore_tests
  ${ZCORE_TEST_SOURCES}
)
target_link_libraries(zcore_tests PRIVATE zcore::zcore GTest::gtest_main)

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
        TARGET zcore_tests
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_zcore_runtime_src}"
          "$<TARGET_FILE_DIR:zcore_tests>"
      )
    endif()
  endforeach()
endif()

include(GoogleTest)
gtest_add_tests(
  TARGET zcore_tests
  SOURCES
    ${ZCORE_TEST_SOURCES}
)
