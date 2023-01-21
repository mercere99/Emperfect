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

  // Load new variables into var_map.
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

  // Load a block of code from the file, using file_scan
  emp::vector<std::string> LoadCode() {
    return file_scan.ReadUntil( [this](std::string in){ return IsCommand(in); } );
  }

  // Setup a new compilation method.
  void SetCompile(const std::string & args) {
    auto setting_map = LoadVars(args);
    compile = LoadCode();
  }

  // Setup new header code.
  void SetHeader(const std::string & args) {
    auto setting_map = LoadVars(args);
    header = LoadCode();
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

    output.FinalizeType();  // If type has not been set, set it.
  }

  // Add a new testcase.
  void AddTestcase(const std::string & args) {
    emp::notify::TestError(compile.size() == 0,
      "Trying to setup testcase, but no compile rules are in place.");
    auto setting_map = LoadVars(args);
    tests.push_back(Testcase());
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
      else {
        emp::notify::Error("Uknown :Testcase argument '", arg, "'.");
      }
    }
    test.code = LoadCode();
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
      if (command == ":compile") SetCompile(line);
      else if (command == ":header") SetHeader(line);
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
};

#endif
