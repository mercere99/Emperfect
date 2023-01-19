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
  emp::File input_file;  // File with all input data.
  size_t file_line = 0;  // Current line being accessed in the file.

  emp::vector<Testcase> tests;
  emp::vector<OutputInfo> outputs;
  emp::vector<std::string> compile;
  emp::vector<std::string> header;

  static constexpr size_t npos = static_cast<size_t>(-1);

  // --- Helper functions ---

  // Find the line number of the next command (starting with ':')
  size_t FindNextCommand(size_t start_line) {
    while (start_line < input_file.size()) {
      const std::string & line = input_file[start_line];
      if (line.size() && line[0] == ':') return start_line;
      ++start_line;
    }
    return npos;
  }

  // Determine if a line has a command on it.
  bool IsCommand(const std::string & str) const {
    if (str.size() == 0) return false;
    return str[0] == ':';
  }

  // Load a block of code from the file, starting from this point.  Advances file_line.
  emp::vector<std::string> LoadCode() {
    // return input_file.GetLinesUntil( file_line, [this](std::string in){ return IsCommand(in); } );
    emp::vector<std::string> code;
    while (file_line < input_file.size() && !IsCommand(input_file[file_line]))) {
      code.push_back( input_file[file_line++] );
    }
    return code;
  }

public:
  Emperfect() { }

  // Load test configurations from a stream.
  void Load(std::istream & is, std::string stream_name="input") {
    input_file.Load(is);
    // NOTE: Do not change whitespace as it might matter for output code.

    // Loop through the file and process each line.
    while (file_line < input_file.size()) {
      std::string line = input_file[file_line++];
      if (emp::is_whitespace(line)) continue;  // Skip empty lines.

      // We need a command before any non-commands.
      emp::notify::TestError(line[0] == ':', "Line ", line_id, " in ", stream_name, " unknown\n", line, "\n");

      const std::string command = emp::to_lower( emp::string_pop_word(line) );
      if (command == ":compile") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.
        compile = LoadCode();
      }
      else if (command == ":header") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.
        header = LoadCode();
      }
      else if (command == ":output") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.
        outputs.push_back( SetupOutput(setting_map) );

      }
      else if (command == ":testcase") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.
        tests.push_back( SetupTestcase(setting_map) );

      } else {
        emp::notify::Error("Unknown Emperfect command '", command, "'.");
      }
    }
  }
};

#endif
