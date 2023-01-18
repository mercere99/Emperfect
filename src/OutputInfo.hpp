/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  OutputInfo.hpp
 *  @brief Tracking for each output.
 */

#ifndef EMPERFECT_TESTCASE_HPP
#define EMPERFECT_TESTCASE_HPP

#include <string>

struct OutputInfo {
  enum Detail {
    SILENT = 0, // No output.
    SUMMARY,    // Output only a count of cases passed/failed.
    NORMAL,     // Provide count of passed cases, but details on failed cases.
    VERBOSE,    // Provide details on all cases, including those passed.
    DEBUG       // Extra details (including parsing) for all cases.
  };

  std::string filename;  // If filename is empty, use std::cout
  Detail visible_detail = NORMAL;
  Detail hidden_detail = SUMMARY;
};

#endif
