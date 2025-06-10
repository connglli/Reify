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

#include <bits/stdc++.h>
#include <csignal>
#include <sstream>
#include <z3++.h>
#include "cxxopts.hpp"
#include "global.hpp"
#include "lib/block.hpp"
#include "lib/function.hpp"
#include "lib/graph.hpp"
#include "lib/logger.hpp"
#include "lib/random.hpp"

std::ofstream funcionFile;
std::ofstream mappingFile;
std::filesystem::path mapFilePath;
std::filesystem::path funcFilePath;

static bool isFormulaSatisfiable = false;

// Write a signal handler to handle SIGINT and SIGTERM signals
void cleanupIfEmpty() {
  if (funcionFile.is_open()) {
    funcionFile.close();
  }
  if (mappingFile.is_open()) {
    mappingFile.close();
  }
  // if output file is empty, remove it from the filesystem
  std::ifstream f(funcFilePath);
  std::ifstream f2(mapFilePath);
  if (f.peek() == std::ifstream::traits_type::eof() ||
      f2.peek() == std::ifstream::traits_type::eof()) {
    std::remove(funcFilePath.c_str());
    std::remove(mapFilePath.c_str());
  }
  f.close();
  f2.close();
}

void signalHandler(int signum) {
  Log::Get().Out() << "Interrupt signal (" << signum << ") received." << std::endl;
  if (isFormulaSatisfiable) {
    return;
  } else {
    if (funcionFile.is_open()) {
      funcionFile.close();
    }
    if (mappingFile.is_open()) {
      mappingFile.close();
    }
    cleanupIfEmpty();
    exit(signum);
  }
};

struct FunGenOpts {
  std::string uuid, sno;
  std::string output;
  bool verbose;

  static FunGenOpts Parse(int argc, char **argv) {
    cxxopts::Options options("fgen");
    // clang-format off
    options.add_options()
      ("uuid", "An UUID identifier as the primary identifier", cxxopts::value<std::string>())
      ("n,sno", "A sample number as the second identifier", cxxopts::value<std::string>())
      ("o,output", "The directory saving the generated functions and mappings", cxxopts::value<std::string>())
      ("s,seed", "The seed for random sampling (negative values for truly random)", cxxopts::value<int>()->default_value("-1"))
      ("v,verbose", "Enable verbose output", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("h,help", "Print help message", cxxopts::value<bool>()->default_value("false")->implicit_value("true"));
    options.parse_positional("uuid");
    options.positional_help("UUID");
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

    std::string sno;
    if (!args.count("sno")) {
      std::cerr << "Error: The sample number (--sno) is not given." << std::endl;
      exit(1);
    } else {
      sno = args["sno"].as<std::string>();
      if (std::stoi(sno) < 0) {
        std::cout << "Error: The sample number (--sno) must be non-negative." << std::endl;
        exit(1);
      }
    }

    std::string output;
    if (!args.count("output")) {
      std::cerr << "Error: The output directory (--output) is not given." << std::endl;
    } else {
      output = args["output"].as<std::string>();
    }

    if (args.count("seed")) {
      int seed = args["seed"].as<int>();
      if (seed >= 0) {
        Random::Get().Seed(seed);
      }
    }

    bool verbose;
    if (args.count("verbose")) {
      verbose = true;
    } else {
      verbose = false;
    }

    return {.uuid = uuid, .sno = sno, .output = output, .verbose = verbose};
  }
};

int main(int argc, char **argv) {
  auto cliOpts = FunGenOpts::Parse(argc, argv);

  std::string uuid = cliOpts.uuid;
  std::string sno = cliOpts.sno;
  bool verbose = cliOpts.verbose;

  std::filesystem::path outputDirectory = cliOpts.output;
  std::filesystem::path functionsDirectory = GetFunctionsDir(outputDirectory);
  std::filesystem::path mappingsDirectory = GetMappingsDir(outputDirectory);
  std::filesystem::create_directories(outputDirectory);
  std::filesystem::create_directories(functionsDirectory);
  std::filesystem::create_directories(mappingsDirectory);

  funcFilePath = GetFunctionPath(uuid, sno, outputDirectory);
  mapFilePath = GetMappingPath(uuid, sno, outputDirectory);
  funcionFile = std::ofstream(funcFilePath);
  mappingFile = std::ofstream(mapFilePath);
  Log::Get().SetFout(GetGenLogPath(uuid, sno, outputDirectory, /*devnull=*/!verbose));

  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGKILL, signalHandler);

  z3::set_param("parallel.enable", true);

  Log::Get().Out() << "==== Generation =============================" << std::endl;

  FunGen f(NUM_NODES_PER_FUNC, NUM_VARS_PER_FUNC);
  f.Generate();

  auto cfg = f.GetCFGBkbone();
  auto basicBlocks = f.GetBBs();

  auto execution = f.SampleExec(100, ENABLE_CONSISTENT_WALKS);
  Log::Get().Out() << "Exec: ";
  for (int i: execution) {
    Log::Get().Out() << i << ",";
  }
  Log::Get().Out() << std::endl;

  z3::context c;
  z3::solver solver(c);

  Log::Get().Out() << "==== Initialization 0 =============================" << std::endl;

  // Let each parameter (coefficient or constant) interesting initially
  if (ENABLE_INTERESTING_INITS) {
    solver.add(f.MakeInitialisationsInteresting(c));
  }
  if (ENABLE_RANDOM_INITS) {
    solver.add(f.AddRandomInitialisations(c));
  }

  Log::Get().Out() << "Initialization SMT: " << solver.to_smt2() << std::endl;

  // Generate UB constraint for each statement in the executed basic block
  std::unordered_map<std::string, int> versions; // SSA version for each definition
  for (int i = 0; i < execution.size() - 1; i++) {
    Log::Get().Out() << "Constraint BB#" << i << std::endl;
    int current_bb = execution[i];
    int next_bb = execution[i + 1];
    // Generate constraints for the current basic block so that
    // no UB occurs and it can reach the next basic block
    basicBlocks[current_bb].GenerateConstraints(next_bb, solver, c, versions);
  }
  basicBlocks[execution[execution.size() - 1]].GenerateConstraints(-1, solver, c, versions);

  // Dump the SMT queries for debugging
  Log::Get().Out() << "Execution SMT: " << solver.to_smt2() << std::endl;

  if (solver.check() == z3::unsat) {
    Log::Get().Out() << "UNSAT" << std::endl;
    // Remove the output file and mapping file from the filesystem because no model was found
    funcionFile.close();
    mappingFile.close();
    std::remove((funcFilePath).c_str());
    std::remove((mapFilePath).c_str());
    return 1;
  }

  isFormulaSatisfiable = true;
  Log::Get().Out() << "SAT" << std::endl;

  z3::model model = solver.get_model();
  Log::Get().Out() << "Execution Model: " << model << std::endl;

  f.ExtractParametersFromModel(model, c);
  funcionFile << f.GenerateCode(sno, uuid);
  funcionFile.close();

  std::vector<int> initialisation = f.ExtractInitialisationsFromModel(model, c);
  std::vector<int> finalisation = f.ExtractFinalizationsFromModel(model, c, versions);
  mappingFile << FunGen::GenerateMapping(initialisation, finalisation);

  // Now, let's try to generate a couple more mappings (initialisation sets).
  // First, let's regenerate the SMT query by repopulating the solver with the
  // same constraints, except this time - the solver would use the coefficients
  // and constants we already generated from the model.
  solver.reset();
  if (ENABLE_INTERESTING_INITS) {
    solver.add(f.MakeInitialisationsInteresting(c));
  }
  if (ENABLE_RANDOM_INITS) {
    solver.add(f.AddRandomInitialisations(c));
  }

  versions.clear();
  for (int i = 0; i < execution.size() - 1; i++) {
    int current_bb = execution[i];
    int next_bb = execution[i + 1];
    basicBlocks[current_bb].GenerateConstraints(next_bb, solver, c, versions);
  }
  basicBlocks[execution[execution.size() - 1]].GenerateConstraints(-1, solver, c, versions);

  for (int i = 0; i < NUM_INITS_PER_WALK - 1; i++) {
    Log::Get().Out() << "==== Initialization " << i + 1
                     << " =============================" << std::endl;

    // Ensure that the initialisation is sufficiently different from the previous one
    solver.add(f.DifferentInitialisationConstraint(initialisation, c));

    Log::Get().Out() << "Execution SMT: " << solver.to_smt2() << std::endl;

    if (solver.check() == z3::unsat) {
      Log::Get().Out() << "UNSAT" << std::endl;
      std::cout << "WARNING: UNSAT at " << i + 1 << "-th initialization" << std::endl;
      mappingFile.close();
      return 0; // There's no need to report an error to our caller
    }

    isFormulaSatisfiable = true;
    Log::Get().Out() << "SAT" << std::endl;

    model = solver.get_model();
    Log::Get().Out() << "Execution Model: " << model << std::endl;

    f.ExtractParametersFromModel(model, c);
    initialisation = f.ExtractInitialisationsFromModel(model, c);
    finalisation = f.ExtractFinalizationsFromModel(model, c, versions);
    mappingFile << FunGen::GenerateMapping(initialisation, finalisation);
  }

  mappingFile.close();

  return 0;
}
