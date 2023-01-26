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

#include <cstdlib>
#include <filesystem>
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
  bool is_init = false;       // Has the run been initialized yet?

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
      std::string var_name = emp::to_lower( line.substr(var_start+2, var_end-var_start-2) );
      emp::notify::TestError(!emp::Has(var_map, var_name), "Unknown variable used: ", var_name);
      out_string += var_map[var_name];

      next_pos = var_end+1;
    }
    // Copy the remainder of the string.
    out_string += line.substr(next_pos, var_start-next_pos);
    return out_string;
  }

  void Init(const std::string & args="") {
    emp::notify::TestError(is_init, "Error: :Init run twice!");
    is_init = true;
    LoadVars(args);

    // Make sure ${DIR} exists.
    std::string dir_name = var_map["dir"];
    if (!std::filesystem::exists(dir_name)) {
      std::cout << "CREATING: " << dir_name << std::endl;
      std::filesystem::create_directories(dir_name);
    }
  }

  // Load a block of code from the file, using file_scan
  void LoadCode(emp::vector<std::string> & code, const std::string & args="", bool remove_blank=true) {
    if (!is_init) Init();

    if (args != "") LoadVars(args);
    code = file_scan.ReadUntil( [](std::string in){ return emp::has_char_at(in, ':', 0); } );
    if (remove_blank) {
      for (size_t i=0; i < code.size();) {
        if (emp::is_whitespace(code[i])) {
          code.erase(code.begin()+i);
        }
        else ++i;
      }
    }
  }

  // Add a new method of collecting output.
  void AddOutput(const std::string & args) {
    if (!is_init) Init();

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

  // Use a set of arguments to configure a testcase.
  void ConfigTestcase(Testcase & test, const std::string & args) {
    // Initialize any special values that may not be set.
    std::string file_base = emp::to_string(var_map["dir"], "/Test", test.id);
    var_map["cpp"] =     file_base + ".cpp";
    var_map["exe"] =     file_base + ".exe";
    var_map["compile"] = file_base + "-compile.txt";
    var_map["out"] =     file_base + "-output.txt";
    var_map["error"] =   file_base + "-errors.txt";

    // Allow these to be overwritten by settings, and then lock them into the testcase.
    auto setting_map = LoadVars(args);
    test.cpp_filename =     var_map["cpp"];
    test.exe_filename =     var_map["exe"];
    test.compile_filename = var_map["compile"];
    test.output_filename =  var_map["out"];
    test.error_filename =   var_map["error"];

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
    // Fill in any variables in the code.
    test.processed_code = ApplyVars( emp::join(test.code, "\n") );

    // Add user-provided headers.
    std::stringstream processed_header;
    for (const auto & line : header) processed_header << ApplyVars(line) << "\n";

    test.GenerateCPP(processed_header.str(), var_map["dir"]);
  }

  void CompileTestCPP(Testcase & test) {
    // Add user-provided headers.
    for (std::string line : compile) {
      line = ApplyVars(line);
      std::cout << line << std::endl;
      test.compile_exit_code = std::system(line.c_str());
      std::cout << "Exit Code: " << test.compile_exit_code << std::endl;
    }

  }

  /// Run a specific test case.
  void RunTest(Testcase & test) {
    var_map["#test"] = emp::to_string(test.id);
    var_map["cpp"] = test.cpp_filename;
    var_map["exe"] = test.exe_filename;
    var_map["out"] = test.output_filename;
    var_map["compile"] = test.compile_filename;
    var_map["error"] = test.error_filename;

    // Running a test case has a series of phases.
    // Phase 1: Generate the CPP file to be tested (including provided header and instrumentation)
    GenerateTestCPP(test);

    // Phase 2: Compile the generated CPP file, reporting back any errors.
    CompileTestCPP(test);

    // Phase 3: Run the executable from the generated file, reporting back any errors.
    // Phase 4: Compare any outputs produced, reporting back any differences in those outputs.
    // Phase 5: Record any necessary point calculations and feedback that will be needed later.

    // @CAO: Should we allow a special symbol in the output to provide debug information without
    // affecting correctness?

  }

  // Add a new Testcase and run it.
  void AddTestcase(const std::string & args) {
    emp::notify::TestError(compile.size() == 0, "Cannot set up testcase without compile rules.");

    tests.emplace_back(tests.size());

    auto & test = tests.back();
    ConfigTestcase(test, args);
    LoadCode(test.code);
    RunTest(test);
  }

public:
  Emperfect() : file_scan(input_file) {
    // Initialize default values
    var_map["dir"] = ".emperfect";
    var_map["debug"] = "false";
    var_map["log"] = "Log.txt";
  }

  // Load test configurations from a stream.
  void Load(std::istream & is, std::string stream_name="input") {
    input_file.Load(is);
    input_file.RemoveComments("//");
    // NOTE: Do not change whitespace as it might matter for output code.
    
    // Setup the output file for all of the tests.
    std::string log_filename = emp::to_string(var_map["DIR"], "/", var_map["LOG"]);
    std::ofstream test_log(log_filename);
    test_log << "== EMPERFECT TEST LOG ==\n" << std::endl;
    test_log.close();  // Individual tests will re-open and append to the log.

    // Loop through the file and process each line.
    while (file_scan) {
      std::string line = ApplyVars( file_scan.Read() );
      if (emp::is_whitespace(line)) continue;  // Skip empty lines.

      // We are expecting a command, but don't get one, report an error.
      emp::notify::TestError(line[0] != ':',
        "Line ", file_scan.GetLine()-1, " in ", stream_name, " unknown\n", line, "\n");

      const std::string command = emp::to_lower( emp::string_pop_word(line) );
      if (command == ":init") Init(line);
      else if (command == ":compile") LoadCode(compile, line);
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
