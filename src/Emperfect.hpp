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

#include "Testcase.hpp"

class Emperfect {
private:
  emp::File input_file;
  emp::vector<Testcase> tests;
  emp::vector<std::string> header;

  static constexpr size_t npos = static_cast<size_t>(-1);

  // Helper functions

  // Find the line number of the next command (starting with ':')
  size_t FindNextCommand(size_t start_line) {
    while (start_line < input_file.size()) {
      const std::string & line = input_file[start_line];
      if (line.size() && line[0] == ':') return start_line;
      ++start_line;
    }
    return npos;
  }

public:
  Emperfect() { }

  // Load test configurations from a stream.
  void Load(std::istream & is, std::string stream_name="input") {
    input_file.Load(is);
    // NOTE: Do not change whitespace as it might matter for output code.

    // Loop through the file and process each line.
    for (size_t line_id = 0; line_id < input_file.size(); ++line_id) {
      std::string line = input_file[line_id];
      if (emp::is_whitespace(line)) continue;  // Skip empty lines.

      // We need a command before any non-commands.
      emp::notify::TestError(line[0] == ':', "Line ", line_id, " in ", stream_name, " unknown\n", line, "\n");

      const std::string command = emp::to_lower( emp::string_pop_word(line) );
      if (command == ":header") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.
        size_t next_id = FindNextCommand(line_id+1);
        header = input_file.GetLines(line_id+1, next_id);
        line_id = next_id-1; // back up to preserve next command.

      } else if (command == ":testcase") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.
        tests.push_back( SetupTestcase(setting_map) );

      } else if (command == ":output") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.

      } else if (command == ":gradefile") {
        auto setting_map = emp::slice_assign(line); // Load in all settings for this line.

      } else {

      }
    }
  }
};

#endif
