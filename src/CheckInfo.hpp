/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  CheckInfo.hpp
 *  @brief Information about an individual check within an Emperfect Testcase.
 * 
 */

#ifndef EMPERFECT_CHECK_INFO_HPP
#define EMPERFECT_CHECK_INFO_HPP

#include <string>

#include "emp/base/notify.hpp"

// Parsed information about a given check.
class CheckString {
private:
  std::string test;
  std::string lhs;
  std::string comparator;
  std::string rhs;

public:
  ParsedCheck(std::string _test) : test(_test) {
    emp::notify::TestError(find_any_of(test, 0, "&&", "||") != std::string::npos,
      to_string("Unit test checks do not allow \"&&\" or \"||\"."), filename, line_num);

    auto comp_pos = find_any_of(test, 0, "==", "!=", "<", "<=", ">", ">=");

    emp::notify::TestError(comp_pos != std::string::npos &&
        find_any_of(test, comp_pos+2, "==", "!=", "<", "<=", ">", ">=") != std::string::npos,
        "Unit test checks can have only one comparison.", filename, line_num);

    // Determine which comparison operator we are working with (if any) and the terms being compared.
    if (comp_pos != std::string::npos) {
      comparator += test[comp_pos];
      if (test.at(comp_pos+1) == '=') comparator += "=";

      // Identify the left and right hand sides
      lhs = test.substr(0,comp_pos);
      emp::compress_whitespace(lhs);
      rhs = test.substr(comp_pos+comparator.size());
      emp::compress_whitespace(rhs);
    }
    else {
      lhs = test;
    }
  }
  CheckString(const CheckString &) = default;
  CheckString(CheckString &&) = default;

  CheckString & operator=(const CheckString &) = default;
  CheckString & operator=(CheckString &&) = default;

  const std::string & ToString() const { return test; }
  std::string GetLHS() const { return lhs; }
  std::string GetRHS() const { return rhs; }
  std::string GetComparator() const { return comparator; }
}

struct CheckInfo {
  CheckString test;            // The test string associated with this check.
  std::string filename = "";   // What file is this check found in?
  size_t line_num = 0;         // What line number is this check on?
  std::string lhs_value = "";  // Representation of the value on the left (e.g., "20")
  std::string rhs_value = "";  // Representation of the value on the right (e.g., "21", if x=16)
  bool passed = false;         // Was this check successful?
  bool resolved = false;       // Are we done performing this check?
  std::string message = "";    // Extra message on failure (e.g., "Grade assessments do not align.")

  CheckInfo(const std::string & _test, const std::string & _filename, size_t _line_num)
    : test(_test), filename(_filename), line_num(_line_num) {}
  CheckInfo(const CheckInfo &) = default;

  void PrintResults_HTML(std::ostream & out) const {
    if (passed) return; // No results printed for passed tests.

    // Show the failed code.
    out << "<p>Check <span style=\"color: red\"><b>FAILED</b></span>:<br>\n"
        << "Test: <code>" << test.ToString() << "</code><br><br>\n";

    // If there was a comparison, show results on both sides of it.
    if (test.GetRHS() != "") {
      out << "<table><tr><td>Left side:<td><code>" << test.GetLHS()
          << "</code><td>&nbsp;&nbsp;resolves to:<td><code>" << lhs_value << "</code></tr>\n"
          << "<tr><td>Right side:<td><code>" << test.GetRHS() << "</code><td>&nbsp;&nbsp;resolves to:<td><code>"
          << rhs_value << "</code></tr></table><br>\n";
    }
  }
};

#endif
