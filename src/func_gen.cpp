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

#include <csignal>
#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>

#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/logger.hpp"
#include "lib/random.hpp"
#include "lib/ubfexec.hpp"

std::ofstream functionFile;
std::ofstream mappingFile;
std::filesystem::path mapFilePath;
std::filesystem::path funcFilePath;

static bool isFormulaSatisfiable = false;

// Write a signal handler to handle SIGINT and SIGTERM signals
void cleanupIfEmpty() {
  if (functionFile.is_open()) {
    functionFile.close();
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
    if (functionFile.is_open()) {
      functionFile.close();
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
  std::optional<std::string> graphdb;
  bool main;
  bool sexpression;
  bool javaclass;
  bool verbose;

  static FunGenOpts Parse(int argc, char **argv) {
    cxxopts::Options options("fgen");
    // clang-format off
    options.add_options()
      ("uuid", "An UUID identifier as the primary identifier", cxxopts::value<std::string>())
      ("n,sno", "A sample number as the second identifier", cxxopts::value<std::string>())
      ("o,output", "The directory saving the generated functions and mappings", cxxopts::value<std::string>())
      ("s,seed", "The seed for random sampling (negative values for truly random)", cxxopts::value<int>()->default_value("-1"))
      ("g,unstable-graphdb", "Path to the JSONL file of a graph database for guiding graph generation", cxxopts::value<std::string>())
      ("m,main", "Generate a main function with all mappings", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("S,sexpression", "Also generate the S Expression of the generated function", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("J,unstable-javaclass", "Also generate a Java class (bytecode) identical to the generated function", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("v,verbose", "Enable verbose output", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("h,help", "Print help message", cxxopts::value<bool>()->default_value("false")->implicit_value("true"));
    options.parse_positional("uuid");
    options.positional_help("UUID");
    GlobalOptions::AddFuncOpts(options);
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
        std::cerr << "Error: The sample number (--sno) must be non-negative." << std::endl;
        exit(1);
      }
    }

    std::string output;
    if (!args.count("output")) {
      std::cerr << "Error: The output directory (--output) is not given." << std::endl;
      exit(1);
    } else {
      output = args["output"].as<std::string>();
    }

    if (const int seed = args["seed"].as<int>(); seed >= 0) {
      Random::Get().Seed(seed);
    }

    std::optional<std::string> graphdb{};
    if (args.count("unstable-graphdb")) {
      const std::string file = args["unstable-graphdb"].as<std::string>();
      if (file.empty()) {
        std::cerr << "Error: The graph database file (--unstable-graphdb) is not a valid path."
                  << std::endl;
        exit(1);
      }
      if (!std::filesystem::exists(file)) {
        std::cerr << "Error: The graph database file (--unstable-graphdb) does not exist: " << file
                  << std::endl;
        exit(1);
      }
      graphdb = file;
    }
    const bool main = args["main"].as<bool>();
    const bool sexpression = args["sexpression"].as<bool>();
    const bool javaclass = args["unstable-javaclass"].as<bool>();
    const bool verbose = args["verbose"].as<bool>();

    GlobalOptions::Get().HandleFuncArgs(args);

    return {
        .uuid = uuid,
        .sno = sno,
        .output = output,
        .graphdb = graphdb,
        .main = main,
        .sexpression = sexpression,
        .javaclass = javaclass,
        .verbose = verbose
    };
  }
};

int main(int argc, char **argv) {
  auto cliOpts = FunGenOpts::Parse(argc, argv);

  std::string uuid = cliOpts.uuid;
  std::string sno = cliOpts.sno;
  std::optional<std::string> graphdb = cliOpts.graphdb;
  bool mainfun = cliOpts.main;
  bool sexpression = cliOpts.sexpression;
  bool javaclass = cliOpts.javaclass;
  bool verbose = cliOpts.verbose;

  std::filesystem::path outputDirectory = cliOpts.output;
  std::filesystem::create_directories(outputDirectory);
  std::filesystem::create_directories(GetFunctionsDir(outputDirectory));
  std::filesystem::create_directories(GetMappingsDir(outputDirectory));

  funcFilePath = GetFunctionPath(uuid, sno, outputDirectory);
  mapFilePath = GetMappingPath(uuid, sno, outputDirectory);
  if (verbose) {
    std::filesystem::create_directories(GetLoggingsDir(outputDirectory));
    Log::Get().SetFout(GetGenLogPath(uuid, sno, outputDirectory, /*devnull=*/false));
  } else {
    Log::Get().SetFout(GetGenLogPath(uuid, sno, outputDirectory, /*devnull=*/true));
  }

  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
  std::signal(SIGKILL, signalHandler);

  z3::set_param("parallel.enable", true);

  // Generate the function code
  FunPlus fun(
      GetFunctionName(uuid, sno), GlobalOptions::Get().NumVarsPerFun,
      GlobalOptions::Get().NumBblsPerFun, GlobalOptions::Get().MaxNumLoopsPerFun,
      GlobalOptions::Get().MaxNumBblsPerLoop
  );
  if (graphdb.has_value()) {
    std::cout << "WARNING: Using the graph database may cause timeout too frequently." << std::endl;
    fun.SetGraphSet(graphdb.value());
  }
  fun.Generate();

  // Populate symbols through the the execution path
  // We keep trying unless when we succeeded
  std::unique_ptr<UBFreeExec> exec = nullptr;
  for (int tries = 0; tries < GlobalOptions::Get().MaxNumExecsPerFun; ++tries) {
    // Sample an execution path from the function
    std::vector<int> execPath = fun.SampleExec(
        GlobalOptions::Get().MaxNumExecStepsPerFun, GlobalOptions::Get().EnableConsistentExecs
    );
    // Try solving the execution following the path
    exec = std::make_unique<UBFreeExec>(fun, execPath);
    int numSolved = exec->Solve(
        GlobalOptions::Get().NumInitsPerExec, GlobalOptions::Get().EnableInterestInits,
        GlobalOptions::Get().EnableRandomInits, GlobalOptions::Get().EnableInterestCoefs,
        /*debug=*/verbose
    );
    if (numSolved != 0) {
      break;
    }
    exec = nullptr;
  }

  // We are unable to find any available executable executions
  if (exec == nullptr) {
    std::cerr << "Unable to obtain any UB-free solutions within "
              << GlobalOptions::Get().MaxNumExecsPerFun << " execution samples" << std::endl;
    return -1; // Unable find any good solutions for the execution
    // TODO: Perhaps we can sample a new execution and fail after N samples?
  }

  // Generate our code with respect to the UB-free execution
  std::string funCode = fun.GenerateFunCode(*exec);
  functionFile = std::ofstream(funcFilePath);
  functionFile << funCode << std::endl;
  functionFile.close();

  // Generate the s-expressions if necessary
  if (sexpression) {
    std::string sexpCode = fun.GenerateFunSexpCode(*exec);
    std::filesystem::create_directories(GetSexpressionsDir(outputDirectory));
    std::ofstream sexpressionFile = std::ofstream(GetSexpressionPath(uuid, sno, outputDirectory));
    sexpressionFile << sexpCode << std::endl;
    sexpressionFile.close();
  }

  // Generate the initialization-finalization mapping
  mappingFile = std::ofstream(mapFilePath);
  mappingFile << fun.GenerateMappingCode(*exec);
  mappingFile.close();

  // Generate an executable program if necessary
  if (mainfun) {
    std::filesystem::create_directories(GetProgramsDir(outputDirectory));
    std::ofstream programFile = std::ofstream(GetProgramPath(uuid, sno, outputDirectory));
    programFile << StatelessChecksum::GetRawCode() << std::endl;
    programFile << funCode << std::endl;
    programFile << fun.GenerateMainCode(*exec, /*debug=*/verbose) << std::endl;
    programFile.close();
  }

  // Generate a Java class if necessary
  if (javaclass) {
    std::cout << "WARNING: The Java class generation is unstable and may not work as expected "
                 "(common failures include bytecode verification failures."
              << std::endl;
    std::filesystem::create_directories(GetJavaClassesDir(outputDirectory));
    Log::Get().OpenSection("Java Class Generation");
    // TODO: Copy the checksum classes to the output directory if they are not already there
    auto javaClass = fun.GenerateFunJavaCode(*exec, GetJavaClassName(uuid, sno), mainfun, verbose);
    Log::Get().CloseSection();
    jnif::stream::OClassFileStream ofs(
        GetJavaClassPath(uuid, sno, outputDirectory).c_str(), javaClass.get()
    );
  }

  return 0;
}
