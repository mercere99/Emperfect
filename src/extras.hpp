/**
 *  @note This file is part of Emperfect, https://github.com/mercere99/Emperfect
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file  extras.hpp
 *  @brief Any extra helper functions.
 * 
 */

#ifndef EMPERFECT_EXTRAS_HPP
#define EMPERFECT_EXTRAS_HPP

#include <string>

/// Flip the provided comparator to the opposite (so "<" becomes ">=").
std::string FlipComparator(std::string in) {
  if (in == "==") return "!=";
  if (in == "!=") return "==";
  if (in == "<")  return ">=";
  if (in == "<=") return ">";
  if (in == ">")  return "<=";
  if (in == ">=") return "<";
  return "";
}

#endif
