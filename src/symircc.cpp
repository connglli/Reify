// MIT License
//
// Copyright (c) 2025
//
// Kavya Chopra (chopra.kavya04@gmail.com)
// Cong Li (cong.li@inf.ethz.ch)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "lib/chksum.hpp"
#include "lib/logger.hpp"
#include "lib/lowers.hpp"
#include "lib/parsers.hpp"

namespace fs = std::filesystem;

int main(int argc, char **argv) {
  cxxopts::Options options("symircc", "A compiler for SymIR");

  options.add_options()
        ("target", "Target language (cxx)", cxxopts::value<std::string>()->default_value("cxx"))
        ("o,output", "Output file", cxxopts::value<std::string>())
        ("M,main", "Generate a main function (with all parameters initialized to zero)", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage")
        ("input", "Input file", cxxopts::value<std::string>());

  options.parse_positional("input");
  options.positional_help("FILE");

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (!result.count("input")) {
    std::cerr << "Error: No input file specified." << std::endl;
    return 1;
  }

  std::string inputFile = result["input"].as<std::string>();
  std::string target = result["target"].as<std::string>();
  std::string outputFile;

  if (result.count("output")) {
    outputFile = result["output"].as<std::string>();
  } else {
    fs::path p(inputFile);
    if (target == "cxx") {
      p.replace_extension(".c");
    } else {
      p.replace_extension(".out");
    }
    outputFile = p.string();
  }

  if (!fs::exists(inputFile)) {
    std::cerr << "Error: Input file '" << inputFile << "' does not exist." << std::endl;
    return 1;
  }

  // Read input file
  std::ifstream t(inputFile);
  std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

  // Parse SymIR
  symir::SymSexpParser parser(str);
  try {
    parser.Parse();
  } catch (const std::exception &e) {
    std::cerr << "Error parsing SymIR: " << e.what() << std::endl;
    return 1;
  }

  auto funct = parser.TakeFunct();
  if (!funct) {
    std::cerr << "Error: No function found in input file." << std::endl;
    return 1;
  }

  // Generate Output
  std::ofstream out(outputFile);
  if (!out.is_open()) {
    std::cerr << "Error: Could not open output file '" << outputFile << "'." << std::endl;
    return 1;
  }

  if (target == "cxx") {
    // Generate C code
    out << StatelessChecksum::GetRawCode() << std::endl;
    out << "#include <stdio.h>" << std::endl;
    out << "#include <stdint.h>" << std::endl;
    out << std::endl;

    // Function Prototype and struct defs
    out << symir::SymCxLower::GetFunPrototype(*funct) << ";" << std::endl;
    out << std::endl;

    // Function Body
    symir::SymCxLower lower(out);
    lower.Lower(*funct);

    // Main function
    if (result["main"].as<bool>()) {
      out << std::endl;
      out << "int main(int argc, char **argv) {" << std::endl;
      out << "    " << StatelessChecksum::GetCrc32InitName() << "();" << std::endl;

      // Initialize parameters with zero
      for (const auto &p: funct->GetParams()) {
        if (p->IsVector()) {
          if (p->GetType() == symir::SymIR::STRUCT) {
            out << "    struct " << p->GetStructName() << " " << p->GetName();
          } else {
            out << "    int " << p->GetName();
          }
          for (int dim: p->GetVecShape()) {
            out << "[" << dim << "]";
          }
          out << " = {0};" << std::endl;
        } else if (p->GetType() == symir::SymIR::STRUCT) {
          out << "    struct " << p->GetStructName() << " " << p->GetName() << " = {0};"
              << std::endl;
        } else {
          out << "    int " << p->GetName() << " = 0;" << std::endl;
        }
      }

      // Call function and print result
      out << "    int res = " << funct->GetName() << "(";
      auto params = funct->GetParams();
      for (size_t i = 0; i < params.size(); ++i) {
        out << params[i]->GetName();
        if (i != params.size() - 1)
          out << ", ";
      }
      out << ");" << std::endl;
      out << "    printf(\"Result: %d\\n\", res);" << std::endl;
      out << "    return 0;" << std::endl;
      out << "}" << std::endl;
    }

  } else {
    std::cerr << "Error: Unsupported target '" << target << "'. Only 'cxx' is supported."
              << std::endl;
    return 1;
  }

  out.close();
  return 0;
}
