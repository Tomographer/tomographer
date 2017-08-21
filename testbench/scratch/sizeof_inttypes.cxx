#include <cstddef>
#include <iostream>

int main()
{
  std::cout << "sizeof(short) = " << sizeof(short) << "\n";
  std::cout << "sizeof(int) = " << sizeof(int) << "\n";
  std::cout << "sizeof(long) = " << sizeof(long) << "\n";
  std::cout << "sizeof(long long) = " << sizeof(long long) << "\n";
  std::cout << "sizeof(std::size_t) = " << sizeof(std::size_t) << "\n";
  std::cout << "sizeof(std::ptrdiff_t) = " << sizeof(std::ptrdiff_t) << "\n";
  return 0;
}
