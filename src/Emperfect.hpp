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

#include "emp/base/vector.hpp"
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

  static constexpr size_t npos = static_cast<size_t>(-1);

  // --- Helper functions ---

  // Determine if a line has a command on it.
  bool IsCommand(const std::string & str) const {
    if (str.size() == 0) return false;
    return str[0] == ':';
  }

  // Load in a value for a setting that translates to a bool.
  bool ParseBool(const std::string & input,
                 const std::string & setting_name)
  {
    if (input == "true" || input == "1") return true;
    if (input == "false" || input == "0") return false;
    emp::notify::Error("Unknown testcase '", setting_name, "' value '", input, "'.");
    return false; // Doesn't actually return since previous line triggers error.
  }

  // Load a block of code from the file, using file_scan
  emp::vector<std::string> LoadCode() {
    return file_scan.ReadUntil( [this](std::string in){ return IsCommand(in); } );
  }

  // Setup a new compilation method.
  void SetCompile(const std::string & args) {
    auto setting_map = emp::slice_assign(args); // Load in all arguments for this command.
    compile = LoadCode();
  }

  // Setup new header code.
  void SetHeader(const std::string & args) {
    auto setting_map = emp::slice_assign(args); // Load in all arguments for this command.
    header = LoadCode();
  }

  // Add a new method of collecting output.
  void AddOutput(const std::string & args) {
    auto setting_map = emp::slice_assign(args); // Load in all arguments for this command.
    outputs.push_back( SetupOutput(setting_map) );
  }

  // Add a new testcase.
  void AddTestcase(const std::string & args) {
    emp::notify::TestError(compile.size() == 0,
      "Trying to setup testcase, but no compile rules are in place.");
    auto setting_map = emp::slice_assign(args); // Load in all arguments for this command.
    tests.push_back();
    auto & test = tests.back();
    for (auto [arg, value] : setting_map) {
      if (value.size() && value[0] == '\"') value = emp::from_literal_string(value);

      if (arg == "args") test.args = value;
      else if (arg == "code_file") test.code_filename = value;
      else if (arg == "hidden") test.hidden = ParseBool(value, "hidden");
      else if (arg == "in_file") test.in_filename = value;
      else if (arg == "match_case") test.match_case = ParseBool(value, "match_case");
      else if (arg == "match_space") test.match_space = ParseBool(value, "match_space");
      else if (arg == "name") test.name = value;
      else if (arg == "out_file") test.out_filename = value;
      else if (arg == "points") test.points = emp::from_string<double>(value);
      else if (arg == "run_main") test.call_main = ParseBool(value, "run_main");
    }
  }

public:
  Emperfect() : file_scan(input_file) { }

  // Load test configurations from a stream.
  void Load(std::istream & is, std::string stream_name="input") {
    input_file.Load(is);
    // NOTE: Do not change whitespace as it might matter for output code.

    // Loop through the file and process each line.
    while (file_scan) {
      std::string line = file_scan.Read();
      if (emp::is_whitespace(line)) continue;  // Skip empty lines.

      // We are expecting a command, but don't get one, report an error.
      emp::notify::TestError(line[0] == ':',
        "Line ", file_scan.GetLine()-1, " in ", stream_name, " unknown\n", line, "\n");

      const std::string command = emp::to_lower( emp::string_pop_word(line) );
      if (command == ":compile") SetCompile(line);
      else if (command == ":header") SetHeader(line);
      else if (command == ":output") AddOutput(line);
      else if (command == ":testcase") AddTestcase(line);
      else {
        emp::notify::Error("Unknown Emperfect command '", command, "'.");
      }
    }
  }
};

#endif
