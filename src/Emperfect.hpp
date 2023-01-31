/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  Emperfect.hpp
 *  @brief Main driver for Emperfect unit testing
 * 
 *  URGENT
 *
 *  SOON
 *  @todo Fix line numbers in output to students.
 *  @todo Make shorter compile output still scroll horizontally if needed.
 *  @todo REQUIRE specific function signatures
 *  @todo Generate a full working code file for students to run the test cases locally.
 *  @todo Show the student any command-line arguments used.
 *  @todo make var_map["DIR"] work properly (i.e., case insensitive)
 *  @todo Figure out why quotes aren't removed from :Init dir=".emperfect"
 * 
 *  USEFUL
 *  @todo Make log actually work.  (var_map["log"])
 *  @todo Refactor most of test-case running into Testcase.hpp
 *  @todo Allow a special symbol in the output to exclude from comparisons?  E.g., lines starting with %.
 *  @todo If multi-line compile, make output append for every line past the first.
 *  @todo Collect times for how long test cases actually took.
 *  @todo Add a "contact your instructors" error message for things that shouldn't break.
 *  @todo Web interface for building a config file.
 *  @todo Do a better job with scrolling text.  Maybe make wrap horizontally (as on the command line)?
 *  @todo Allow a testcase to provide more dynamic feedback based on student errors.
 *  @todo Add a way to "check near", perhaps a =~ operator in checks or some such?
 */

#ifndef EMPERFECT_EMPERFECT_HPP
#define EMPERFECT_EMPERFECT_HPP

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "emp/base/notify.hpp"
#include "emp/base/vector.hpp"
#include "emp/datastructs/map_utils.hpp"
#include "emp/io/File.hpp"

#include "OutputInfo.hpp"
#include "Testcase.hpp"

// Set -DEMPERFECT_COMMENT on the command line to change internal comment marker (removed for students).
#ifndef EMPERFECT_COMMENT
#define EMPERFECT_COMMENT "///"
#endif

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
      else if (arg == "filename") output.SetFilename(value);
      else if (arg == "type") output.SetType(value);
      else {
        emp::notify::Error("Uknown :Output argument '", arg, "'.");
      }
    }
  }

  // Use a set of arguments to configure a testcase.
  void ConfigTestcase(Testcase & test, const std::string & args) {
    // Initialize any special values that may not be set.
    std::string file_base = emp::to_string(var_map["dir"], "/Test", test.id);
    var_map["compile"] = file_base + "-compile.txt";
    var_map["cpp"] =     file_base + ".cpp";
    var_map["error"] =   file_base + "-errors.txt";
    var_map["exe"] =     file_base + ".exe";
    var_map["out"] =     file_base + "-output.txt";
    var_map["result"] =  file_base + "-result.txt";

    // Allow these to be overwritten by settings, and then lock them into the testcase.
    auto setting_map = LoadVars(args);
    test.compile_filename = var_map["compile"];
    test.cpp_filename =     var_map["cpp"];
    test.exe_filename =     var_map["exe"];
    test.error_filename =   var_map["error"];
    test.output_filename =  var_map["out"];
    test.result_filename =  var_map["result"];

    for (auto [arg, value] : setting_map) {
      if (value.size() && value[0] == '\"') value = emp::from_literal_string(value);

      if (arg == "args") test.args = value;
      else if (arg == "code_file") test.code_filename = value;
      else if (arg == "expect") test.expect_filename = value;
      else if (arg == "hidden") test.hidden = ParseBool(value, "hidden");
      else if (arg == "input") test.input_filename = value;
      else if (arg == "match_case") test.match_case = ParseBool(value, "match_case");
      else if (arg == "match_space") test.match_space = ParseBool(value, "match_space");
      else if (arg == "name") test.name = value;
      else if (arg == "output") test.output_filename = value;
      else if (arg == "points") test.points = emp::from_string<double>(value);
      else if (arg == "result") test.result_filename = value;
      else if (arg == "run_main") test.call_main = ParseBool(value, "run_main");
      else if (arg == "timeout") test.timeout = emp::from_string<size_t>(value);
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

    test.GenerateCPP(processed_header.str());
  }

  void CompileTestCPP(Testcase & test) {
    // Add user-provided headers.
    for (std::string line : compile) {
      line = ApplyVars(line);
      std::cout << line << std::endl;
      test.compile_exit_code = std::system(line.c_str());
      std::cout << "Compile exit code: " << test.compile_exit_code << std::endl;
    }
  }

  bool RunTestExe(Testcase & test) {
    std::string run_command = emp::to_string("timeout ", test.timeout, " ./", test.exe_filename);
    if (test.args.size()) run_command += emp::to_string(" ", test.args);
    if (test.input_filename.size()) run_command += emp::to_string(" < ", test.input_filename);
    run_command += emp::to_string(" > ", test.output_filename, " 2> ", test.error_filename);
    std::cout << run_command << std::endl;
    test.run_exit_code = std::system(run_command.c_str()); // % 256;
    // Timeout exit code may be first byte or second byte.
    if (test.run_exit_code % 256 == 124 ||
        test.run_exit_code / 256 == 124) {
      test.hit_timeout = true;
      std::cout << "...Halted due to timeout." << std::endl;
    }
    std::cout << "Executable exit code: " << test.run_exit_code << std::endl;
    return (test.run_exit_code == 0);
  }

  void CompareTestResults(Testcase & test) {
    if (test.expect_filename.size()) {
      emp::File expect_output(test.expect_filename);
      emp::File exe_output(test.output_filename);

      if (test.match_case == false) {
        expect_output.Apply(emp::to_lower);
        exe_output.Apply(emp::to_lower);
      }
      if (test.match_space == false) {
        expect_output.RemoveWhitespace();
        exe_output.RemoveWhitespace();
      } else {
        // We always at least remove completely blank lines before comparisons.
        expect_output.RemoveEmpty();
        exe_output.RemoveEmpty();
      }

      test.output_match = (expect_output == exe_output);
      if (test.output_match) {
        std::cout << "Output match: Passed!" << std::endl;
      } else {
        std::cout << "Output match: Failed." << std::endl;
        expect_output.Write(std::cout);
        std::cout << "---" << std::endl;
        exe_output.Write(std::cout);
      }
    }
    else {
      test.output_match = true; // No output to match...
      std::cout << "No output to match." << std::endl;
    }
  }

  void RecordTestResults(Testcase & test) {
    emp::File result_file(test.result_filename);
    result_file.RemoveEmpty();
    size_t check_id = 0;
    for (std::string line : result_file) {
      std::string field = emp::string_pop_word(line);
      if (field == ":CHECK:") check_id = emp::from_string<size_t>(line);
      else if (field == ":TEST:") {} // We already have this...
      else if (field == ":RESULT:") {
        test.checks[check_id].passed = (line == "1");
        test.checks[check_id].resolved = true;
      }
      else if (field == ":LHS:") test.checks[check_id].lhs_value = line;
      else if (field == ":RHS:") test.checks[check_id].rhs_value = line;
      else if (field == ":MSG:") test.checks[check_id].error_out = line;
      else if (field == "SCORE") {
        test.score = emp::from_string<double>(line);
        std::cout << "Score = " << test.score << " of " << test.points << std::endl;
      }
      else emp::notify::Error("Unknown field in result file: ", field);
    }

    // And print results to the output files...
    for (auto & output : outputs) {
      test.PrintResult(output);
    }
  }


  /// Run a specific test case.
  void RunTest(Testcase & test) {
    var_map["#test"] = emp::to_string(test.id);
    var_map["compile"] = test.compile_filename;
    var_map["cpp"] = test.cpp_filename;
    var_map["error"] = test.error_filename;
    var_map["exe"] = test.exe_filename;
    var_map["out"] = test.output_filename;
    var_map["result"] = test.result_filename;

    // Running a test case has a series of phases.
    // Phase 1: Generate the CPP file to be tested (including provided header and instrumentation)
    GenerateTestCPP(test);

    // Phase 2: Compile the generated CPP file, reporting back any errors.
    CompileTestCPP(test);

    if (test.compile_exit_code == 0) {
      // Phase 3: Run the executable from the generated file, reporting back any errors.
      RunTestExe(test);

      // Phase 4: Compare any outputs produced, reporting back any differences in those outputs.
      CompareTestResults(test);
    }

    // Phase 5: Record any necessary point calculations and feedback.
    RecordTestResults(test);
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
    input_file.RemoveComments(EMPERFECT_COMMENT);
    // NOTE: Do not change whitespace as it might matter for output code.
    
    // Setup the output file for all of the tests.
    std::string log_filename = emp::to_string(var_map["dir"], "/", var_map["log"]);
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

    PrintSummary();
  }

  void Load(std::string filename) {
    std::ifstream file(filename);
    Load (file, filename);
  }

  double CountTotalPoints() const {
    double total = 0.0;
    for (const auto & test_case : tests) { total += test_case.points; }
    return total;
  }

  double CountEarnedPoints() const {
    double total = 0.0;
    for (const auto & test_case : tests) { total += test_case.EarnedPoints(); }
    return total;
  }

  double GetPercentEarned() const {
    return std::round( 100.0 * CountEarnedPoints() / CountTotalPoints() );
  }

  void PrintSummary_Text(std::ostream & out) {
    // Loop through test cases for printing to standard out.
    for (auto & test_case : tests) {
      out << test_case.id << " : " << test_case.name
          << " : passed " << test_case.CountPassed()
          << " of " << test_case.GetNumChecks() << " checks; "
          << test_case.EarnedPoints() << " points." << std::endl;
    }
    out << "\nFinal Score: " << GetPercentEarned() << std::endl;
  }
  
  void PrintSummary_HTML(std::ostream & out) {
    out << "\n<hr>\n<h1>Summary</h1>\n\n"
        << "<table style=\"background-color:#3fc0FF;\" cellpadding=\"5px\" border=\"1px solid black\" cellspacing=\"0\">"
        << "<tr><th>Test Case<th>Status<th>Checks<th>Passed<th>Failed<th>Score</tr>\n";

    for (auto & test_case : tests) {
      out << "<tr>" 
	// << "<td>" << test_case.id << ": <a href=\"#Test" << test_case.id << "\">" << test_case.name << "</a>"
          << "<td>" << test_case.id << ": " << test_case.name
          << "<td>" << test_case.GetStatusString()
          << "<td>" << test_case.GetNumChecks()
          << "<td>" << test_case.CountPassed()
          << "<td>" << test_case.CountFailed()
          << "<td>" << test_case.EarnedPoints() << " / " << test_case.points
          << "</tr>\n";
    }
      out << "<tr><th>" << "TOTAL"
          << "<td><td><td><td><td>" << CountEarnedPoints() << " / " << CountTotalPoints()
          << "</tr>\n";

    out << "</table>\n<h2>Final Score: <span style=\"color: blue\">"
        << GetPercentEarned() << "%</span></h2>\n<br><br><br>\n" << std::endl;
  }

  void PrintSummary() {
    for (auto & output : outputs) {
      if (output.HasSummary()) {
        if (output.IsHTML()) PrintSummary_HTML(output.GetFile());
        else PrintSummary_Text(output.GetFile());
      }
      else if (output.HasScore()) {
        output.GetFile() << CountEarnedPoints() << " of " << CountTotalPoints();
      }
      else if (output.HasPercent()) {
        output.GetFile() << GetPercentEarned() << "%" << std::endl;
      }
    }
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
