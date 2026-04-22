/**************************************************************************/
/*  public_include_surface_test.cpp                                       */
/**************************************************************************/
/*  zcore                                                                 */
/*  Copyright (c) 2026 Logan Yagadics                                     */
/*  Copyright (c) 2026 zcore Contributors                                 */
/*                                                                        */
/*  This file is part of the zcore project.                               */
/*  Use of this source code is governed by the license defined            */
/*  in the LICENSE file at the root of this repository.                   */
/**************************************************************************/
/**
 * @file tests/public_include_surface_test.cpp
 * @brief Unit tests enforcing canonical public include mapping contracts.
 * @par Usage
 * @code{.sh}
 * cmake --build <build-dir> --target zcore_tests
 * ctest --output-on-failure
 * @endcode
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace {

[[nodiscard]] std::string ReadAllText(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    return {};
  }
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

[[nodiscard]] bool IsHeaderFile(const std::filesystem::path& path) {
  return path.extension() == ".hpp";
}

[[nodiscard]] bool IsTopLevelHeader(const std::string& relativePath) {
  return relativePath.find('/') == std::string::npos;
}

[[nodiscard]] std::map<std::string, std::vector<std::string>> CollectHeadersByBasename(
    const std::filesystem::path& includeRoot) {
  std::map<std::string, std::vector<std::string>> headersByBasename;

  for (const auto& entry : std::filesystem::recursive_directory_iterator(includeRoot)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const std::filesystem::path& headerPath = entry.path();
    if (!IsHeaderFile(headerPath)) {
      continue;
    }

    const std::string relativePath = headerPath.lexically_relative(includeRoot).generic_string();
    const std::string basename = headerPath.filename().string();
    headersByBasename[basename].push_back(relativePath);
  }

  for (auto& [basename, paths] : headersByBasename) {
    (void)basename;
    std::sort(paths.begin(), paths.end());
  }

  return headersByBasename;
}

TEST(PublicIncludeSurfaceTest, DuplicateBasenamesHaveSingleTopLevelFacade) {
  const std::filesystem::path projectRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
  const std::filesystem::path includeRoot = projectRoot / "include" / "zcore";
  ASSERT_TRUE(std::filesystem::exists(includeRoot)) << "Missing include root: " << includeRoot.string();

  const auto headersByBasename = CollectHeadersByBasename(includeRoot);
  for (const auto& [basename, paths] : headersByBasename) {
    if (paths.size() <= 1) {
      continue;
    }

    const std::size_t topLevelCount = static_cast<std::size_t>(std::count_if(
        paths.begin(), paths.end(), [](const std::string& path) { return IsTopLevelHeader(path); }));

    EXPECT_EQ(paths.size(), 2U)
        << "Duplicate basename should map to exactly one facade + one module header: " << basename;
    EXPECT_EQ(topLevelCount, 1U)
        << "Duplicate basename must expose exactly one top-level facade include: " << basename;
  }
}

TEST(PublicIncludeSurfaceTest, FacadeHeaderIncludesItsMappedModuleHeader) {
  const std::filesystem::path projectRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
  const std::filesystem::path includeRoot = projectRoot / "include" / "zcore";
  ASSERT_TRUE(std::filesystem::exists(includeRoot)) << "Missing include root: " << includeRoot.string();

  const auto headersByBasename = CollectHeadersByBasename(includeRoot);
  for (const auto& [basename, paths] : headersByBasename) {
    if (paths.size() != 2) {
      continue;
    }

    const auto topLevelIt = std::find_if(
        paths.begin(), paths.end(), [](const std::string& path) { return IsTopLevelHeader(path); });
    if (topLevelIt == paths.end()) {
      continue;
    }

    const std::string& facadePath = *topLevelIt;
    const std::string modulePath = (paths.front() == facadePath) ? paths.back() : paths.front();

    const std::filesystem::path facadeHeaderPath = includeRoot / std::filesystem::path(facadePath);
    const std::string facadeContents = ReadAllText(facadeHeaderPath);
    ASSERT_FALSE(facadeContents.empty()) << "Unable to read facade header: " << facadeHeaderPath.string();

    const std::string expectedInclude = "#include <zcore/" + modulePath + ">";
    EXPECT_NE(facadeContents.find(expectedInclude), std::string::npos)
        << "Facade does not include mapped module header. Facade: " << facadePath
        << " expected include: " << expectedInclude;
  }
}

}  // namespace
