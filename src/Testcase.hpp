/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  Testcase.hpp
 *  @brief An individual testcase in Emperfect.
 * 
 *  @todo bold failed test cases in PrintCode
 */

#ifndef EMPERFECT_TESTCASE_HPP
#define EMPERFECT_TESTCASE_HPP

#include "emp/base/vector.hpp"
#include "emp/tools/String.hpp"
#include "dtl.hpp"
#include "CheckInfo.hpp"

enum class TestStatus {
  PASSED = 0,
  FAILED_COMPILE, // Compilation failed.
  FAILED_CHECK,   // Failed one of the CHECK statements.
  FAILED_TIME,    // Took too long and had a timeout.
  FAILED_RUN,     // Had an error output during run.
  FAILED_OUTPUT,  // Output didn't match expected.
  MISSED_ERROR    // Wrong error code was returned.
};

class Testcase {
  friend class Emperfect;
private:
  // -- Configured from args --
  emp::String name;          // Name of this test case.
  size_t id;                 // Unique ID number for this test case.
  double points = 0.0;       // Number of points this test case is worth.

  emp::String input_filename;  // Name of file to feed as standard input, if any.
  emp::String expect_filename; // Name of file to compare against standard output.
  emp::String code_filename;   // Name of file with code to test.
  emp::String args;            // Command-line arguments.
  int expect_exit_code = 0;    // The expected exist code.

  // Names for generated files.
  emp::String cpp_filename;     // To create with C++ code for this test
  emp::String compile_filename; // To collect compiler outputs (usually warnings and errors)
  emp::String exe_filename;     // To create for executable
  emp::String output_filename;  // To collect output generated by executable.
  emp::String error_filename;   // To collect errors generated by executable.
  emp::String result_filename;  // To log results for test checks.

  // Testcase run configs.
  bool call_main = true;     // Should main() function be called?
  bool hidden = false;       // Should this test case be seen by students?
  bool match_case = true;    // Does case need to match perfectly in the output?
  bool match_space = true;   // Does whitespace need to match perfectly in the output?
  size_t timeout = 5;        // How many seconds should this testcase be allowed run?

  // -- Configured elsewhere --
  string_block_t code;       // The actual code associated with this test case.
  emp::String processed_code; // Code after ${} are filled in and collapsed to one string.
  emp::String filename;      // What file is this test case originally in?
  size_t start_line = 0;     // At which line number is this test case start?
  size_t end_line = 0;       // At which line does this test case end?

  std::vector<CheckInfo> checks;

  // -- Results --
  int compile_exit_code = -1;  // Exit code from compilation (results compiler_filename)
  int run_exit_code = -1;      // Exit code from running the test.
  bool output_match = true;    // Did exe output match expected output?
  bool hit_timeout = false;    // Did this testcase need to be halted?
  double score = 0.0;          // Final score awarded for this testcase.

  // Helper functions

  // Determine how many tests match a particular lambda.
  size_t CountIf(auto test) const {
    return std::count_if(checks.begin(), checks.end(), test);
  }

  // Generate and print a diff between two files to an output stream.
  void PrintDiffHtml(std::ostream & output, const std::stringstream &out_ss, const std::stringstream &expect_ss) const {
    std::string out_str = out_ss.str();
    std::string expect_str = expect_ss.str();
    dtl::Diff<char, std::string> d(out_str, expect_str);
    d.compose(); // Compute the diff, which gives us a sequence of edits.
    auto ses = d.getSes().getSequence(); // Get the sequence of edits, in this case a vector<pair<char, eleminfo>>.
    output << "<table>\n"
        << "<tr><th>Diff</tr>\n"
        << "<tr><td valign=\"top\" style=\"background-color:LightGray\"><pre>\n";
    bool span_open = false;
    for (auto edit_it = ses.begin(); edit_it != ses.end(); ++edit_it) {
      const auto &next = edit_it < ses.end() - 1 ? *(edit_it + 1) : *edit_it; // Get the next edit, or the current one if we're at the end.
      if (!span_open) {
        output << "<span style=\"background-color:";
        span_open = true;
        switch(edit_it->second.type) {
          case dtl::SES_ADD:
            output << "LightGreen\">";
            break;
          case dtl::SES_DELETE:
            output << "LightCoral\">";
            break;
          default:
            output << "LightGray\">"; // should never happen
            break;
        }
      }
      if (edit_it->first == '\0' && edit_it->second.type == dtl::SES_DELETE) output << "[NULL]";
      else output << edit_it->first; // here's where we actually print the character.

      if (edit_it->second.type != next.second.type || *edit_it == next) { // if edit_it is the last edit of its type, or if we're at the end of the sequence.
        output << "</span>";
        span_open = false;
      }
    }
    output << "</pre></tr></table>\n";
  }

public:
  Testcase(size_t _id) : id(_id) { }

  size_t GetNumChecks() const { return checks.size(); }
  size_t CountPassed() const {
    return CountIf([](const auto & check){ return check.Passed(); });
  }
  size_t CountFailed() const {
    return CountIf([](const auto & check){ return !check.Passed(); });
  }

  TestStatus GetStatus() const {
    if (compile_exit_code) return TestStatus::FAILED_COMPILE;
    if (hit_timeout) return TestStatus::FAILED_TIME;
    if (run_exit_code != expect_exit_code) {
      if (expect_exit_code) return TestStatus::MISSED_ERROR;
      if (run_exit_code) return TestStatus::FAILED_RUN;
    }
    if (CountPassed() != checks.size()) return TestStatus::FAILED_CHECK;
    if (!output_match) return TestStatus::FAILED_OUTPUT;
    return TestStatus::PASSED;
  }

  emp::String GetStatusString() const {
    switch (GetStatus()) {
    case TestStatus::PASSED: return "Passing";
    case TestStatus::FAILED_CHECK: return "Checks Failing";
    case TestStatus::FAILED_COMPILE: return "Compilation Error";
    case TestStatus::FAILED_TIME: return "Timed Out";
    case TestStatus::FAILED_RUN: return "Error During Run";
    case TestStatus::FAILED_OUTPUT: return "Incorrect Output";
    case TestStatus::MISSED_ERROR:
      return emp::MakeString("Wrong exit code (expected ", expect_exit_code,
                             " received ", run_exit_code, ")");
    }
    return "Unknown";
  }

  bool Passed() const { return GetStatus() == TestStatus::PASSED; }
  bool Failed() const { return !Passed(); }

  // Test if a check at particular line number passed.
  bool Passed(size_t test_id) const {
    for (const auto & check : checks) {
      if (check.GetID() == test_id) return check.Passed();
    }
    return false; // No check here.
  }

  double EarnedPoints() const { return Passed() ? points : 0.0; }

  // Convert all CHECK macros.
  emp::String ProcessChecks() {
    // Take an input line and convert "CHECK" macro into full analysis and output code.
    emp::String out_code = processed_code.ReplaceMacro("CHECK(", ")",
      [this](const std::string & check_body, size_t line_num, size_t check_id){
        emp::String location =
          emp::MakeString("Testcase #", id, ", Line", line_num, " (check ", check_id, ")");
        checks.emplace_back(check_body, location, check_id, CheckType::ASSERT);
        return checks.back().ToCPP();
      });

    // Take an input line and convert "CHECK" macro into full analysis and output code.
    out_code = emp::replace_macro(out_code, "CHECK_TYPE",
      [this](const std::string & check_body, size_t line_num, size_t check_id){
        emp::String location =
          emp::MakeString("Testcase #", id, ", Line", line_num, " (check ", check_id, ")");
        checks.emplace_back(check_body, location, check_id, CheckType::TYPE_COMPARE);
        return checks.back().ToCPP();
      });

    return out_code;
  }

  
  // Generate a C++ file for internal testing with the provided header.
  void GenerateTestCPP(const emp::String & header) {
    // If we are using a file for test code, load it in.
    if (code_filename.size()) {
      emp::notify::TestError(code.size(),
        "Test case ", id, " cannot have both a code filename and in-place code provided.");

      emp::File file(code_filename);
      code = file.GetAllLines();
    }

    // Start with boilerplate.
    std::cout << "Creating: " << cpp_filename << std::endl;
    std::ofstream cpp_file(cpp_filename);
    cpp_file
      << "// This is a test file autogenerated by Emperfect.\n"
      << "// See: https://github.com/mercere99/Emperfect\n\n"
      << "#include <fstream>\n"
      << "#include <iostream>\n"
      << "#include <unordered_map>\n"
      << "#include <sstream>\n"
      << "#include <string>\n"
      << "#include <tuple>\n"
      << "#include <type_traits>\n"
      << "#include <vector>\n"
      << "\n"
      << "// Extract information about a function.\n"
      << "template <typename... Ts> struct FunInfo;\n"
      << "template <typename RETURN_T, typename... ARG_Ts>\n"
      << "struct FunInfo<RETURN_T(ARG_Ts...)> {\n"
      << "  using return_t = RETURN_T;\n"
      << "  template <size_t N> using arg_t = std::tuple_element_t<N, std::tuple<ARG_Ts...>>;\n"
      << "  static constexpr size_t ArgCount() { return sizeof...(ARG_Ts); }\n"
      << "};\n"
      << "// Build a map of internal type names to human-readable type names.\n"
      << "static std::string _EMP_ConvertTypeName(std::string in) {\n"
      << "  static std::unordered_map<std::string, std::string> type_map;\n"
      << "  if (type_map.size() == 0) {\n"
      << "    type_map[ typeid(bool).name() ]        = \"bool\";\n"
      << "    type_map[ typeid(char).name() ]        = \"char\";\n"
      << "    type_map[ typeid(double).name() ]      = \"double\";\n"
      << "    type_map[ typeid(float).name() ]       = \"float\";\n"
      << "    type_map[ typeid(int).name() ]         = \"int\";\n"
      << "    type_map[ typeid(int8_t).name() ]      = \"int8_t\";\n"
      << "    type_map[ typeid(int16_t).name() ]     = \"int16_t\";\n"
      << "    type_map[ typeid(int32_t).name() ]     = \"int32_t\";\n"
      << "    type_map[ typeid(int64_t).name() ]     = \"int64_t\";\n"
      << "    type_map[ typeid(uint8_t).name() ]     = \"uint8_t\";\n"
      << "    type_map[ typeid(uint16_t).name() ]    = \"uint16_t\";\n"
      << "    type_map[ typeid(uint32_t).name() ]    = \"uint32_t\";\n"
      << "    type_map[ typeid(uint64_t).name() ]    = \"uint64_t\";\n"
      << "    type_map[ typeid(int).name() ]         = \"int\";\n"
      << "    type_map[ typeid(size_t).name() ]      = \"size_t\";\n"
      << "    type_map[ typeid(std::string).name() ] = \"std::string\";\n"
      << "  }\n"
      << "  if (type_map.find(in) != type_map.end()) return type_map[in];\n"
      << "  return in; // No alternative name found.\n"
      << "}\n"
      << "\n"
      << "// Type Traits...\n"
      << "template<typename... Ts> struct is_vector : std::false_type { };\n"
      << "template<typename... Ts> struct is_vector<std::vector<Ts...>> : std::true_type { };\n"
      << "\n"
      << "// Convert a C++ type into its type name.\n"
      << "template <typename T>\n"
      << "std::string _EMP_GetTypeName() {\n"
      << "  std::string name = typeid(T).name();\n"
      << "  constexpr bool is_fun = std::is_function<T>();\n"
      << "  std::cout << \"TEST TYPE - \" << name << \" \" << is_fun << std::endl;\n"
      << "  if constexpr (is_fun) {\n"
      << "    using return_t = typename FunInfo<T>::return_t;\n"
      << "    constexpr size_t arg_count = FunInfo<T>::ArgCount();\n"
      << "    name = _EMP_GetTypeName<return_t>();\n"
      << "    name += '(';\n"
      << "    if constexpr (arg_count >= 1) { using arg_t = typename FunInfo<T>::arg_t<0>; name += _EMP_GetTypeName<arg_t>(); }\n"
      << "    if constexpr (arg_count >= 2) { using arg_t = typename FunInfo<T>::arg_t<1>; name += ','; name += _EMP_GetTypeName<arg_t>(); }\n"
      << "    if constexpr (arg_count >= 3) { using arg_t = typename FunInfo<T>::arg_t<2>; name += ','; name += _EMP_GetTypeName<arg_t>(); }\n"
      << "    if constexpr (arg_count >= 4) { using arg_t = typename FunInfo<T>::arg_t<3>; name += ','; name += _EMP_GetTypeName<arg_t>(); }\n"
      << "    if constexpr (arg_count >= 5) { using arg_t = typename FunInfo<T>::arg_t<4>; name += ','; name += _EMP_GetTypeName<arg_t>(); }\n"
      << "    if constexpr (arg_count >= 6) { using arg_t = typename FunInfo<T>::arg_t<5>; name += ','; name += _EMP_GetTypeName<arg_t>(); }\n"
      << "    name += ')';\n"
      << "  }\n"
      << "  else if constexpr (is_vector<T>()) {\n"
      << "    name = \"std::vector<\";\n"
      << "    name += _EMP_GetTypeName<typename T::value_type>();\n"
      << "    name += '>';"
      << "  }\n"
      << "  else {\n"
      << "    name = _EMP_ConvertTypeName(name);\n"
      << "  }\n"
      << "  if (std::is_const<T>()) name += \" const\";\n"
      << "  if (std::is_reference<T>()) name += \" &\";\n"
      << "  return name;\n"
      << "}\n"
      << "\n"
      << "// Convert a char to an escape char if needed.\n"
      << "std::string to_esc(char c) {\n"
      << "  switch (c) {\n"
      << "  case '\\0': return \"\\\\0\";\n"
      << "  case '\\n': return \"\\\\n\";\n"
      << "  case '\\r': return \"\\\\r\";\n"
      << "  case '\\t': return \"\\\\t\";\n"
      << "  case '\\\'': return \"\\\\\\\'\";\n"
      << "  case '\\\"': return \"\\\\\\\"\";\n"
      << "  case '\\\\': return \"\\\\\\\\\";\n"
      << "  }\n"
      << "  return std::string(1,c);\n"
      << "}\n"
      << "std::string to_esc(std::string str) {\n"
      << "  std::string out;\n"
      << "  for (char x : str) out += to_esc(x);\n"
      << "  return out;\n"
      << "}\n"
      << "\n"
      << "std::string to_literal(char c) {\n"
      << "  std::string out(\"\\\'\");\n"
      << "  out += to_esc(c);\n"
      << "  out += \"\\\'\";\n"
      << "  return out;\n"
      << "}\n"
      << "std::string to_literal(std::string str) {\n"
      << "  while (str.size() && str[0]==' ') str.erase(0,1); // Erase leading whitespace\n"
      << "  std::string out(\"\\\"\");\n"
      << "  out += to_esc(str);\n"
      << "  out += \"\\\"\";\n"
      << "  return out;\n"
      << "}\n"
      << "std::string to_literal(const char * str) { return to_literal(std::string(str)); }\n"
      << "template <typename T> const T & to_literal(const T & val) { return val; }\n"
      << "\n"
      << header << "\n"
      << "void _emperfect_main() {\n"
      << "  std::ofstream _emperfect_results(\"" << result_filename << "\");\n"
      << "  size_t _emperfect_error_count = 0;\n"
      << "  [[maybe_unused]] size_t _emperfect_check_id = 0;\n\n";

    // Add updated code for this specific test.
    cpp_file << ProcessChecks() << "\n";

    // Close out the main and make sure it get's run appropriately.
    cpp_file
      << "  _emperfect_results << \"SCORE \" << (!_emperfect_error_count ? "
          << points << " : 0) << \"\\n\";\n"
      << "}\n\n"
      << "// Build a test runner to be executed before main().\n"
      << "struct _emperfect_runner {\n"
      << "  _emperfect_runner() {\n"
      << "    _emperfect_main();\n";
    if (call_main == false) cpp_file << "    exit(0); // Don't execute main().\n";
    cpp_file
      << "  }\n"
      << "};\n\n"
      << "static _emperfect_runner runner;\n";
  }


  void PrintCode(OutputInfo & output) const {
    std::ostream & out = output.GetFile();

    // If there is no code, don't worry about printing it.
    if (code.size() == 0) return;
    // {
    //   out << "No test sourcecode.\n";
    //   if (output.IsHTML()) out << "<br><br>";
    //   return;
    // }

    if (output.IsHTML()) {
      out << "Sourcecode for Test:<br><br>\n";
      out << "<table style=\"background-color:#E3E0CF;\"><tr><td><pre>\n\n";
      for (auto line : code) {
        const bool highlight = false; // Passed(0);
        if (highlight) out << "<b>";
        out << line << "\n";
        if (highlight) out << "</b>";
      }
      out << "</pre></tr></table>\n";
    } else {
      out << "Sourcecode for Test:\n\n";
      for (auto line : code) out << line << "\n";
    }
  }

  void PrintCompileResults(OutputInfo & output) const {
    std::ostream & out = output.GetFile();
    emp::File file(compile_filename);

    if (output.IsHTML()) {
      out << "<p>Compile Results for Test:<br><br>\n";
      emp::String size_style = "width:800px;";
      if (file.size() > 25) size_style += " height:400px; overflow-y:scroll;";
      out << "<table style=\"background-color:Lavender\">"
          << "<tr><td style=\"" << size_style << " display:block;\"><pre>\n\n";
      for (auto line : file) {
        out << line << "\n";
      }
      out << "</pre></tr></table>\n";
    } else {
      out << "Compile Results for Test:\n\n";
      for (auto line : file) out << line << "\n";
    }
  }

  void PrintErrorResults(OutputInfo & output) const {
    std::ostream & out = output.GetFile();

    emp::File error_file(error_filename);

    if (output.IsHTML()) {
      out << "<table>\n"
          << "<tr><th>Run-time Error Messages:</tr>\n"
          << "<tr><td valign=\"top\" style=\"background-color:LightGray\"><pre>\n";
      for (auto line : error_file) out << line << "\n";
      out << "</pre></tr></table>\n";
    } else {
      out << "========== RUN-TIME ERRORS ==========\n";
      for (auto line : error_file) out << line << "\n";
    }
  }

  void PrintArgs(OutputInfo & output) const {
    if (args.size() == 0) return; // No arguments to print.

    std::ostream & out = output.GetFile();
    out << "Command Line Arguments: ";
    if (output.IsHTML()) {
      out << "<code>" << args << "</code><br>\n";
    } else {
      out << args << "\n";
    }
  }

  void PrintInputFile(OutputInfo & output) const {
    std::ostream & out = output.GetFile();

    if (input_filename.size() == 0) { // No inputs to print.
      out << "No input for test.";
      if (output.IsHTML()) out << "<br>";
      out << "\n";
      return;
    }

    emp::File input_file(input_filename);

    if (output.IsHTML()) {
      out << "<table>\n"
          << "<tr><th>Input</tr>\n"
          << "<tr><td valign=\"top\" style=\"background-color:LightGreen\"><pre>\n";
      for (auto line : input_file) {
        out << line << "\n";
      }
      out << "</pre></tr></table>\n";
    } else {
      out << "========== INPUT ==========\n";
      for (auto line : input_file) out << line << "\n";
    }
  }

  void PrintOutputDiff(OutputInfo & output) const {
    std::ostream & out = output.GetFile();
    emp::File output_file(output_filename);
    emp::File expect_file(expect_filename);
    std::stringstream out_ss, exp_ss;
    output_file.Write(out_ss);
    expect_file.Write(exp_ss);
    if (output.IsHTML()) {
      out << "<table>\n"
          << "<tr><th>Your Output<th> <th>Expected Output</tr>\n"
          << "<tr><td valign=\"top\" style=\"background-color:LightGoldenrodYellow\"><pre>\n";
      for (auto line : output_file) {
        out << emp::MakeEscaped(line) << "\n";
      }
      out << "</pre>\n"
          << "<td>&nbsp;<td valign=\"top\" style=\"background-color:LightBlue\"><pre>\n";
      for (auto line : expect_file) {
        out << emp::MakeEscaped(line) << "\n";
      }
      out << "</pre></tr></table>\n";
      PrintDiffHtml(out, out_ss, exp_ss); // Print a diff of the two files.
    } else {
      out << "========== YOUR OUTPUT ==========\n";
      for (auto line : output_file) out << emp::MakeEscaped(line) << "\n";
      out << "\n========== EXPECTED OUTPUT ==========\n";
      for (auto line : expect_file) out << emp::MakeEscaped(line) << "\n";
      out << "\n========== END OUTPUT ==========\n";
    }
  }

  void PrintResult_Title(OutputInfo & output) const {
    std::ostream & out = output.GetFile();

    if (output.IsHTML()) {
      out << "<h2 id=\"Test" << id << "\">Test Case " << id << ": " << name;
      if (hidden) out << " <small>[HIDDEN]</small>";
      out << "</h2>\n";
    } else {
      out << "TEST CASE " << id << ": " << name;
      if (hidden) out << " [HIDDEN]";
      out << "\n";
    }
  }

  void PrintResult_Success(OutputInfo & output) const {
    std::ostream & out = output.GetFile();

    // Notify whether the overall test passed.
    emp::String color, message;

    switch (GetStatus()) {
      case TestStatus::PASSED:
        color = "Green"; message = "PASSED!"; break;
      case TestStatus::FAILED_CHECK:
        color = "Red"; message = "FAILED due to unsuccessful check."; break;
      case TestStatus::FAILED_COMPILE:
        color = "DarkRed"; message = "FAILED during compilation."; break;
      case TestStatus::FAILED_TIME:
        color = "Purple"; message = "FAILED due to timeout."; break;
      case TestStatus::FAILED_RUN:
        color = "OrangeRed"; message = "FAILED due to run-time error."; break;
      case TestStatus::FAILED_OUTPUT:
        color = "OrangeRed"; message = "FAILED due to mis-matched output."; break;
      case TestStatus::MISSED_ERROR:
        color = "OrangeRed";
        message.Set("FAILED due to wrong error code (expected ", expect_exit_code,
                    "; received ", run_exit_code, ")."); break;
    }

    if (output.IsHTML()) {
      out << "<b>Result: <span style=\"color: " << color << "\">"
          << message << "</b></span><br><br>\n\n";
    } else {
      out << "Result: " << message << "\n";
    }
  }

  void PrintResult_Checks(OutputInfo & output) const {
    // Print the results for each check.
    if (!hidden || output.HasHiddenDetails()) {
      for (const auto & check : checks) {
        check.PrintResults(output);
      }
    }
  }

  void PrintResult(OutputInfo & output) const {
    if (!output.HasResults()) return;

    PrintResult_Title(output);
    PrintResult_Success(output);

    // Print extra information only if we are allowed to.
    if ((hidden && !output.HasHiddenDetails())) return;

    // Decide what else we print based on the status.
    const auto status = GetStatus();
    bool print_checks = status == TestStatus::FAILED_CHECK || output.HasPassedDetails();
    bool print_code = Failed() || output.HasPassedDetails();
    bool print_compile = status == TestStatus::FAILED_COMPILE;
    bool print_error = status == TestStatus::FAILED_RUN;
    bool print_input = status == TestStatus::MISSED_ERROR || status == TestStatus::FAILED_OUTPUT || output.HasPassedDetails();
    bool print_diff = status == TestStatus::FAILED_RUN || status == TestStatus::FAILED_OUTPUT;

    if (print_checks) PrintResult_Checks(output);
    if (print_code) PrintCode(output);
    if (print_compile) PrintCompileResults(output);
    if (print_error) PrintErrorResults(output);
    if (print_input) { PrintArgs(output); PrintInputFile(output); }
    if (print_diff) PrintOutputDiff(output);
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
        << "FILENAME Input to provide...: " << (input_filename.size() ? input_filename : "(none)") << "\n"
        << "FILENAME Expected exit code.: " << expect_exit_code << "\n"
        << "FILENAME Expected output....: " << (expect_filename.size() ? expect_filename : "(none)") << "\n"
        << "FILENAME Code for testcase..: " << (code_filename.size() ? code_filename : "(none)") << "\n"
        << "FILENAME Generated CPP file.: " << (cpp_filename.size() ? cpp_filename : "(none)") << "\n"
        << "FILENAME Compiler results...: " << (compile_filename.size() ? compile_filename : "(none)") << "\n"
        << "FILENAME Executable.........: " << (exe_filename.size() ? exe_filename : "(none)") << "\n"
        << "FILENAME Execution output...: " << (output_filename.size() ? output_filename : "(none)") << "\n"
        << "FILENAME Execution errors...: " << (error_filename.size() ? error_filename : "(none)") << "\n"
        << "FILENAME Log of test results: " << (result_filename.size() ? result_filename : "(none)") << "\n"
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
