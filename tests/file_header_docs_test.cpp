/**************************************************************************/
/*  file_header_docs_test.cpp                                             */
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
 * @file tests/file_header_docs_test.cpp
 * @brief Unit test enforcing file banner and Doxygen summary blocks.
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

void NormalizeLineEndings(std::string& text) {
  text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
}

[[nodiscard]] bool HasTargetExtension(const std::filesystem::path& path) {
  const std::string extension = path.extension().string();
  return extension == ".hpp" || extension == ".cpp";
}

TEST(FileHeaderDocsTest, AllHeadersAndSourcesHaveBannerAndDoxygenSummary) {
  const std::filesystem::path projectRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
  const std::filesystem::path includeDir = projectRoot / "include";
  const std::filesystem::path srcDir = projectRoot / "src";

  // Strict doc banner enforcement applies to public and production implementation code.
  // Test sources are intentionally excluded to reduce internal iteration friction.
  std::vector<std::filesystem::path> scanDirs;
  scanDirs.push_back(includeDir);
  if (std::filesystem::exists(srcDir)) {
    scanDirs.push_back(srcDir);
  }

  for (const std::filesystem::path& dir : scanDirs) {
    ASSERT_TRUE(std::filesystem::exists(dir)) << "Missing directory: " << dir.string();

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }

      const std::filesystem::path& filePath = entry.path();
      if (!HasTargetExtension(filePath)) {
        continue;
      }

      std::string contents = ReadAllText(filePath);
      ASSERT_FALSE(contents.empty()) << "Unable to read: " << filePath.string();
      NormalizeLineEndings(contents);

      EXPECT_TRUE(contents.rfind("/**************************************************************************/\n", 0) == 0)
          << "Missing banner header in: " << filePath.string();
      EXPECT_NE(contents.find("This file is part of the zcore project."), std::string::npos)
          << "Missing project ownership text in: " << filePath.string();
      EXPECT_NE(contents.find("@file "), std::string::npos) << "Missing @file block in: " << filePath.string();
      EXPECT_NE(contents.find("@brief "), std::string::npos) << "Missing @brief block in: " << filePath.string();
      EXPECT_NE(contents.find("@par Usage"), std::string::npos) << "Missing usage section in: " << filePath.string();
    }
  }
}

}  // namespace
