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
    DEBUG,     // Extra details (including parsing) for all cases.
    ERROR      // Error; detail level unknown.
  };

  std::string filename;  // If filename is empty, use std::cout
  Detail detail = Detail::STUDENT;
  std::string type;

  Detail NameToDetail(std::string level) {
    level = emp::to_lower(level);
    if (level == "none") return Detail::NONE;
    else if (level == "percent") return Detail::PERCENT;
    else if (level == "score") return Detail::SCORE;
    else if (level == "summary") return Detail::SUMMARY;
    else if (level == "student") return Detail::STUDENT;
    else if (level == "teacher") return Detail::TEACHER;
    else if (level == "full") return Detail::FULL;
    else if (level == "debug") return Detail::DEBUG;
    emp::notify::Error("Tying to set unknown detail level '", level, "'.");
    return Detail::ERROR;
  }

  std::string DetailToName(Detail detail) {
    switch (detail) {
      case Detail::NONE: return "NONE"; break;
      case Detail::PERCENT: return "PERCENT"; break;
      case Detail::SCORE: return "SCORE"; break;
      case Detail::SUMMARY: return "SUMMARY"; break;
      case Detail::STUDENT: return "STUDENT"; break;
      case Detail::TEACHER: return "TEACHER"; break;
      case Detail::FULL: return "FULL"; break;
      case Detail::DEBUG: return "DEBUG"; break;
      default:
        return "ERROR";
    }
  }

  void SetDetail(const std::string & level) { detail = NameToDetail(level); }

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

  void PrintDebug(std::ostream & out=std::cout) {
    out << "  Target: ";
    if (filename != "") out << "file '" << filename << "'; ";
    else out << "standard out; ";

    out << "Detail: " << DetailToName(detail)
        << "; Encoding: " << type << std::endl;
  }
};

#endif
