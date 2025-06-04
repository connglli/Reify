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

std::ofstream logFile;
std::ofstream outputFile;
std::ofstream mappingFile;
std::filesystem::path logFileName;
std::filesystem::path mappingFileName;
std::filesystem::path outputFileName;


static bool isFormulaSatisfiable = false;

std::string generateMapping(const std::vector<int> &initialisation, const std::vector<int> &finalisation) {
  std::ostringstream mapping;
  for (auto x: initialisation) {
    mapping << x << ",";
  }
  mapping << " : ";
  for (auto x: finalisation) {
    mapping << x << ",";
  }
  mapping << std::endl;
  return mapping.str();
}

// write a signal handler to handle SIGINT and SIGTERM signals
void cleanupIfEmpty() {
  if (outputFile.is_open()) {
    outputFile.close();
  }
  if (mappingFile.is_open()) {
    mappingFile.close();
  }
  // if output file is empty, remove it from the filesystem
  std::ifstream f(outputFileName);
  std::ifstream f2(mappingFileName);
  if (f.peek() == std::ifstream::traits_type::eof() || f2.peek() == std::ifstream::traits_type::eof()) {
    std::remove(outputFileName.c_str());
    std::remove(mappingFileName.c_str());
  }
  f.close();
  f2.close();
}

void signalHandler(int signum) {
  logFile << "Interrupt signal (" << signum << ") received.\n";
  if (isFormulaSatisfiable) {
    return;
  } else {
    if (outputFile.is_open()) {
      outputFile.close();
    }
    if (mappingFile.is_open()) {
      mappingFile.close();
    }
    cleanupIfEmpty();
    exit(signum);
  }
};

struct CliOpts {
  int sno;
  std::string uuid;
  bool verbose;
};

CliOpts parseCliOpts(int argc, char **argv) {
  cxxopts::Options options("fgen");
  // clang-format off
  options.add_options()
    ("uuid", "An UUID identifier", cxxopts::value<std::string>())
    ("n,sno", "A sample number", cxxopts::value<int>())
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

  int sno = -1;
  if (!args.count("sno")) {
    std::cerr << "Error: The sample number (--sno) is not given." << std::endl;
    exit(1);
  } else {
    sno = args["sno"].as<int>();
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
    std::replace_if(uuid.begin(), uuid.end(), [](auto c) -> bool { return !std::isalnum(c); }, '_');
  }

  bool verbose;
  if (args.count("verbose")) {
    verbose = true;
  } else {
    verbose = false;
  }

  return {.sno = sno, .uuid = uuid, .verbose = verbose};
}

int main(int argc, char **argv) {
  auto args = parseCliOpts(argc, argv);

  int sno = args.sno;
  std::string uuid = args.uuid;
  bool verbose = args.verbose;

  mappingFileName = GetMappingPath(uuid, sno);
  mappingFile = std::ofstream(mappingFileName);
  outputFileName = GetProcedurePath(uuid, sno);
  outputFile = std::ofstream(outputFileName);
  logFileName = GetGenLogPath(uuid, sno, /*devnull=*/!verbose);
  logFile = std::ofstream(logFileName);

  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGKILL, signalHandler);

  z3::set_param("parallel.enable", true);

  logFile << "==== Generation =============================" << std::endl;

  Func f(NUM_NODES_PER_FUNC, NUM_VARS_PER_FUNC);
  f.GenCFG();

  auto g = f.GetUdlyGraph();
  auto basicBlocks = f.GetBBs();

  auto execution = f.SampleExec(100, ENABLE_CONSISTENT_WALKS);
  logFile << "Exec: ";
  for (int i: execution) {
    logFile << i << ",";
  }
  logFile << std::endl;

  z3::context c;
  z3::solver solver(c);

  logFile << "==== Initialization 0 =============================" << std::endl;

  // Let each parameter (coefficient or constant) interesting initially
  if (ENABLE_INTERESTING_INITS) {
    solver.add(f.MakeInitialisationsInteresting(c));
  }
  if (ENABLE_RANDOM_INITS) {
    solver.add(f.AddRandomInitialisations(c));
  }

  logFile << "Initialization SMT: " << solver.to_smt2() << std::endl;

  // Generate UB constraint for each statement in the executed basic block
  std::unordered_map<std::string, int> versions; // SSA version for each definition
  for (int i = 0; i < execution.size() - 1; i++) {
    logFile << "Constraint BB#" << i << std::endl;
    int current_bb = execution[i];
    int next_bb = execution[i + 1];
    // Generate constraints for the current basic block so that
    // no UB occurs and it can reach the next basic block
    basicBlocks[current_bb].GenerateConstraints(next_bb, solver, c, versions);
  }
  basicBlocks[execution[execution.size() - 1]].GenerateConstraints(-1, solver, c, versions);

  // Dump the SMT queries for debugging
  logFile << "Execution SMT: " << solver.to_smt2() << std::endl;

  if (solver.check() == z3::unsat) {
    logFile << "UNSAT" << std::endl;
    // Remove the output file and mapping file from the filesystem because no model was found
    outputFile.close();
    mappingFile.close();
    std::remove((outputFileName).c_str());
    std::remove((mappingFileName).c_str());
    logFile.close();
    return 1;
  }

  isFormulaSatisfiable = true;
  logFile << "SAT" << std::endl;

  z3::model model = solver.get_model();
  logFile << "Execution Model: " << model << std::endl;

  f.ExtractParametersFromModel(model, c);
  outputFile << f.GenerateCode(sno, uuid);
  outputFile.close();

  std::vector<int> initialisation = f.ExtractInitialisationsFromModel(model, c);
  std::vector<int> finalisation = f.ExtractFinalizationsFromModel(model, c, versions);
  mappingFile << generateMapping(initialisation, finalisation);

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
    logFile << "==== Initialization " << i + 1 << " =============================" << std::endl;

    // Ensure that the initialisation is sufficiently different from the previous one
    solver.add(f.DifferentInitialisationConstraint(initialisation, c));

    logFile << "Execution SMT: " << solver.to_smt2() << std::endl;

    if (solver.check() == z3::unsat) {
      logFile << "UNSAT" << std::endl;
      mappingFile.close();
      logFile.close();
      return 1;
    }

    isFormulaSatisfiable = true;
    logFile << "SAT" << std::endl;

    model = solver.get_model();
    logFile << "Execution Model: " << model << std::endl;

    f.ExtractParametersFromModel(model, c);
    initialisation = f.ExtractInitialisationsFromModel(model, c);
    finalisation = f.ExtractFinalizationsFromModel(model, c, versions);
    mappingFile << generateMapping(initialisation, finalisation);
  }

  mappingFile.close();
  logFile.close();

  return 0;
}
