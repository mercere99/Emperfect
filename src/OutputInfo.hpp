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

class OutputInfo {
private:
  enum Detail {
    ERROR = 0, // Error; detail level unknown.
    NONE,      // No output.
    PERCENT,   // Percentage of passed cases overall (e.g., "60%")
    SCORE,     // Number of points earned overall (e.g. "70 / 100")
    SUMMARY,   // Pass/fail status only for all (visible and hidden) test cases
    STUDENT,   // Details about failed visible cases; pass/fail status for hidden
    TEACHER,   // Detailed information about all failed test cases.
    FULL,      // Detailed information about all cases, including those passed.
    DEBUG      // Extra details (including parsing) for all cases.
  };

  std::string filename;  // If filename is empty, use std::cout
  Detail detail = Detail::STUDENT;
  std::string type = "";
  std::string link_to = ""; // Should links in this file go to another (typically more detailed) file?

  emp::Ptr<std::ostream> file_ptr = nullptr;

public:
  ~OutputInfo() {
    if (filename.size()) file_ptr.Delete();
  }

  const std::string & GetFilename() const { return filename; }
  const std::string & GetType() const { return type; }

  bool IsHTML() const { return type == "html"; }
  bool IsText() const { return type == "txt"; }

  bool HasPercent() const { return detail >= PERCENT; }       // Print percent score at end of file?
  bool HasScore() const { return detail >= SCORE; }           // Print numerical score at end of file?
  bool HasSummary() const { return detail >= SUMMARY; }       // Print result summary at end of file?
  bool HasResults() const { return detail >= STUDENT; }       // Print case-by-case pass/fail result?
  bool HasFailedDetails() const { return detail >= STUDENT; } // Print code for visible failed cases?
  bool HasHiddenDetails() const { return detail >= TEACHER; } // Print code for hidden failed cases?
  bool HasPassedDetails() const { return detail >= FULL; }    // Print code for passed cases?
  bool HasDebug() const { return detail >= DEBUG; }           // Print additional debug data?
  
  bool HasLink() const { return link_to.size(); }             // Send links to a different file?

  std::ostream & GetFile() {
    if (file_ptr.IsNull()) { InitFile(); }
    return *file_ptr;
  }
  const std::string & GetLinkFile() const { return link_to; }

  void InitFile() {
    if (filename.size()) file_ptr = emp::NewPtr<std::ofstream>(filename);
    else {
      if (type.empty()) type = "txt";
      file_ptr = &std::cout;
    }

    // If we are not outputting at least the summary, don't put a header on the file.
    if (!HasSummary()) return;

    std::string header = "";

    switch (detail) {
      case SUMMARY: header = "Autograde Summary"; break;
      case STUDENT: header = "Autograde Results"; break;
      case TEACHER: header = "Autograde Results (Instructor Eyes Only)"; break;
      case FULL: header = "Autograde Results (All details)"; break;
      case DEBUG: header = "Autograde Results (DEBUG mode)"; break;
      default:
        emp::notify::Error("Disallowed detail level: ", detail);
    }

    if (type == "html") {
      *file_ptr << "<h1>" << header << "</h1>\n\n";
    } else {
      *file_ptr << header << "\n\n";
    }
  }

  void SetFilename(const std::string & _in) {
    emp::notify::TestError(file_ptr, "Cannot change filename once output file is used. (new name=", _in, ")");
    filename = _in;

    // If we don't have a type, use file extension to set one.
    if (type.empty()) {
      size_t dot_pos = filename.rfind(".");
      std::string extension = filename.substr(dot_pos+1, filename.size() - dot_pos-1);
      SetType(extension);
    }
  }
  void SetDetail(const std::string & level) { detail = NameToDetail(level); }
  void SetType(const std::string & _in) {
    type = _in;
    if (type == "htm") type = "html";
    if (type != "html" && type != "txt") {
      emp::notify::Warning("Unkown type '", type, "'; using TEXT.");
      type = "txt";
    }
  }
  void SetLinkFile(const std::string & _in) { link_to = _in; }

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

  void PrintDebug(std::ostream & out=std::cout) {
    out << "  Target: ";
    if (filename != "") out << "file '" << filename << "'; ";
    else out << "standard out; ";

    out << "Detail: " << DetailToName(detail)
        << "; Encoding: " << type << std::endl;
  }
};

#endif
