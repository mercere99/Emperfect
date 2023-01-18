# Emperfect

A unit test framework geared toward a classroom setting.

## Requirements

Must have access to the Empirical library (by default, assumed to be in a parallel directory).

## Usage

```
Emperfect config_file
```

Commands available to configure testing are:

| Command | Description |
| ------- | ----------- |
| `:Header` | Header information follows (usually #includes) to prepend to the beginning of every test. |
| `:Testcase` | Code for the testcase follows (unless overridden); many setting are available to customize how the test case should be run (see below). |
| `:Output` | Output configuration; if `name` is supplied, use as filename otherwise send to standard out.  If `detail` setting specifies granularity of output for visible/hidden test cases; options are "none", "summary", "normal", "full", or "debug".  Example: `:Output filename="student.html", detail="normal/summary"` |
| `:GradeFile` | File info for the final grade. |

Each TestCase starts with a `:Testcase` at the beginning of a line, followed by comma-separated settings.  Subsequent lines typically provide the testcase.  The following setting types are available:

| Setting | Description | Usage |
| ------- | ----------- | ----- |
| `type`  | `"unit"` will suppress main(); `"io"` (default) will run main() | type="unit" |
| `name`  | Name to use when reporting on test case (default="Test #", counting from 1).  | `name="1: Testing Square() function"` |
| `score` | Number of points a test case is worth. (default=10.0) | `score=30.0` |
| `in_file` | Name of the file to use as standard input (default=none) | `in_file="input01.txt"` |
| `out_file` | Expected output. If provided, must match (default=none) | `out_file="output01.txt"` |
| `code_file` | If provided, uses file instead of local code that follows. | `code_file="test01.cpp` |
| `args` | Command line arguments to provide. (default=none) | `args="1 2 3"` |
| `hidden` | Should this test case be hidden? (default=false) | `hidden=true` |
| `match_case` | Must output matches have the same case? (default=true) | `match_case=false` |
| `match_space` | Must output matches have the same whitespace? (default=true) | `match_space=false` |

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

