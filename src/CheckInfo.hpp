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
#include "emp/bits/BitVector.hpp"

using string_block_t = emp::vector<std::string>;

// Parsed information about a given check.
class CheckString {
private:
  std::string test = "";
  std::string lhs = "";
  std::string comparator = "";
  std::string rhs = "";

public:
  void SetFrom(const std::string & _test, std::string location) {
    test = _test;
    emp::notify::TestError(emp::find_any_of(test, 0, "&&", "||") != std::string::npos,
      location, ": Unit test checks do not allow \"&&\" or \"||\".");

    size_t comp_pos = emp::find_any_of(test, 0, "==", "!=", "<", "<=", ">", ">=");
    bool has_comp = (comp_pos != std::string::npos);

    if (has_comp) {
      // Make sure it doesn't have TWO comparisons.
      emp::notify::TestError(
        emp::find_any_of(test, comp_pos+2, "==", "!=", "<", "<=", ">", ">=") != std::string::npos,
        location, ": Unit test checks can have only one comparison.");

      size_t comp_size = (test[comp_pos+1] == '=') ? 2 : 1;
      comparator = test.substr(comp_pos, comp_size);
      lhs = test.substr(0, comp_pos);
      rhs = test.substr(comp_pos+comp_size);
    }
    else {
      lhs = test; // Whole test is on the left-hand-side.
    }
  }

  const std::string & ToString() const { return test; }
  std::string ToLiteral() const { return emp::to_literal(test); }
  std::string GetLHS() const { return lhs; }
  std::string GetRHS() const { return rhs; }
  std::string GetComparator() const { return comparator; }
  bool HasComp() const { return comparator.size(); }
};

class CheckInfo {
private:
  CheckString test;            // The test string associated with this check.
  std::string location;        // Position in the file where this check is located.
  size_t id;                   // Unique ID for this check.
  string_block_t error_msgs;   // Extra arguments from check to use in error messages.

  emp::vector<std::string> lhs_value;  // Resulting value on left (e.g., "20")
  emp::vector<std::string> rhs_value;  // Resulting value on right (e.g., "21", if rhs is "x+5" and x=16)
  emp::BitVector passed = false;         // Was this check successful?
  emp::vector<std::string> error_out;       // Message from test runner for students.

public:
  CheckInfo(const std::string & check_body, std::string _location, size_t _id)
    : location(_location), id(_id)
  {
    error_msgs = emp::slice(check_body, ',', 256, true, true, true);

    // Split off the test (the first argument) and make sure it's valid.
    emp::notify::TestError(error_msgs.size() == 0, location, ": CHECK cannot be empty.");
    test.SetFrom(error_msgs[0], location);
    error_msgs.erase(error_msgs.begin());
  }
  CheckInfo(const CheckInfo &) = default;

  size_t GetID() const { return id; }
  bool Passed() const { return passed.All(); }
  bool PassedAny() const { return passed.Any(); }

  void PushResult(bool success) { passed.push_back(success); }
  void PushLHSValue(const std::string in_value) { lhs_value.push_back(in_value); }
  void PushRHSValue(const std::string in_value) { rhs_value.push_back(in_value); }
  void PushErrorMsg(const std::string in_value) { error_out.push_back(in_value); }

  std::string ToCPP() const {
    std::stringstream out;

    // Generate code for this test.
    out << "  // CHECK #" << id << "\n"
        << "  {\n"
        << "    auto _emperfect_lhs = " << test.GetLHS() << ";\n";
    if (test.HasComp()) {
      out << "    auto _emperfect_rhs = " << test.GetRHS() << ";\n"
          << "    bool _emperfect_success = (_emperfect_lhs " << test.GetComparator() << " _emperfect_rhs);\n";
    } else {
      out << "    auto _emperfect_rhs = \"N/A\";\n"
          << "    bool _emperfect_success = _emperfect_lhs;\n";
    }
    out << "    _emperfect_passed &= _emperfect_success;\n"
        << "    std::string _emperfect_msg = \"Success!\";\n"
        << "    if (!_emperfect_success) {\n"
        << "      std::stringstream ss;\n";
    for (std::string x : error_msgs) {
      out << "      ss << " << x << ";\n";
    }
    out << "      _emperfect_msg = ss.str();"
        << "    }\n"
        << "    _emperfect_results << \":CHECK: " << id << "\\n\"\n"
        << "                       << \":TEST: \" << " << test.ToLiteral() << " << \"\\n\"\n"
        << "                       << \":RESULT: \" << _emperfect_success << \"\\n\"\n"
        << "                       << \":LHS: \" << _emperfect_lhs << \"\\n\"\n"
        << "                       << \":RHS: \" << _emperfect_rhs << \"\\n\"\n"
        << "                       << \":MSG: \" << _emperfect_msg << \"\\n\\n\";\n"
        << "    _emperfect_check_id++;\n"
        << "  }\n";

    return out.str();
  }

  void PrintResults(OutputInfo & output) const {
    // Loop through all call instances.
    for (size_t i = 0; i < passed.size(); ++i) {
      if (passed[i] && !output.HasPassedDetails()) continue; // No results printed for passed checks?
      PrintResults(output, i);
    }
  }

  void PrintResults(OutputInfo & output, size_t call_id) const {
    std::ostream & out = output.GetFile();

    std::string color = "green";
    std::string message = "Passed!";
    if (!passed[call_id]) { color = "red"; message = "Failed."; }

    if (output.IsHTML()) {
      // Show the test code.
      out << "\nTest: <b><code>" << test.ToString() << "</code></b>\n"
          << "<p>Result: <span style=\"color: " << color << "\"><b>"
          << message << "</b></span><br>\n";

      if (error_out[call_id].size()) {
        out << "Error Message: " << error_out[call_id] << "<br>\n";
      }

      // If there was a comparison, show results on both sides of it.
      if (test.HasComp()) {
        out << "<table><tr><td>Left side:<td><code>" << test.GetLHS()
            << "</code><td>&nbsp;&nbsp;==><td><code>" << lhs_value[call_id] << "</code></tr>\n"
            << "<tr><td>Right side:<td><code>" << test.GetRHS() << "</code><td>&nbsp;&nbsp;==><td><code>"
            << rhs_value[call_id] << "</code></tr></table><br>\n";
      }
    } else {
      // Show the test code.
      out << "\nTest: " << test.ToString() << "\n\n";
      out << "Result: " << message << "\n";
      if (error_out[call_id].size()) {
        out << "Error Message: " << error_out[call_id] << "\n";
      }

      // If there was a comparison, show results on both sides of it.
      if (test.HasComp()) {
        size_t max_width = std::max(test.GetLHS().size(), test.GetRHS().size());
        out << "Left side : " << emp::pad_back(test.GetLHS(), ' ', max_width)
                              << "  ==>  " << lhs_value[call_id] << "\n"
            << "Right side: " << emp::pad_back(test.GetRHS(), ' ', max_width)
                              << "  ==>  " << rhs_value[call_id] << "\n";
      }

    }
  }
};

#endif
