function(zcore_configure_docs)
  find_package(Doxygen REQUIRED)

  set(ZCORE_DOXYGEN_AWESOME_CSS "")
  set(ZCORE_DOXYGEN_HTML_COLORSTYLE "AUTO_LIGHT")
  set(ZCORE_DOXYGEN_FULL_SIDEBAR "YES")

  if(ZCORE_USE_DOXYGEN_AWESOME)
    include(FetchContent)
    FetchContent_Declare(
      doxygen_awesome_css
      URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.4.2.zip
    )
    FetchContent_MakeAvailable(doxygen_awesome_css)

    set(ZCORE_DOXYGEN_AWESOME_CSS "${doxygen_awesome_css_SOURCE_DIR}/doxygen-awesome.css")
    set(ZCORE_DOXYGEN_HTML_COLORSTYLE "LIGHT")
    set(ZCORE_DOXYGEN_FULL_SIDEBAR "NO")
  endif()

  set(ZCORE_DOXYFILE_IN "${PROJECT_SOURCE_DIR}/Docs/Doxyfile.in")
  set(ZCORE_DOXYFILE_OUT "${PROJECT_BINARY_DIR}/Doxyfile")
  set(ZCORE_DOXYGEN_HTML_DIR "${PROJECT_BINARY_DIR}/docs/doxygen/html")
  configure_file("${ZCORE_DOXYFILE_IN}" "${ZCORE_DOXYFILE_OUT}" @ONLY)

  add_custom_target(
    zcore_docs_doxygen
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/docs/doxygen"
    COMMAND ${DOXYGEN_EXECUTABLE} "${ZCORE_DOXYFILE_OUT}"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "Generating zcore API reference with Doxygen"
    VERBATIM
  )

  add_custom_target(
    zcore_docs
    DEPENDS zcore_docs_doxygen
    COMMENT "Generating docs artifacts in build directory (no source-tree sync)"
  )
endfunction()
