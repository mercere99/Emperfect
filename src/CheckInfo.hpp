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

#include "emp/base/notify.hpp"
#include "emp/bits/BitVector.hpp"
#include "emp/datastructs/vector_utils.hpp"
#include "emp/tools/String.hpp"

using string_block_t = emp::vector<emp::String>;

// Parsed information about a given check.
class CheckString {
private:
  emp::String test = "";
  emp::String lhs = "";
  emp::String comparator = "";
  emp::String rhs = "";

public:
  void SetCheck(const emp::String & _test, emp::String location) {
    test = _test;
    emp::notify::TestError(emp::find_any_of(test, 0, "&&", "||") != emp::String::npos,
      location, ": Unit test checks do not allow \"&&\" or \"||\".");

    size_t comp_pos = emp::find_any_of(test, 0, "==", "!=", "<", "<=", ">", ">=");
    bool has_comp = (comp_pos != emp::String::npos);

    if (has_comp) {
      // Make sure it doesn't have TWO comparisons.
      emp::notify::TestError(
        emp::find_any_of(test, comp_pos+2, "==", "!=", "<", "<=", ">", ">=") != emp::String::npos,
        location, ": Unit test checks can have only one comparison.");

      size_t comp_size = (test[comp_pos+1] == '=') ? 2 : 1;
      comparator = test.substr(comp_pos, comp_size);
      lhs = test.substr(0, comp_pos);
      rhs = test.substr(comp_pos+comp_size);
      lhs.Trim();
      rhs.Trim();
    }
    else {
      lhs = test; // Whole test is on the left-hand-side.
    }
  }

  void SetCheckType(const emp::String & expression, const emp::String & type, emp::String location) {
    (void) location;
    test.Set("TYPE(", expression, ") == ", type);
    lhs = expression;
    rhs = type;
    comparator = "TYPE";
  }

  const emp::String & ToString() const { return test; }
  emp::String ToLiteral() const { return test.AsLiteral(); }
  emp::String GetLHS() const { return lhs; }
  emp::String GetRHS() const { return rhs; }
  emp::String GetComparator() const { return comparator; }
  bool HasComp() const { return comparator.size(); }
};

enum class CheckType {
  UNKNOWN = 0,
  ASSERT,
  TYPE_COMPARE
};

class CheckInfo {
private:

  CheckString test;            // The test string associated with this check.
  emp::String location;        // Position in the file where this check is located.
  size_t id;                   // Unique ID for this check.
  CheckType type;              // What type of check are we doing?
  string_block_t error_msgs;   // Extra arguments from check to use in error messages.

  emp::vector<emp::String> lhs_value;  // Resulting value on left (e.g., "20")
  emp::vector<emp::String> rhs_value;  // Resulting value on right (e.g., "21", if rhs is "x+5" and x=16)
  emp::BitVector passed = false;       // Was this check successful?
  emp::vector<emp::String> error_out;  // Message from test runner for students.

public:
  CheckInfo(const emp::String & check_body, emp::String _location, size_t _id, CheckType _type)
    : location(_location), id(_id), type(_type)
  {
    emp_assert(type != CheckType::UNKNOWN);

    error_msgs = check_body.Slice(",", emp::StringSyntax{"\"","(){}"}, true);

    if (type == CheckType::ASSERT) {
      // Split off the test (the first argument) and make sure it's valid.
      emp::notify::TestError(error_msgs.size() == 0, location, ": CHECK cannot be empty.");
      test.SetCheck(emp::PopFront(error_msgs), location);
    }
    else if (type == CheckType::TYPE_COMPARE) {
      // The first argument is the expression, the second is the type to use.
      emp::notify::TestError(error_msgs.size() < 2, location, ": CHECK_TYPE needs at least two args.");
      emp::String lhs = emp::PopFront(error_msgs);
      emp::String rhs = emp::PopFront(error_msgs);
      test.SetCheckType(lhs, rhs, location);
    }

  }
  CheckInfo(const CheckInfo &) = default;

  size_t GetID() const { return id; }
  bool Passed() const { return passed.size() && passed.All(); }
  bool PassedAny() const { return passed.Any(); }

  void PushResult(bool success) { passed.push_back(success); }
  void PushLHSValue(emp::String _in) { _in.Trim(); lhs_value.push_back(_in); }
  void PushRHSValue(emp::String _in) { _in.Trim(); rhs_value.push_back(_in); }
  void PushErrorMsg(emp::String _in) { _in.Trim(); error_out.push_back(_in); }

  void ToCPP_CHECK(std::ostream & out) const {
    // Generate code for this test.
    out << "  // CHECK #" << id << "\n"
        << "  {\n"
        << "    auto _emperfect_lhs = " << test.GetLHS() << ";\n";
    if (test.HasComp()) {
      emp::String rhs_string = test.GetRHS();
      if (rhs_string.IsNumber()) {
        out << "    auto _emperfect_rhs = " << rhs_string << ";\n"
            << "    bool _emperfect_success = (_emperfect_lhs " << test.GetComparator() << " " << rhs_string << ");\n";
      } else {
        out << "    auto _emperfect_rhs = " << rhs_string << ";\n"
            << "    bool _emperfect_success = (_emperfect_lhs " << test.GetComparator() << " _emperfect_rhs);\n";
      }
    } else {
      out << "    auto _emperfect_rhs = \"N/A\";\n"
          << "    bool _emperfect_success = _emperfect_lhs;\n";
    }
  }

  void ToCPP_CHECK_TYPE(std::ostream & out) const {
    // Generate code for this test.
    out << "  // CHECK #" << id << " (CHECK_TYPE)\n"
        << "  {\n"
        << "    using _emperfect_type1 = decltype(" << test.GetLHS() << ");\n"
        << "    using _emperfect_type2 = " << test.GetRHS() << ";\n"
        << "    std::string _emperfect_lhs = _EMP_GetTypeName<_emperfect_type1>();\n"
        << "    std::string _emperfect_rhs = " << test.GetRHS().AsLiteral() << ";\n"
        << "    bool _emperfect_success = std::is_same<_emperfect_type1, _emperfect_type2>();\n";
  }

  emp::String ToCPP() const {
    std::stringstream out;

    // Generate the test
    if (type == CheckType::ASSERT) ToCPP_CHECK(out);
    else if (type == CheckType::TYPE_COMPARE) ToCPP_CHECK_TYPE(out);

    // Save the results.
    out << "    _emperfect_passed &= _emperfect_success;\n"
        << "    std::string _emperfect_msg = \"Success!\";\n"
        << "    if (!_emperfect_success) {\n"
        << "      std::stringstream ss;\n";
    for (emp::String x : error_msgs) {
      out << "      ss << " << x << ";\n";
    }
    out << "      _emperfect_msg = ss.str();"
        << "    }\n"
        << "    _emperfect_results << \":CHECK: " << id << "\\n\"\n"
        << "                       << \":TEST: \" << " << test.ToLiteral() << " << \"\\n\"\n"
        << "                       << \":RESULT: \" << _emperfect_success << \"\\n\"\n"
        << "                       << \":LHS: \" << to_literal(_emperfect_lhs) << \"\\n\"\n"
        << "                       << \":RHS: \" << to_literal(_emperfect_rhs) << \"\\n\"\n"
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

    emp::String color = "green";
    emp::String message = "Passed!";
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
        out << "Left side : " << test.GetLHS().PadBack(' ', max_width)
                              << "  ==>  " << lhs_value[call_id] << "\n"
            << "Right side: " << test.GetRHS().PadBack(' ', max_width)
                              << "  ==>  " << rhs_value[call_id] << "\n";
      }

    }
  }
};

#endif
