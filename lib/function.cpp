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


#include "lib/function.hpp"

#include <sstream>

#include "global.hpp"
#include "lib/logger.hpp"
#include "lib/naming.hpp"
#include "lib/random.hpp"
#include "lib/samputils.hpp"

void FunGen::Generate() {
  // Generate the shape of us
  cfg.Generate();
  for (int i = 0; i < numBBs; i++) {
    bbs.emplace_back(*this, i, cfg.GetAdj(i));
    bbs.back().Generate();
  }
}

std::vector<int> FunGen::SampleExec(int execStep, bool consistent) {
  std::vector<int> sampleWalk = {};
  int startNode = 0, endNode = numBBs - 1;
  if (consistent) {
    sampleWalk = cfg.SampleConsistentWalk(startNode, endNode, execStep);
  } else {
    sampleWalk = cfg.SampleWalk(startNode, endNode, execStep);
  }
  int stopNode = sampleWalk.back();
  if (stopNode == endNode) {
    return sampleWalk;
  }
  // This means that the stop node is not the end node, probably because we're
  // going in a cycle, so we need to add a pass counter to the stop node (as
  // if we're artificially creating an edge to the end node).
  // Modify the basic block at the end of the sample walk to have pass counter
  // equal to the number of times that basic block has been visited.
  int passCounter =
      static_cast<int>(std::ranges::count_if(sampleWalk, [=](auto n) { return n == stopNode; }));
  bbs[stopNode].SetPassCounter(passCounter);
  state[NamePassCounter()] = passCounter;
  sampleWalk.push_back(endNode);
  Log::Get().Out() << "Sample walk has been modified to end at the last node." << std::endl;
  return sampleWalk;
}

///////////////////////////////////////////////////////////////////
/////// Constraint Initialization and Finalization
///////////////////////////////////////////////////////////////////

z3::expr FunGen::MakeInitialisationsInteresting(z3::context &c) const {
  std::vector<z3::expr> allVars;
  for (int i = 0; i < numVars; i++) {
    allVars.push_back(c.int_const((NameVar(i) + "_0").c_str()));
  }
  // let's assign each variable a random value between -100 and 10
  return AtMostKZeroes(c, allVars, numVars / 2);
}

z3::expr FunGen::AddRandomInitialisations(z3::context &c) {
  auto rand = Random::Get().Uniform(
      GlobalOptions::Get().LowerInitBound, GlobalOptions::Get().UpperInitBound
  );
  std::vector<z3::expr> allVars;
  z3::expr allAssignedConstraint = c.bool_val(true);
  for (int i = 0; i < numVars; i++) {
    std::string varName = NameVar(i);
    allVars.push_back(c.int_const((varName + "_0").c_str()));
    z3::expr randomInit = c.bool_val(true);
    if (ParamDefined(varName)) {
      randomInit = (allVars[i] == GetParamVal(varName));
    } else {
      int randomValue = rand();
      DefineParam(varName, randomValue);
      randomInit = (allVars[i] == randomValue);
    }
    allAssignedConstraint = allAssignedConstraint && randomInit;
  }
  // let's assign each variable a random value between -100 and 10
  return allAssignedConstraint;
}

z3::expr
FunGen::DifferentInitialisationConstraint(std::vector<int> initialisation, z3::context &c) const {
  // There should be at lease NUM_VAR/2 variables which are not equal in both initialisations
  std::vector<z3::expr> differentInitialisationConstraints;
  for (int i = 0; i < numVars; i++) {
    int boundedValue1 = initialisation[i];
    z3::expr boundedValue2 = c.int_const((NameVar(i) + "_0").c_str());
    differentInitialisationConstraints.push_back(
        z3::ite(boundedValue1 != boundedValue2, c.int_val(1), c.int_val(0))
    );
  }
  z3::expr differentInitialisationConstraint =
      AtMostKZeroes(c, differentInitialisationConstraints, std::min(3, numVars / 2));
  return differentInitialisationConstraint;
}

// Function to extract parameters from the model, so that we can store the
// coefficients and constants that the solver found an interpretation for
void FunGen::ExtractParametersFromModel(z3::model &model, z3::context &ctx) {
  for (const auto &param: state) {
    const std::string &name = param.first;
    const auto key = ctx.int_const(name.c_str()).decl();

    if (model.has_interp(key)) {
      z3::expr value = model.get_const_interp(key);

      // For int values
      if (value.is_numeral()) {
        int int_value;
        if (Z3_get_numeral_int(ctx, value, &int_value)) {
          state[name] = int_value;
          Log::Get().Out() << "Parameter " << name << " is in the model with value: " << int_value
                           << std::endl;
        }
      }
    } else {
      Log::Get().Out() << "Parameter " << name << " is not explicitly defined in the model"
                       << std::endl;
    }
  }
}

// Extract the initial values of each variable that's an input to the function from the model
std::vector<int> FunGen::ExtractInitialisationsFromModel(z3::model &model, z3::context &ctx) const {
  std::vector<int> initialisations;
  for (int i = 0; i < numVars; i++) {
    std::string varName = NameVar(i) + "_0";
    z3::expr varExpr = ctx.int_const(varName.c_str());
    assert(model.has_interp(varExpr.decl()) && "Variable not found in model");
    if (model.has_interp(varExpr.decl())) {
      z3::expr value = model.get_const_interp(varExpr.decl());
      if (value.is_numeral()) {
        int intValue;
        if (Z3_get_numeral_int(ctx, value, &intValue)) {
          initialisations.push_back(intValue);
        } else {
          initialisations.push_back(0); // Default value
        }
      } else {
        initialisations.push_back(0); // Default value
      }
    } else {
      initialisations.push_back(0); // Default value
    }
  }
  return initialisations;
}

// i wanted a bit of flexibility for the checksum function in the future so i
// decided to just store the final values of all the variables (at the end basic
// block) instead of just the checksum
std::vector<int> FunGen::ExtractFinalizationsFromModel(
    z3::model &model, z3::context &ctx, std::unordered_map<std::string, int> &versions
) const {
  std::vector<int> finalisations;
  for (int i = 0; i < numVars; i++) {
    std::string varName = NameVar(i) + "_" + std::to_string(versions[NameVar(i)]);
    z3::expr varExpr = ctx.int_const(varName.c_str());
    assert(model.has_interp(varExpr.decl()) && "Variable not found in model");
    if (model.has_interp(varExpr.decl())) {
      z3::expr value = model.get_const_interp(varExpr.decl());
      if (value.is_numeral()) {
        int intValue;
        if (Z3_get_numeral_int(ctx, value, &intValue)) {
          finalisations.push_back(intValue);
        } else {
          finalisations.push_back(0); // Default value
        }
      } else {
        finalisations.push_back(0); // Default value
      }
    } else {
      finalisations.push_back(0); // Default value
    }
  }
  return finalisations;
}

///////////////////////////////////////////////////////////////////
/////// Code Generation
///////////////////////////////////////////////////////////////////

std::string FunGen::GenerateFunCode(const std::string &sno, const std::string &uuid) {
  std::ostringstream code;
  code << "int function_" << uuid << "_" << sno << "(";
  for (int i = 0; i < numVars; ++i) {
    code << "int " << NameVar(i);
    if (i < numVars - 1) {
      code << ", ";
    }
  }
  code << ")" << std::endl;
  code << "{" << std::endl;
  code << "    int " << NamePassCounter() << " = 0;" << std::endl;
  for (const auto &bb: bbs) {
    code << bb.GenerateCode() << std::endl;
  }
  code << "}" << std::endl;
  return code.str();
}

std::string FunGen::GenerateMainCode(
    const std::string &sno, const std::string &uuid,
    const std::vector<std::vector<int>> &initialisations,
    const std::vector<std::vector<int>> &finalizations
) {
  std::ostringstream main;
  main << "int main(int argc, int* argv[])" << std::endl;
  main << "{" << std::endl;
  for (auto init: initialisations) {
    auto numVars = init.size();
    main << "  function_" << uuid << "_" << sno << "(";
    for (auto i = 0; i < numVars; i++) {
      main << init[i];
      if (i < numVars - 1) {
        main << ", ";
      }
    }
    main << ");" << std::endl;
  }
  main << "  return 0;" << std::endl;
  main << "}" << std::endl;
  return main.str();
}

std::string FunGen::GenerateMapping(
    const std::vector<int> &initialisation, const std::vector<int> &finalisation
) {
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
