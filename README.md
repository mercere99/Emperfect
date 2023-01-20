# Emperfect

A unit test framework geared toward a classroom setting.

## Requirements

Must have access to the Empirical library (by default, assumed to be in a parallel directory).

## Usage

```
Emperfect config_file
```

Commands available to configure testing are:

| Command     | Description |
| ----------- | ----------- |
| `:Compile`  | Subsequent lines specify compile rules.  Use ${CPP} to specify generated c++ test file. |
| `:Header`   | Header information follows (e.g., #includes) to prepend to the beginning of generated c++ files. |
| `:Output`   | Output configuration. If `filename` is supplied, use as filename otherwise send to standard out;  `detail` specifies granularity of output.  Example: `:Output filename="student.html", detail="student"` |
| `:Testcase` | Code for the testcase follows (unless overridden); many setting are available to customize how the test case should be run (see below). |

Note that all commands are case insensitive.  More information about the settings associated with each of these commands is below.

### The `:Compile` Command

The compile command specifies how each of the test cases below should be compiled.  Each step of the compilation process is monitored and if any fail they are stored as error cases.  Once compilation has been completed successfully, the executable is run and output from that is reported back to the user (at the specified level of detail).

The settings on `:Compile` rarely need to be overridden.  They are:

| Setting |  Description                                               |  Usage                           |
| ------- | ---------------------------------------------------------- | -------------------------------- |
| `cpp`   | Name of the C++ files that Emperfect generates for testing | `cpp="emperfect_test_file_.cpp"` |
| `exe`   | Name of the executable program to run after compilation    | `exe="emperfect_test_exe_"`      |
| `debug` | Set to true to save generated files.                       | `debug=true`                     |

 The default filenames for `cpp` and `exe` are shown in usage; these are long names unlikely to clobber existing files.  The generated files will be removed once testing of a given case is finished unless debug is set to true.

In the compile rules provided on subsequent lines, you can use `${CPP}` to indicate the name of the Emperfect-generated cpp file that needs to be compiled, and `${EXE}` to indicate the executable to be generated.  A typical `:Compile` command usage might look like:

```
:Compile exe="CSE232_unit_test"
  g++ -std=c++20 -Wall -Wextra ${CPP} -o ${EXE}
```

### The `:Output` Command

The `:Output` command specifies which files to produce or what to print on the command line.  A typical configuration may have multiple output commands for information to be presented in multiple forms.  The available settings are:

|  Setting   |  Description                                                |  Usage                    |
| ---------- | ----------------------------------------------------------- | ------------------------- |
| `detail`   | Level of information to provide about test case results.    | `detail="summary"`        |
| `filename` | Name of output file; default: send results to standard out. | `filename="results.html"` |
| `type`     | Output format ("txt" or "html")                             | `type="html"`             |

Standard out defaults to "txt"; files default to file extension.  The `detail` levels that can be selected (from least to most informative) are:

|  Level    |  Output                                                                               |
| --------- | ------------------------------------------------------------------------------------- |
| "none"    | No output.                                                                            |
| "percent" | Percentage of passed cases overall (e.g., "60%")                                      |
| "score"   | Number of points earned overall (e.g. "70 / 100")                                     |
| "summary" | Pass/fail status for all (visible and hidden) test cases                              |
| "student" | Detailed information about failed visible cases, but only pass/fail status for others |
| "teacher" | Detailed information about all failed test cases.                                     |
| "full"    | Detailed information about all test cases, passed or failed.                          |
| "debug"   | Like "full", but include extra information about parsing to find errors.              |

### The `:Testcase` Command

Each TestCase starts with a `:Testcase` at the beginning of a line, followed by comma-separated settings.  Subsequent lines typically provide the testcase.  The following setting arguments are available:

| Setting       | Description                                              | Usage                     |
| ------------- | -------------------------------------------------------- | ------------------------- |
| `name`        | Name to use when reporting on test case.                 | `name="1: Test Square() function"` |
| `args`        | Command line arguments to provide. (default=none)        | `args="1 2 3"`            | 
| `run_main`    | Should student's main() function be run? (default=true)  | `run_main=true`             |
| `points`      | Number of points a test case is worth. (default=10.0)    | `points=30.0`             |
| `in_file`     | Name of the file to use as standard input (default=none) | `in_file="input01.txt"`   |
| `out_file`    | Expected output. If provided, must match (default=none)  | `out_file="output01.txt"` |
| `code_file`   | If provided, use file instead of local code that follows | `code_file="test01.cpp`   |
| `hidden`      | Should this test case be hidden? (default=false)         | `hidden=true`             |
| `match_case`  | Must output matches have same case? (default=true)       | `match_case=false`        |
| `match_space` | Must output matches have same whitespace? (default=true) | `match_space=false`       |

Example:

```
:TestCase run_main=false, name="Simple Tests", points=10.0  // All code is used until next :line.
  size_t x = 5;
  size_t y = 8;
  CHECK(x + 2 < y+1, "This is an error message.");

  std::string str = "Test string.";
  std::string error_middle = " multi-segment";
  CHECK(str == "Test string.", "This is an error message for a", error_middle, " string test.");
  CHECK(str == "Test string2.", "This is an error message for a string test that should fail.");
```

