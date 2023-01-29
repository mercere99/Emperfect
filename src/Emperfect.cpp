#include <iostream>

#include "emp/config/command_line.hpp"

#include "Emperfect.hpp"

int main(int argc, char *argv[])
{
  std::cout << "Welcome to Emperfect!" << std::endl;

  if (argc != 2) {
    std::cout << "Format: " << argv[0] << " [config filename]" << std::endl;
    exit(1);
  }

  Emperfect control;
  control.Load(argv[1]);
  // control.PrintDebug();
}
