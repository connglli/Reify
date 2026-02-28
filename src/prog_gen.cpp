// MIT License
//
// Copyright (c) 2025-2025
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

#include <set>
#include <string>

#include "cxxopts.hpp"
#include "global.hpp"
#include "lib/logger.hpp"
#include "lib/program.hpp"
#include "lib/random.hpp"

namespace fs = std::filesystem;

struct ProgGenOpts {
  std::string uuid;
  std::string input;
  int limits;
  bool wasm;
  bool debug;

  static ProgGenOpts Parse(int argc, char **argv) {
    cxxopts::Options options("pgen");
    // clang-format off
    options.add_options()
      ("uuid", "An UUID identifier", cxxopts::value<std::string>())
      ("i,input", "The directory saving the seed functions and mappings", cxxopts::value<std::string>())
      ("l,limit", "The number of new programs to generate (0 for unlimited generation)", cxxopts::value<int>()->default_value("0"))
      ("s,seed", "The seed for random sampling (negative values for truly random)", cxxopts::value<int>()->default_value("-1"))
      ("W,wasm", "Also generate a Wasm program identical to the generated program", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("debug", "Enable debugging mode which add checksum check assertions", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("h,help", "Print help message", cxxopts::value<bool>()->default_value("false")->implicit_value("true"));
    options.parse_positional("uuid");
    options.positional_help("UUID");
    GlobalOptions::AddProgOpts(options);
    // clang-format on

    cxxopts::ParseResult args;
    try {
      args = options.parse(argc, argv);
    } catch (cxxopts::exceptions::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      exit(1);
    }

    if (args.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    std::string uuid;
    if (!args.count("uuid")) {
      std::cerr << "Error: The UUID identifier (UUID) is not given." << std::endl;
      exit(1);
    } else {
      uuid = args["uuid"].as<std::string>();
      // Replace uuid's non-alphanumeric characters with underscore
      // used a UUID so that i could throw everything from different runs
      // into the same directory without worrying about name clashes
      std::replace_if(
          uuid.begin(), uuid.end(), [](auto c) -> bool { return !std::isalnum(c); }, '_'
      );
    }

    std::string input;
    if (!args.count("input")) {
      std::cerr << "Error: The input directory (--input) is not given." << std::endl;
      exit(1);
    } else {
      input = args["input"].as<std::string>();
      if (!fs::exists(input)) {
        std::cerr << "Error: The input directory (--input) does not exist." << std::endl;
        exit(1);
      }
    }

    const int limit = args["limit"].as<int>();
    if (limit < 0) {
      std::cerr << "Error: The limit (--limit) must be greater than or equal to 0." << std::endl;
      exit(1);
    }

    if (const int seed = args["seed"].as<int>(); seed >= 0) {
      Random::Get().Seed(seed);
    }

    const bool wasm = args["wasm"].as<bool>();
    const bool debug = args["debug"].as<bool>();

    GlobalOptions::Get().HandleProgArgs(args);

    return {.uuid = uuid, .input = input, .limits = limit, .wasm = wasm, .debug = debug};
  }
};

int main(int argc, char *argv[]) {
  auto cliOpts = ProgGenOpts::Parse(argc, argv);

  std::string progUuid = cliOpts.uuid;

  fs::path inputDir = cliOpts.input;
  fs::path funsDir(GetWasmDir(inputDir));

  int genLimit = cliOpts.limits;
  bool wasm = cliOpts.wasm;
  bool enableDebug = cliOpts.debug;
  if (enableDebug) {
    Log::Get().SetCout();
  }

  // Read all function files from the input directory
  std::vector<std::string> allFunPaths;
  // Open the directory and read all files
  for (const auto &entry: fs::directory_iterator(funsDir)) {
    if (entry.is_regular_file()) {
      allFunPaths.push_back(entry.path());
    }
  }
  Assert(
      !allFunPaths.empty(), "No function files found in the input directory: %s", funsDir.c_str()
  );

  // Without a sorting, our results may not be deterministic as
  // the order of directory_iterator is not decidable.
  std::sort(allFunPaths.begin(), allFunPaths.end());

  for (int sampNo = 0; genLimit == 0 || sampNo < genLimit; ++sampNo) {
    Log::Get().Out() << "[" << sampNo << "] Generating ... " << std::endl;

    // Randomly select FUNCTION_DEPTH functions for the new program
    std::set<int> selFunInds;
    std::vector<std::string> selFunPaths;

    int numAll = static_cast<int>(allFunPaths.size());
    int numSel = std::min(GlobalOptions::Get().FunctionDepth, numAll);
    auto rand = Random::Get().Uniform(0, static_cast<int>(numAll - 1));

    while (selFunInds.size() < static_cast<size_t>(numSel)) {
      int index = rand();
      // TODO: Perhaps allow using functions more than once
      if (selFunInds.contains(index)) {
        continue;
      }
      Log::Get().Out() << "[" << sampNo << "] Selecting func#" << index << ": "
                       << allFunPaths[index] << std::endl;
      selFunInds.insert(index);
      selFunPaths.push_back(allFunPaths[index]);
    }

    // Now we construct our new program
    auto prog = std::make_unique<ProgPlus>(progUuid, sampNo, selFunPaths);
    prog->Generate();

    Log::Get().Out() << "[" << sampNo << "] Storing" << std::endl;

    // Generate Wasm code
    if (wasm) {
      // Make programs directory if doesn't exist yet
      fs::create_directories(GetProgramsDir(inputDir));
      prog->GenerateWasmCode(GetProgramsDir(inputDir) / (prog->GetName() + ".wat"), enableDebug);
    }
    // Generate C code
    else {
      prog->GenerateCode(GetProgramsDir(inputDir) / (prog->GetName() + ".c"), enableDebug);
      prog->GenerateCode(
        GetProgramsDir(inputDir) / (prog->GetName() + "_static.c"), enableDebug, true
      );
    }
  }
}
