#include <argparse/argparse.hpp>
#include <iostream>

int main(int argc, const char *argv[]) {
  argparse::ArgumentParser parser("example-00");

  parser.add_argument("-h", "--help").help("show help message and exit");
  parser.add_argument<std::vector<int>>("ints").help(
      "list of integers to sum together");
  parser.add_argument<unsigned int>("-b", "--base")
      .help("base to print the output in")
      .default_value(10)
      .implicit_value(16);
  parser.add_argument("file");

  std::cout << parser.help() << std::endl;

  /* parser.add_argument<int>("-i", "--integer")
      .help("Input an arbitrary integer value"); */
  return 0;
}
