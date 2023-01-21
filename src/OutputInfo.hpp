/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  OutputInfo.hpp
 *  @brief Tracking for each output.
 */

#ifndef EMPERFECT_OUTPUT_INFO_HPP
#define EMPERFECT_OUTPUT_INFO_HPP

#include <string>

#include "emp/base/notify.hpp"

struct OutputInfo {
  enum Detail {
    NONE = 0,  // No output.
    PERCENT,   // Percentage of passed cases overall (e.g., "60%")
    SCORE,     // Number of points earned overall (e.g. "70 / 100")
    SUMMARY,   // Pass/fail status only for all (visible and hidden) test cases
    STUDENT,   // Details about failed visible cases; pass/fail status for hidden
    TEACHER,   // Detailed information about all failed test cases.
    FULL,      // Detailed information about all cases, including those passed.
    DEBUG      // Extra details (including parsing) for all cases.
  };

  std::string filename;  // If filename is empty, use std::cout
  Detail detail = STUDENT;
  std::string type;

  void SetDetail(const std::string & level) {
    if (level == "none") detail = NONE;
    else if (level == "percent") detail = PERCENT;
    else if (level == "score") detail = SCORE;
    else if (level == "summary") detail = SUMMARY;
    else if (level == "student") detail = STUDENT;
    else if (level == "teacher") detail = TEACHER;
    else if (level == "full") detail = FULL;
    else if (level == "debug") detail = DEBUG;
    else emp::notify::Error("Tying to set unknown detail level '", level, "'.");
  }

  // Make sure we have a type set for this output.
  void FinalizeType() {
    if (type.empty()) {
      // If filename HAS been set, see if it has a useful suffix.
      if (!filename.empty()) {
        size_t dot_pos = filename.rfind(".");
        std::string extension = filename.substr(dot_pos, filename.size() - dot_pos);
        if (extension == ".html" || extension == ".htm") type = "html";
        else type = "txt";
      }
      else type = "txt";
    }
  }
};

#endif
