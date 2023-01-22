/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  Testcase.hpp
 *  @brief An individual testcase in Emperfect.
 * 
 */

#ifndef EMPERFECT_TESTCASE_HPP
#define EMPERFECT_TESTCASE_HPP

#include "emp/base/vector.hpp"

#include "CheckInfo.hpp"

using string_block_t = emp::vector<std::string>;

class Testcase {
  friend class Emperfect;
private:
  // -- Configured from args --
  std::string name;          // Unique name for this test case.
  double points = 0.0;       // Number of points this test case is worth.

  std::string in_filename;   // Name of file to feed as standard input.
  std::string out_filename;  // Name of file to compare against standard output.
  std::string code_filename; // Name of file with code to test.
  std::string args;          // Command-line arguments.

  bool call_main = true;     // Should main() function be called?
  bool hidden = false;       // Should this test case be seen by students?
  bool match_case = true;    // Does case need to match perfectly in the output?
  bool match_space = true;   // Does whitespace need to match perfectly in the output?

  // -- Configured elsewhere --
  string_block_t code;       // The actual code associated with this test case.
  std::string filename;      // What file is this test case in?
  size_t start_line = 0;     // At which line number is this test case start?
  size_t end_line = 0;       // At which line does this test case end?

  std::vector<CheckInfo> checks;

  // Helper functions

  // Determine how many tests match a particular lambda.
  size_t CountIf(auto test) const {
    return std::count_if(checks.begin(), checks.end(), test);
  }

public:
  size_t GetNumChecks() const { return checks.size(); }
  size_t CountPassed() const {
    return CountIf([](const auto & check){ return check.passed; });
  }
  size_t CountFailed() const {
    return CountIf([](const auto & check){ return !check.passed; });
  }

  bool Passed() const { return CountPassed() == checks.size(); }

  // Test if a check at particular line number passed.
  bool Passed(size_t test_line) const {
    for (const auto & check : checks) {
      if (check.file_pos.GetLine() == test_line) return check.passed;
    }
    return true;  // No check on line == passed.
  }

  double EarnedPoints() const { return Passed() ? points : 0.0; }

  void PrintCode_HTML(std::ostream & out) const {
    out << "<table style=\"background-color:#E3E0CF;\"><tr><td><pre>\n\n";
    std::ifstream source(filename);
    std::string code;
    size_t line = 0;

    // Skip beginning of file.
    while (++line < start_line) std::getline(source, code);
    while (++line < end_line && std::getline(source, code)) {
      const bool highlight = !Passed(line-1);
      if (highlight) out << "<b>";
      out << code << "\n";
      if (highlight) out << "</b>";
    }
    out << "</pre></tr></table>\n";
  }

  void PrintResult_HTML(std::ostream & out, bool is_student=true) const {
    // Notify if the test passed.
    if (Passed()) {
      out << "Test case <span style=\"color: green\"><b>Passed!</b></span><br><br>\n\n";
    }
    else {
      // Failed cases will already have an error noted except for hidden cases in student file.
      if (hidden && is_student) {
        out << "Test case <span style=\"color: red\"><b>Failed.</b></span><br><br>\n";
        return; // No more details.
      }
    
      out << "Source";
      if (!is_student) out << " (starting from line " << start_line << ")";
      out << ":<br><br>\n";

      PrintCode_HTML(out);
    }
  }

  void PrintDebug(std::ostream & out=std::cout) {
    out << "===============\n"
        << "Name..............: " << name << "\n"
        << "Points............: " << points << "\n"
        << "Hidden............: " << (hidden ? "true" : "false") << "\n"
        << "match_case........: " << (match_case ? "true" : "false") << "\n"
        << "match_space.......: " << (match_space ? "true" : "false") << "\n"
        << "call_main.........: " << (call_main ? "true" : "false") << "\n"
        << "Command Line Args.: " << args << "\n"
        << "Input to provide..: " << (in_filename.size() ? in_filename : "(none)") << "\n"
        << "Output to compare.: " << (out_filename.size() ? out_filename : "(none)") << "\n"
        << std::endl;

    // std::string code_filename; // Name of file with code to test.
    // string_block_t code;       // The actual code associated with this test case.
    // std::string filename;      // What file is this test case in?
    // size_t start_line = 0;     // At which line number is this test case start?
    // size_t end_line = 0;       // At which line does this test case end?
    // std::vector<CheckInfo> checks;
  }
};

#endif
