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
#include <climits>
#include <iostream>
#include <memory>
#include <string>

#include "artifacts.hpp"
#include "lib/lang.hpp"
#include "lib/random.hpp"
#include "lib/logger.hpp"
#include "lib/naming.hpp"
#include "lib/transformations.hpp"
#include "lib/lowers.hpp"
#include "lib/chksum.hpp"

struct DeoptFunGenOpts {
  std::string uuid, sno;
  std::string output;
  bool main;
  bool verbose;

  static DeoptFunGenOpts Parse(int argc, char **argv) {
    cxxopts::Options options("Deopt", "Deopt: Using Reify's Deopt for leaf function generation\n");
    // clang-format off
    options.add_options()
      ("uuid", "An UUID identifier as the primary identifier", cxxopts::value<std::string>())
      ("n,sno", "A sample number as the second identifier", cxxopts::value<std::string>())
      ("o,output", "The directory saving the generated functions and mappings", cxxopts::value<std::string>())
      ("s,seed", "The seed for random sampling (negative values for truly random)", cxxopts::value<int>()->default_value("-1"))
      ("m,main", "Generate a main function with all mappings", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
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

    const bool main = args["main"].as<bool>();

    const bool verbose = args["verbose"].as<bool>();

    return {
        .uuid = uuid,
        .sno = sno,
        .output = output,
        .main = main,
        .verbose = verbose
    };
  }
};

// Very hacky way of overriding the checksum behavior of default reify
class SymCxLowerWithAssertReturn : public symir::SymCxLower {
public:
  explicit SymCxLowerWithAssertReturn(std::ostream &os, const symir::VarUse *retVar, const int retVal) :
      symir::SymCxLower(os), retVar(retVar), retVal(retVal) {}

  void Visit(const symir::RetStmt &r) override {
    indent();
    out << "assert(";
    retVar->Accept(*this);
    out << " == " << retVal << " && \"final value mismatches invalid transformation occures\");" << std::endl;
    indent();
    out << "return ";
    retVar->Accept(*this);
    out << ";" << std::endl;
  }
private:
  const symir::VarUse *retVar;
  const int retVal;
};

class DeoptFun {
public:
  DeoptFun(std::string name, int numParams) : name(name), numParams(numParams) {}

  void Generate() {
    Log::Get().OpenSection("Deopt function generation");
    auto builder = std::make_unique<symir::FunctBuilder>(name, symir::SymIR::I32);

    std::vector<const symir::VarDef *> params;
    params.reserve(numParams);
    for (int i = 0; i < numParams; i++) {
      params.push_back(builder->SymScaParam(NameVar(i), symir::SymIR::Type::I32, false));
    }

    auto retVal = builder->SymUnInitLocal(NameVar(numParams));
    this->targetVar = std::make_unique<symir::VarUse>(retVal);

    this->targetVal = Random::Get().Uniform(INT_MIN, INT_MAX)();
    auto bblBd = builder->OpenBlock(NameLabel(0));
    bblBd->SymCommitStmt(
      bblBd->SymAssStmt(
        retVal->GetDefinition(),
        bblBd->SymAddExpr({
          bblBd->SymCstTerm(
            builder->SymI32Const(this->targetVal),
            nullptr
          )
        })
      )
    );

    auto revopt = RewriteEngine();
    revopt.addRule(std::make_unique<VariableInjection>(), 3);
    revopt.addRule(std::make_unique<ConstToAdd>(), 3);
    revopt.addRule(std::make_unique<ConstToForSum>(), 2);
    revopt.addRule(std::make_unique<AssToDeadCode>(), 1);
    revopt.run(builder.get(), bblBd, 20);

    auto cstTerms = ConstQuery(builder.get(), bblBd).query();
    Log::Get().Out() << "Embedding variables:" << std::endl;
    for (size_t i = 0; i < cstTerms.size() && i < params.size(); i++) {
      Log::Get().Out() 
        << "  "
        << params[i]->GetName() 
        << " = " 
        << cstTerms[i]->GetCoef()->GetI32Value() 
        << std::endl;
    }

    std::map<const symir::Term *, symir::BlockBuilder::TermID> varMap;
    this->args.resize(numParams);
    for (size_t i = 0; i < cstTerms.size() && i < params.size(); i++) {
      this->args[i] = cstTerms[i]->GetCoef()->GetI32Value();
      varMap[cstTerms[i]] = bblBd->SymMulTerm(
        builder->SymI32Const(1),
        params[i], {}
      );
    }
    VariableEmbedder(builder.get(), bblBd).embed(varMap);

    bblBd->SymCommitStmt(bblBd->SymReturn());
    builder->CloseBlock(bblBd);

    this->fun = builder->Build();
    Log::Get().CloseSection();
  }

  std::string GenerateFunCode() const {
    std::ostringstream oss;
    oss << "#include <assert.h>" << std::endl;
    SymCxLowerWithAssertReturn lower(oss, this->targetVar.get(), this->targetVal);
    lower.Lower(*this->fun.get());
    return oss.str();
  }

  std::string GenerateMainCode() const {
    std::ostringstream main;
    main << "#include <stdio.h>" << std::endl;

    main << "extern int " << this->fun->GetName() << "(";
    for (int i = 0; i < numParams; i++) {
      main << "int";
      if (i != numParams - 1) {
        main << ", ";
      }
    }
    main << ");" << std::endl;

    main << "int main() {" << std::endl;
    main << "  printf(\"%d\\n\", " << this->fun->GetName() << "(";
    for (int i = 0; i < numParams; i++) {
      main << this->args[i];
      if (i != numParams - 1) {
        main << ", ";
      }
    }
    main << "));" << std::endl;
    main << "}";
    return main.str();
  }

private:
  std::string name;      // The name of the function
  int numParams;         // The number of parameters that the function has

  std::unique_ptr<symir::Funct> fun;
  std::unique_ptr<symir::VarUse> targetVar;
  std::vector<int> args;
  int targetVal;
};

int main(int argc, char **argv) {
  auto cliOpts = DeoptFunGenOpts::Parse(argc, argv);

  std::string uuid = cliOpts.uuid;
  std::string sno = cliOpts.sno;
  bool mainfun = cliOpts.main;
  bool verbose = cliOpts.verbose;

  FunArts arts(uuid, sno, cliOpts.output);
  fs::create_directories(arts.GetTestDir());

  if (verbose) {
    Log::Get().SetFout(arts.GetLogPath(/*devnull=*/false));
  } else {
    Log::Get().SetFout(arts.GetLogPath(/*devnull=*/true));
  }

  DeoptFun fun(arts.GetFunName(), 4);
  fun.Generate();

  std::string funCode = fun.GenerateFunCode();
  std::ofstream functionFile(arts.GetFunPath());
  functionFile << funCode << std::endl;
  functionFile.close();

  // Generate an executable program if necessary
  if (mainfun) {
    std::ofstream programFile = std::ofstream(arts.GetMainPath());
    programFile << fun.GenerateMainCode() << std::endl;
    programFile.close();
  }
}
