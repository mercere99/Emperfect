#include <iostream>

double Half(double x) { return x/2.0; }

int main()
{
  std::cout << "HELLO HALF WORLD!" << '\0'<< std::endl;
  std::cout << "2345678whitespace" << std::endl;
  std::cout << "There should be no space here: " << std::endl;
  std::cout << "There should be a space here:" << std::endl;
}
