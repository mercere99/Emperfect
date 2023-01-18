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
| `:Header`   | Header information follows (usually #includes) to prepend to the beginning of generated c++ files. |
| `:Output`   | Output configuration. If `filename` is supplied, use as filename otherwise send to standard out;  `detail` specifies granularity of output.  Example: `:Output filename="student.html", detail="student"` |
| `:Testcase` | Code for the testcase follows (unless overridden); many setting are available to customize how the test case should be run (see below). |

Note that all commands are case insensitive.  More information about the settings associated with each of these commands is below.

### :Output

The `:Output` command specifies which files to produce or what to print on the command line.  A typical configuration may have multiple output commands for information to be presented in multiple forms.  The available settings are:

| Setting    | Description | Usage |
| ---------- | ----------- | ----- |
| `detail`   | Level of information to provide about test case results. | `detail="summary"` |
| `filename` | Name of the file to be used for output.  If no name is provided, send results to standard out. | `filename="results.html"` |
| `type`     | What format should the output be in? Options are "txt" (default for standard out) or "html" (default for files) | `type="html"` |

The `detail` levels translate to:

| Level     | Output |
| --------- | ----------- |
| "none"    | No output. |
| "score"   | Number of points earned overall (e.g. "70 / 100") |
| "percent" | Percentage of passed cases overall (e.g., "60%")  |
| "summary" | Pass/fail status for all (visible and hidden) test cases |
| "student" | Detailed information about failed visible cases, but only pass/fail status for others |
| "teacher" | Detailed information about all failed test cases. |
| "full"    | Detailed information about all test cases, passed or failed. |
| "debug"   | Like "full", but include extra information about parsing to find errors. |

### :Testcase

Each TestCase starts with a `:Testcase` at the beginning of a line, followed by comma-separated settings.  Subsequent lines typically provide the testcase.  The following setting types are available:

| Setting       | Description                                              | Usage                     |
| ------------- | -------------------------------------------------------- | ------------------------- |
| `type`        | `"unit"` suppressed main(); `"io"` (default) runs main() | `type="unit"`             |
| `name`        | Name to use when reporting on test case.                 | `name="1: Test Square() function"` |
| `score`       | Number of points a test case is worth. (default=10.0)    | `score=30.0`              |
| `in_file`     | Name of the file to use as standard input (default=none) | `in_file="input01.txt"`   |
| `out_file`    | Expected output. If provided, must match (default=none)  | `out_file="output01.txt"` |
| `code_file`   | If provided, use file instead of local code that follows | `code_file="test01.cpp`   |
| `args`        | Command line arguments to provide. (default=none)        | `args="1 2 3"`            | 
| `hidden`      | Should this test case be hidden? (default=false)         | `hidden=true`             |
| `match_case`  | Must output matches have same case? (default=true)       | `match_case=false`        |
| `match_space` | Must output matches have same whitespace? (default=true) | `match_space=false`       |

Example:

```
:TestCase type="unit", name="Simple Tests", score=10.0  // All code is used until next :line.
  size_t x = 5;
  size_t y = 8;
  CHECK(x + 2 < y+1, "This is an error message.");

  std::string str = "Test string.";
  std::string error_middle = " multi-segment";
  CHECK(str == "Test string.", "This is an error message for a", error_middle, " string test.");
  CHECK(str == "Test string2.", "This is an error message for a string test that should fail.");
```

