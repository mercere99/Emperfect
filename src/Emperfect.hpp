/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  Emperfect.hpp
 *  @brief Main driver for Emperfect unit testing
 * 
 */

#ifndef EMPERFECT_EMPERFECT_HPP
#define EMPERFECT_EMPERFECT_HPP

#include <iostream>

#include "emp/base/notify.hpp"
#include "emp/base/vector.hpp"
#include "emp/datastructs/map_utils.hpp"
#include "emp/io/File.hpp"

#include "OutputInfo.hpp"
#include "Testcase.hpp"

class Emperfect {
private:
  emp::File input_file;       // File with all input data.
  emp::File::Scan file_scan;  // Position in file.

  emp::vector<Testcase> tests;
  emp::vector<OutputInfo> outputs;
  emp::vector<std::string> compile;
  emp::vector<std::string> header;

  std::map<std::string, std::string> var_map; // Map of all usable variables.

  static constexpr size_t npos = static_cast<size_t>(-1);

  // --- Helper functions ---

  // Load in a value for a setting that translates to a bool.
  bool ParseBool(const std::string & input,
                 const std::string & setting_name)
  {
    if (input == "true" || input == "1") return true;
    if (input == "false" || input == "0") return false;
    emp::notify::Error("Unknown testcase '", setting_name, "' value '", input, "'.");
    return false; // Doesn't actually return since previous line triggers error.
  }

  // Load new variables into var_map and return just the new variables.
  auto LoadVars(const std::string & args) {
    // Return an empty map if all we have is whitespace.
    if (emp::is_whitespace(args)) return std::map<std::string, std::string>();

    // Otherwise assume comma-separated assignments.
    auto setting_map = emp::slice_assign(args);
    for (auto [var, value] : setting_map) {
      var_map[var] = value;
    }

    // Return just the new vars.
    return setting_map;
  }

  // Take an input line and fill out any variables, as needed.
  std::string ApplyVars(const std::string & line) {
    size_t next_pos = 0, var_start = 0;
    std::string out_string;
    while ((var_start = line.find("${", next_pos)) != std::string::npos) {
      // Copy the string to the start of the variable.
      out_string += line.substr(next_pos, var_start-next_pos);

      // Replace the variable we found.
      size_t var_end = line.find("}", var_start);
      emp::notify::TestError(var_end == std::string::npos, "No end to variable on line: ", line);
      std::string var_name = line.substr(var_start+2, var_end-var_start-2);
      emp::notify::TestError(!emp::Has(var_map, var_name), "Unknown variable used: ", var_name);
      out_string += var_map[var_name];

      next_pos = var_end+1;
    }
    // Copy the remainder of the string.
    out_string += line.substr(next_pos, var_start-next_pos);
    return out_string;
  }

  // Take an input line and convert any "CHECK" macro into full analysis and output code.
  std::string ProcessChecks(const std::string & line, size_t test_id) {
    std::stringstream out;

    // We need to identify the comparator and divide up arguments in CHECK.
    size_t check_pos = line.find("CHECK(");
    size_t check_end = 0;
    size_t check_id = 0;
    while (check_pos != std::string::npos) {
      std::string location = emp::to_string("Test #", test_id, ", Check #", check_id);

      // Output everything from the end of the last check to the beginning of this one.
      out << line.substr(check_end, check_pos-check_end);

      // Isolate this check and divide it into arguments.
      check_end = emp::find_paren_match(line, check_pos+5);
      std::string check_body = line.substr(check_pos+6, check_end-check_pos-6);
      check_end += 2;  // Advance the end past the ");" at the end of the check.
      auto check_args = emp::slice(check_body, ',', 256, true, true, true);

      // Split off the test (the first argument) and make sure it's valid.
      emp::notify::TestError(check_args.size() == 0, location, ": CHECK cannot be empty.");
      std::string check_test = check_args[0];
      check_args.erase(check_args.begin());

      emp::notify::TestError(emp::find_any_of(check_test, 0, "&&", "||") != std::string::npos,
        location, ": Unit test checks do not allow \"&&\" or \"||\".");

      size_t comp_pos = emp::find_any_of(check_test, 0, "==", "!=", "<", "<=", ">", ">=");

      // Generate code for this test.
      out << "  /* CHECK #" << check_id << " */\n"
          << "  {\n";

      // No comparisons in the test...
      if (comp_pos == std::string::npos) {
        out << "    auto lhs = " << check_test << ";\n"
            << "    auto rhs = \"\";      /* No RHS in this test. */\n"
            << "    bool success = " << check_test << ";\n";
      }

      // Comparison in the test.
      else {
        emp::notify::TestError(
          emp::find_any_of(check_test, comp_pos+2, "==", "!=", "<", "<=", ">", ">=") != std::string::npos,
          location, ": Unit test checks can have only one comparison.");

        size_t comp_size = (check_test[comp_pos+1] == '=') ? 2 : 1;
        std::string comp = check_test.substr(comp_pos, comp_size);

        out << "    auto lhs = " << check_test.substr(0, comp_pos) << ";\n"
            << "    auto rhs = " << check_test.substr(comp_pos+comp_size)<< ";\n"
            << "    bool success = (lhs " << comp << " rhs);\n";
      }

      out << "    _emperfect_passed &= success;\n"
          << "    std::string _emperfect_msg;\n"
          << "    if (!success) {\n"
          << "      std::stringstream ss;\n";
      for (std::string x : check_args) {
        out << "      ss << " << x << ";\n";
      }
      out << "      _emperfect_msg = ss.str();"
          << "    }\n"
          << "    _emperfect_out << \":CHECK: " << check_id << "\\n\"\n"
          << "                   << \":TEST: \" << " << emp::to_literal(check_test) << "\\n\"\n"
          << "                   << \":RESULT: \" << (success ? \" PASSED\\n\" : \" FAILED\\n\")\n"
          << "                   << \":LHS: \" << lhs << \"\\n\"\n"
          << "                   << \":RHS: \" << rhs << \"\\n\"\n"
          << "                   << \":MSG: \" << _emperfect_msg << \"\\n\\n\";\n"
          << "    _emperfect_check_id++;\n"
          << "  }\n";

      // Find the next check and loop starting from the end of this one.
      check_id++;
      check_pos = line.find("CHECK(", check_end);
    }

    return out.str();
  }

  // Load a block of code from the file, using file_scan
  void LoadCode(emp::vector<std::string> & code, const std::string & args="") {
    if (args != "") LoadVars(args);
    code = file_scan.ReadUntil( [](std::string in){ return emp::has_char_at(in, ':', 0); } );
  }

  // Add a new method of collecting output.
  void AddOutput(const std::string & args) {
    auto setting_map = LoadVars(args);
    outputs.push_back( OutputInfo() );
    auto & output = outputs.back();

    for (auto [arg, value] : setting_map) {
      if (value.size() && value[0] == '\"') value = emp::from_literal_string(value);

      if (arg == "detail") output.SetDetail(value);
      else if (arg == "filename") output.filename = value;
      else if (arg == "type") output.type = value;
      else {
        emp::notify::Error("Uknown :Output argument '", arg, "'.");
      }
    }

    output.FinalizeType();  // If type has not been set, use filename to set it.
  }

  // Add a new, basic testcase without argument information.
  Testcase & AddTestcase() {
    size_t id = tests.size();
    tests.push_back(Testcase());
    auto & test = tests.back();
    test.id = id;
    return test;
  }

  // Use a set of arguments to configure a testcase.
  void ConfigTestcase(Testcase & test, const std::string & args) {
    // Initialize any special values that may not be set.
    test.cpp_filename = emp::to_string("_emp_out_", test.id, ".cpp");
    test.exe_filename = emp::to_string("_emp_out_", test.id, ".exe");
    test.output_filename = emp::to_string("_emp_out_", test.id, ".txt");

    auto setting_map = LoadVars(args);
    for (auto [arg, value] : setting_map) {
      if (value.size() && value[0] == '\"') value = emp::from_literal_string(value);

      if (arg == "args") test.args = value;
      else if (arg == "code_file") test.code_filename = value;
      else if (arg == "expected") test.expect_filename = value;
      else if (arg == "hidden") test.hidden = ParseBool(value, "hidden");
      else if (arg == "input") test.input_filename = value;
      else if (arg == "match_case") test.match_case = ParseBool(value, "match_case");
      else if (arg == "match_space") test.match_space = ParseBool(value, "match_space");
      else if (arg == "name") test.name = value;
      else if (arg == "output") test.output_filename = value;
      else if (arg == "points") test.points = emp::from_string<double>(value);
      else if (arg == "run_main") test.call_main = ParseBool(value, "run_main");
      else {
        emp::notify::Error("Unknown :Testcase argument '", arg, "'.");
      }
    }
  }

  void GenerateTestCPP(Testcase & test) {
    // If we are using a filename, load it in as code.
    if (test.code_filename.size()) {
      emp::notify::TestError(test.code.size(),
        "Test case ", test.id, " cannot have both a code filename and in-place code provided.");

      emp::File file(test.code_filename);
      test.code = file.GetAllLines();
    }

    // Start with boilerplate.
    std::cout << "Creating: " << test.cpp_filename << std::endl;
    std::ofstream cpp_file(test.cpp_filename);
    cpp_file
      << "// This is a test file autogenerated by Emperfect.\n"
      << "// See: https://github.com/mercere99/Emperfect\n\n"
      << "#include <fstream>\n"
      << "#include <iostream>\n"
      << "#include <sstream>\n"
      << "\n";


    // Add user-provided headers.
    for (const auto & x : header) cpp_file << ApplyVars(x) << "\n";

    // Start up main function.
    cpp_file
      << "void _emperfect_main() {\n"
      << "  std::ofstream _emperfect_out(\"_emperfect_log.txt\");\n"
      << "  _emperfect_out << \"TESTCASE " << test.id << "\\n\";\n"
      << "  bool _emperfect_passed = true;\n"
      << "  size_t _emperfect_check_id = 0;\n\n";

    // Add updated code for this specific test.
    cpp_file << ProcessChecks(ApplyVars( emp::join(test.code, "\n") ), test.id)
             << "\n";

    // Close out the main and make sure it get's run appropriately.
    cpp_file
      << "  _emperfect_out << \"SCORE \" << (_emperfect_passed ? "
          << test.points << " : 0) << \"\\n\";\n"
      << "}\n\n"
      << "/* Build a test runner to be executed before main(). */\n"
      << "struct _emperfect_runner {\n"
      << "  _emperfect_runner() {\n"
      << "    _emperfect_main();\n";
    if (test.call_main == false) cpp_file << "    exit(0); // Don't execute main().\n";
    cpp_file
      << "  }\n"
      << "};\n\n"
      << "static _emperfect_runner runner;\n";
  }

  /// @brief  Run a specific test case.
  /// @param test_id ID of the test case to run.
  void RunTest(Testcase & test) {
    // Running a test case has a series of phases.
    // 1. Generate the CPP file to be tested (including provided header and instrumentation)
    // 2. Compile the generated CPP file, reporting back any errors.
    // 3. Run the executable from the generated file, reporting back any errors.
    // 4. Compare any outputs produced, reporting back any differences in those outputs.
    // 5. Record any necessary point calculations and feedback that will be needed later.

    // @CAO: Should we allow a special symbol in the output to provide debug information without
    // affecting correctness?

    GenerateTestCPP(test);
  }

  // Add a new Testcase and run it.
  void AddTestcase(const std::string & args) {
    emp::notify::TestError(compile.size() == 0, "Cannot set up testcase without compile rules.");

    auto & test = AddTestcase();
    ConfigTestcase(test, args);
    LoadCode(test.code);
    RunTest(test);
  }

public:
  Emperfect() : file_scan(input_file) {
    // Initialize default values
    var_map["cpp"] = "emperfect_test_file_.cpp";
    var_map["exe"] = "emperfect_test_exe_";
    var_map["debug"] = "false";
  }

  // Load test configurations from a stream.
  void Load(std::istream & is, std::string stream_name="input") {
    input_file.Load(is);
    input_file.RemoveComments("//");
    // NOTE: Do not change whitespace as it might matter for output code.

    // Loop through the file and process each line.
    while (file_scan) {
      std::string line = ApplyVars( file_scan.Read() );
      if (emp::is_whitespace(line)) continue;  // Skip empty lines.

      // We are expecting a command, but don't get one, report an error.
      emp::notify::TestError(line[0] != ':',
        "Line ", file_scan.GetLine()-1, " in ", stream_name, " unknown\n", line, "\n");

      const std::string command = emp::to_lower( emp::string_pop_word(line) );
      if (command == ":compile") LoadCode(compile, line);
      else if (command == ":header") LoadCode(header, line);
      else if (command == ":output") AddOutput(line);
      else if (command == ":testcase") AddTestcase(line);
      else {
        emp::notify::Error("Unknown Emperfect command '", command, "'.");
      }
    }
  }

  void Load(std::string filename) {
    std::ifstream file(filename);
    Load (file, filename);
  }


  void PrintDebug(std::ostream & out=std::cout) {
    out << "Vars: " << var_map.size() << "\n"
        << "Outputs: " << outputs.size() << "\n"
        << "Compile Lines: " << compile.size() << "\n"
        << "Header Lines: " << header.size() << "\n"
        << "Tests: " << tests.size() << "\n";

    out << "\n-- Vars --\n";
    for (auto [name, val] : var_map) {
      out << "  ${" << name << "} = " << val << "\n";
    }

    out << "\n-- Outputs --\n";
    for (auto x : outputs) {
      x.PrintDebug(out);
    }

    out << "\n-- Compile Lines --\n";
    for (auto x : compile) {
      std::cout << x << std::endl;
    }

    out << "\n-- Header Lines --\n";
    for (auto x : header) {
      std::cout << x << std::endl;
    }

    out << "\n-- Tests --\n";
    for (auto x : tests) {
      x.PrintDebug(out);
    }
  }
};

#endif
