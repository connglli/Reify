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
#include <optional>
#include <signal.h>
#include <unistd.h>
#include "better_graph.hpp"
#include "cxxopts.hpp"
#include "func_params.hpp"
#include "params.hpp"
#include "z3++.h"

std::ofstream logFile;
std::ofstream outputFile;
std::ofstream mappingFile;
std::filesystem::path logFileName;
std::filesystem::path mappingFileName;
std::filesystem::path outputFileName;

static bool isFormulaSatisfiable = false;

using VarIndex = int;
using BasicBlockNo = int;

z3::expr atMostKZeroes(z3::context &ctx, const std::vector<z3::expr> &vec, int k, int val) {
  z3::expr_vector zero_constraints(ctx);

  for (const auto &expr: vec) {
    zero_constraints.push_back(z3::ite(expr == val, ctx.int_val(1), ctx.int_val(0)));
  }

  z3::expr sum_zeros = sum(zero_constraints);
  return sum_zeros <= k;
}

std::unordered_map<std::string, std::optional<int>> parameters;

std::vector<int> sampleKDistinct(int n, int k) {
  n -= 1;
  if (k > n + 1) {
    throw std::invalid_argument("Error: k must be at most n + 1 to sample k distinct numbers.");
  }
  std::vector<int> numbers(n + 1);
  for (int i = 0; i <= n; ++i) {
    numbers[i] = i;
  }
  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(numbers.begin(), numbers.end(), gen);

  return std::vector<int>(numbers.begin(), numbers.begin() + k);
}

// MOTIVATION: We need to generate a pass counter name for the block in case we
// end up in a consistent walk with a loop which never reaches the end node
std::string generatePassCounterName() { return "pass_counter"; }

std::string getVarName(int varIndex) { return "var_" + std::to_string(varIndex); }

std::string getCoefficientName(int blockno, int statementIndex, int statementSubIndex) {
  return "a_" + std::to_string(blockno) + "_" + std::to_string(statementIndex) + "_" +
         std::to_string(statementSubIndex);
}

std::string generateConstantName(int blockno, int statementIndex) {
  return "a_" + std::to_string(blockno) + "_" + std::to_string(statementIndex);
}

std::string generateLabelName(int blockno) { return "BB" + std::to_string(blockno); }

std::string generateConditionalCoefficientName(int blockno, int statementIndex, int statementSubIndex) {
  return "b_" + std::to_string(blockno) + "_" + std::to_string(statementIndex) + "_" +
         std::to_string(statementSubIndex);
}

std::string generateConditionalConstantName(int blockno, int statementIndex) {
  return "b_" + std::to_string(blockno) + "_" + std::to_string(statementIndex);
}

std::string generateConditionalConstraint(int blockno, int target, std::vector<VarIndex> conditionalVariables) {
  std::ostringstream constraint;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
  constraint << "    if (";
  int ctr = 0;
  for (auto i: conditionalVariables) {
    std::string coefficientKey = generateConditionalCoefficientName(blockno, 0, ctr);
    int coefficient;
    if (parameters.find(coefficientKey) != parameters.end() && parameters[coefficientKey].has_value()) {
      coefficient = parameters[coefficientKey].value();
    } else {
      coefficient = dist(gen);
      parameters[coefficientKey] = coefficient;
    }
    constraint << coefficient << " * " << getVarName(i);
    constraint << " + ";
    ++ctr;
  }
  std::string constantKey = generateConditionalConstantName(blockno, target);
  int constant;
  if (parameters.find(constantKey) != parameters.end() && parameters[constantKey].has_value()) {
    constant = parameters[constantKey].value();
  } else {
    constant = dist(gen);
    parameters[constantKey] = constant;
  }
  constraint << constant;
  constraint << " >= 0) goto " << generateLabelName(target) << ";\n";

  // Output the constraint
  return constraint.str();
}

std::string generateUnconditionalGoto(int target) {
  std::ostringstream code;
  code << "    goto " << generateLabelName(target) << ";\n";
  return code.str();
}

z3::expr makeCoefficientsInteresting(const std::vector<z3::expr> &coeffs, z3::context &c) {
  // z3::expr allZero = c.bool_const(("true"));
  // for(auto coeff: coeffs)
  // {
  //     allZero = allZero && (coeff == 0);
  // }
  // return !allZero;
  if (!enableInterestingCoefficients) {
    return c.bool_val(true);
  }
  return atMostKZeroes(c, coeffs, coeffs.size() / 2, 0);
}

z3::expr addRandomInitialisations(z3::context &c) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(LOWER_INIT_BOUND, UPPER_INIT_BOUND);
  std::vector<z3::expr> allVars;
  z3::expr allAssignedConstraint = c.bool_val(true);
  for (int i = 0; i < NUM_VARS; i++) {
    allVars.push_back(c.int_const((getVarName(i) + "_0").c_str()));
    z3::expr randomInit = c.bool_val(true);
    if (parameters.find(getVarName(i)) != parameters.end() && parameters[getVarName(i)].has_value()) {
      randomInit = (allVars[i] == parameters[getVarName(i)].value());
    } else {
      int randomValue = dist(gen);
      parameters[getVarName(i)] = randomValue;
      randomInit = (allVars[i] == randomValue);
    }
    allAssignedConstraint = allAssignedConstraint && randomInit;
  }
  // let's assign each variable a random value between -100 and 10
  return allAssignedConstraint;
}

z3::expr boundCoefficients(z3::context &c, const std::vector<z3::expr> &coeffs) {
  z3::expr allAssignedConstraint = c.bool_val(true);
  for (const auto &coeff: coeffs) {
    allAssignedConstraint = allAssignedConstraint && (coeff >= LOWER_COEFF_BOUND && coeff <= UPPER_COEFF_BOUND);
  }
  return allAssignedConstraint;
}

z3::expr makeInitialisationsInteresting(z3::context &c) {
  if (!enableInterestingInitialisations) {
    return c.bool_val(true);
  }
  std::vector<z3::expr> allVars;
  for (int i = 0; i < NUM_VARS; i++) {
    allVars.push_back(c.int_const((getVarName(i) + "_0").c_str()));
  }
  // let's assign each variable a random value between -100 and 10
  return atMostKZeroes(c, allVars, NUM_VARS / 2, 0);
}

void pushAfterSearch(std::vector<z3::expr> &coeffs, std::string name, z3::context &ctx) {
  // if we've already extracted the value from the model, we use that value,
  //  otherwise it's an uninterpreted constant which the solver needs to figure
  //  out the value for
  if (parameters.find(name) != parameters.end() && parameters[name].has_value()) {
    coeffs.push_back(ctx.int_val(parameters[name].value()));
  } else if (parameters.find(name) == parameters.end() ||
             (parameters.find(name) != parameters.end() && parameters[name] == std::nullopt)) {
    coeffs.push_back(ctx.int_const((name).c_str()));
    parameters[name] = std::nullopt;
  } else {
    assert(false && "This should not happen");
  }
}

z3::expr differentInitialisationConstraint(std::vector<int> initialisation, z3::context &c) {
  // there should be atlease NUM_VAR/2 variables which are not equal in both
  // initialisations
  std::vector<z3::expr> differentInitialisationConstraints;
  for (int i = 0; i < NUM_VARS; i++) {
    int boundedValue1 = initialisation[i];
    z3::expr boundedValue2 = c.int_const((getVarName(i) + "_0").c_str());
    differentInitialisationConstraints.push_back(z3::ite(boundedValue1 != boundedValue2, c.int_val(1), c.int_val(0)));
  }
  z3::expr differentInitialisationConstraint =
      atMostKZeroes(c, differentInitialisationConstraints, std::min(3, NUM_VARS / 2), 0);
  return differentInitialisationConstraint;
}

class BB {
public:
  BasicBlockNo blockno;
  int numStatements;
  std::map<VarIndex, std::vector<VarIndex>> statementMappings;
  std::vector<VarIndex> assignmentOrder;
  std::vector<BasicBlockNo> blockTargets;
  std::vector<VarIndex> conditionalVariables;
  bool needPassCounter = false;
  int passCounterValue = 0;

public:
  BB(BasicBlockNo blockno, const std::set<BasicBlockNo> &graphTargets) : blockno(blockno), needPassCounter(false) {
    numStatements = ASSIGNMENTS_PER_BB;
    assignmentOrder = sampleKDistinct(NUM_VARS, ASSIGNMENTS_PER_BB);
    int statementIndex = 0;
    for (auto varIndex: assignmentOrder) {
      statementMappings[varIndex] = sampleKDistinct(
          NUM_VARS,
          VARIABLES_PER_ASSIGNMENT_STATEMENT
      ); // sample the variables which
         // will be used in the RHS of the
         // assignment statement

      // check every element present in vector is less than NUM_VARS
      for (auto var: statementMappings[varIndex]) {
        if (var >= NUM_VARS) {
          throw std::invalid_argument("Error: variable index out of bounds");
        }
      }
      for (int i = 0; i < statementMappings[varIndex].size(); i++) {
        std::string coefficientKey = getCoefficientName(
            blockno, statementIndex,
            i
        ); // statement index is the index of the assignment statement in a
           // basic block, and i is the index of the variable in the RHS of
           // the assignment statement
        parameters[coefficientKey] = std::nullopt;
      }
      std::string constantKey = generateConstantName(blockno, statementIndex);
      parameters[constantKey] = std::nullopt;
      statementIndex++;
    }
    conditionalVariables = assignmentOrder;
    for (int i = 0; i < NUM_VARIABLES_IN_CONDITIONAL - assignmentOrder.size(); i++) {
      int randomVariable = std::rand() % NUM_VARS;
      while (std::find(conditionalVariables.begin(), conditionalVariables.end(), randomVariable) !=
             conditionalVariables.end()) {
        randomVariable = std::rand() % NUM_VARS;
      }
      conditionalVariables.push_back(randomVariable);
    }
    logFile << "size of conditional variables: " << conditionalVariables.size() << '\n';

    std::vector<BasicBlockNo> targets;
    for (auto target: graphTargets) {
      blockTargets.push_back(target);
    }
    if (blockTargets.size() > 1) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::shuffle(blockTargets.begin(), blockTargets.end(), gen);
      for (int i = 0; i < NUM_VARIABLES_IN_CONDITIONAL; i++) {
        std::string coefficientKey = generateConditionalCoefficientName(
            blockno, 0,
            i
        ); // 0 is the statement index here (even I'm not sure why i chose
           // this as a parameter but i don't want to break the code so
           // let's just not touch this), and i is the index of the
           // variable in the actual conditional
        parameters[coefficientKey] = std::nullopt;
      }
      std::string constantKey = generateConditionalConstantName(
          blockno,
          blockTargets[0]
      ); // target block is a parameter here if we ever
         // wanted to support more that 2 potential targets
         // for the same basic block
      parameters[constantKey] = std::nullopt;
    }
  }

  void setPassCounter(int value) {
    needPassCounter = true;
    passCounterValue = value;
  }

  std::string generateCode() const {
    std::ostringstream code;

    // Generate the label for the basic block
    code << generateLabelName(blockno) << ":\n";
    // code << "printf(\" at basic block"  << generateLabelName(blockno) <<
    // "\\n\");\n";
    if (needPassCounter) {
      code << generatePassCounterName() << "++;\n";
    }
    // code<< "    printf(\"starting off at
    // "<<generateLabelName(blockno)<<"\\n\");\n";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
    int statementIndex = 0;
    for (const auto &[varIndex, dependencies]: statementMappings) {
      code << "    " << getVarName(varIndex) << " = ";
      for (size_t i = 0; i < dependencies.size(); ++i) {
        // Look up the coefficient in the parameters map
        std::string coefficientKey = getCoefficientName(blockno, statementIndex, i);
        int coefficient;
        if (parameters.find(coefficientKey) != parameters.end() && parameters[coefficientKey].has_value()) {
          coefficient = parameters[coefficientKey].value();
        } else {
          // dead code that the model didn't find an interpretation for so we
          // can do whatever. assign a random value to these coefficients
          coefficient = dist(gen);
          parameters[coefficientKey] = coefficient; // Store it in parameters
        }

        // Multiply the dependency by the coefficient
        code << coefficient << " * " << getVarName(dependencies[i]);
        if (i < dependencies.size() - 1) {
          code << " + "; // Example operation
        }
      }

      // Add a constant term
      std::string constantKey = generateConstantName(blockno, statementIndex);
      int constant;
      if (parameters.find(constantKey) != parameters.end() && parameters[constantKey].has_value()) {
        constant = parameters[constantKey].value();
      } else {
        constant = dist(gen);
        parameters[constantKey] = constant;
      }
      code << " + " << constant << ";\n";

      ++statementIndex;
    }
    if (needPassCounter) {
      code << "    if(" << generatePassCounterName() << " >= " << passCounterValue << ")\n";
      code << "    {\n";
      code << "        goto " << generateLabelName(NUM_NODES - 1) << ";\n";
      code << "    }\n";
    }
    if (blockTargets.size() > 1) {
      code << generateConditionalConstraint(blockno, blockTargets[0], conditionalVariables);
      code << generateUnconditionalGoto(blockTargets[1]);
    } else if (blockTargets.size() == 1) {
      code << generateUnconditionalGoto(blockTargets[0]);
    } else {
      // print the values of all the variables, separated by commas
      //  code << "    return 0;";
      code << "    return computeStatelessChecksum(";
      for (int i = 0; i < NUM_VARS; ++i) {
        code << getVarName(i);
        if (i < NUM_VARS - 1) {
          code << ",";
        }
      }
      code << ");\n";
    }
    return code.str();
  }

  void
  generateConstraints(int target, z3::solver &solver, z3::context &c, std::unordered_map<std::string, int> &versions) {
    // check if all subexpressions are free of Undefined Behaviour
    int statementIndex = 0;
    for (const auto &[varIndex, dependencies]: statementMappings) {
      std::vector<z3::expr> terms;
      // Define integer variables and coefficients
      std::vector<z3::expr> vars;
      std::vector<z3::expr> coeffs;
      // UB checks for RHS of the assignment statement. Each subexpression is a
      // multiplication of a coefficient and a variable, and it should be
      // between LOWER_BOUND and UPPER_BOUND
      for (int i = 0; i < dependencies.size(); i++) {
        vars.push_back(c.int_const(
            (getVarName(dependencies[i]) + "_" + std::to_string(versions[getVarName(dependencies[i])])).c_str()
        ));
        solver.add(vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
        pushAfterSearch(coeffs, getCoefficientName(blockno, statementIndex, i), c);
        terms.push_back(coeffs.back() * vars.back()); // a_i * var_i
      }
      solver.add(boundCoefficients(c, coeffs));
      solver.add(makeCoefficientsInteresting(coeffs, c));
      pushAfterSearch(coeffs, generateConstantName(blockno, statementIndex), c);
      terms.push_back(coeffs.back());
      solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
      z3::expr sum = terms[0];
      z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
      if (enableSafetyChecks)
        solver.add(term_constraint);
      for (int i = 1; i < terms.size(); i++) {
        z3::expr constraint =
            (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND); // addition is left associative, so in an addition of
                                                          // k terms, the subexpressions with the first 1, 2,
                                                          // ...k terms added should all be between LOWER_BOUND
                                                          // and UPPER_BOUND as well
        if (enableSafetyChecks)
          solver.add(constraint);
        sum = sum + terms[i];
        term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
        if (enableSafetyChecks)
          solver.add(term_constraint);
      }
      z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
      if (enableSafetyChecks)
        solver.add(constraint);
      versions[getVarName(varIndex)]++;
      // unrolled the entire graph and tracking the versions of each variable
      // because they change adter assignment statements LHS of the assignment
      // statement's version number is changed
      z3::expr assignment =
          c.int_const((getVarName(varIndex) + "_" + std::to_string(versions[getVarName(varIndex)])).c_str());
      constraint = (assignment == sum);
      solver.add(constraint);
      solver.add(makeCoefficientsInteresting(coeffs, c));
      statementIndex++;
    }
    // now, we need to generate the conditional constraint
    logFile << blockTargets.size() << '\n';
    logFile << "Target: " << target << '\n';
    if (blockTargets.size() > 1) {
      // same checks for conditional statements
      std::vector<z3::expr> terms;
      // Define integer variables and coefficients
      std::vector<z3::expr> vars;
      std::vector<z3::expr> coeffs;
      int ctr = 0;
      for (auto i: conditionalVariables) {
        vars.push_back(c.int_const((getVarName(i) + "_" + std::to_string(versions[getVarName(i)])).c_str()));
        solver.add(vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
        pushAfterSearch(coeffs, generateConditionalCoefficientName(blockno, 0, ctr), c);
        terms.push_back(coeffs.back() * vars.back());
        ++ctr;
      }
      solver.add(boundCoefficients(c, coeffs));
      solver.add(makeCoefficientsInteresting(coeffs, c));
      pushAfterSearch(coeffs, generateConditionalConstantName(blockno, blockTargets[0]), c);
      terms.push_back(coeffs.back());
      solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);
      z3::expr sum = terms[0];
      z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
      if (enableSafetyChecks)
        solver.add(term_constraint);
      for (int i = 1; i < terms.size(); i++) {
        z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
        if (enableSafetyChecks)
          solver.add(constraint);
        sum = sum + terms[i];
        term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
        if (enableSafetyChecks)
          solver.add(term_constraint);
      }
      z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
      if (enableSafetyChecks)
        solver.add(constraint);
      if (target == blockTargets[0]) {
        constraint = (sum >= 0);
      } else {
        constraint = (sum < 0);
      }
      solver.add(constraint);
    }
  }
};

// Function to extract parameters from the model, so that we can store the
// coefficients and constants that the solver found an interpretation for
void extractParametersFromModel(z3::model &model, z3::context &ctx) {
  for (const auto &param: parameters) {
    const std::string &name = param.first;

    if (model.has_interp(ctx.int_const(name.c_str()).decl())) {
      z3::expr value = model.get_const_interp(ctx.int_const(name.c_str()).decl());

      // For int values
      if (value.is_numeral()) {
        int int_value;
        if (Z3_get_numeral_int(ctx, value, &int_value)) {
          parameters[name] = int_value;
          logFile << "Parameter " << name << " is in the model with value: " << int_value << '\n';
        }
      }
    } else {
      logFile << "Parameter " << name << " is not explicitly defined in the model\n";
    }
  }
}

// to extract the initial values of each variable that's an input to the
// function from the model
std::vector<int> extractInitialisationsFromModel(z3::model &model, z3::context &ctx) {
  std::vector<int> initialisations;
  for (int i = 0; i < NUM_VARS; i++) {
    std::string varName = getVarName(i) + "_0";
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
std::vector<int>
extractFinalVariableVersions(z3::model &model, z3::context &ctx, std::unordered_map<std::string, int> &versions) {
  std::vector<int> final_variable_versions;
  for (int i = 0; i < NUM_VARS; i++) {
    std::string varName = getVarName(i) + "_" + std::to_string(versions[getVarName(i)]);
    z3::expr varExpr = ctx.int_const(varName.c_str());
    assert(model.has_interp(varExpr.decl()) && "Variable not found in model");
    if (model.has_interp(varExpr.decl())) {
      z3::expr value = model.get_const_interp(varExpr.decl());
      if (value.is_numeral()) {
        int intValue;
        if (Z3_get_numeral_int(ctx, value, &intValue)) {
          final_variable_versions.push_back(intValue);
        } else {
          final_variable_versions.push_back(0); // Default value
        }
      } else {
        final_variable_versions.push_back(0); // Default value
      }
    } else {
      final_variable_versions.push_back(0); // Default value
    }
  }
  return final_variable_versions;
}

void generateCodeForProcedure(
    std::vector<BB> basicBlocks, z3::model &model, z3::context &ctx, int sno, std::string uuid
) {
  // outputFile << "#include <stdio.h>\n\n";
  outputFile << "int function_" << uuid << "_" << std::to_string(sno) << "(";
  for (int i = 0; i < NUM_VARS; ++i) {
    outputFile << "int " << getVarName(i);
    if (i < NUM_VARS - 1) {
      outputFile << ", ";
    }
  }
  outputFile << ")\n";
  outputFile << "{\n";
  outputFile << "    int " << generatePassCounterName() << " = 0;\n";
  // Generate the code for each basic block
  for (const auto &bb: basicBlocks) {
    outputFile << bb.generateCode() << '\n';
  }
  outputFile << "}\n";

  outputFile.close();
}

void appendMappingToFile(std::vector<int> initialisation, std::vector<int> final_variable_versions) {
  for (auto x: initialisation) {
    mappingFile << x << ",";
  }
  mappingFile << " : ";
  for (auto x: final_variable_versions) {
    mappingFile << x << ",";
  }
  mappingFile << '\n';
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

CliOpts parse_opts(int argc, char **argv) {
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
  } catch (cxxopts::exceptions::exception e) {
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
  auto args = parse_opts(argc, argv);

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

  int nodes = NUM_NODES; // Number of nodes
  Graph g(nodes);
  g.generate_graph();
  std::vector<std::set<int>> adjacency_list = g.adj;
  std::vector<BB> basicBlocks;
  for (int i = 0; i < nodes; i++) {
    BB bb(i, adjacency_list[i]);
    basicBlocks.push_back(bb);
  }
  std::vector<int> sample_walk = {};
  if (enableConsistentWalks) {
    sample_walk = g.sample_consistent_walk(0, nodes - 1, 100);
  } else {
    sample_walk = g.sample_walk(0, nodes - 1, 100);
  }
  // g.print_graph();
  if (sample_walk[sample_walk.size() - 1] != NUM_NODES - 1) {
    // this means that the last node is not the end node, probably because we're
    // going in a cycle, so we need to add a pass counter to the last node (as
    // if we're artificially creating an edge to the end node)

    // modify the basic block at the end of the sample walk to have pass counter
    // equal to the number of times that basic block has been visited
    int passCounter = 0;
    for (int i = 0; i < sample_walk.size(); i++) {
      if (sample_walk[i] == sample_walk[sample_walk.size() - 1]) {
        passCounter++;
      }
    }
    basicBlocks[sample_walk[sample_walk.size() - 1]].setPassCounter(passCounter);
    parameters[generatePassCounterName()] = passCounter;
    sample_walk.push_back(NUM_NODES - 1);
    logFile << "Sample walk has been modified to end at the last node." << '\n';
  }

  z3::context c;
  z3::solver solver(c);
  solver.add(makeInitialisationsInteresting(c));
  if (enableRandomInitialisations) {
    solver.add(addRandomInitialisations(c));
  }
  std::unordered_map<std::string, int> versions;
  for (int i = 0; i < sample_walk.size() - 1; i++) {
    int current_bb = sample_walk[i];
    int next_bb = sample_walk[i + 1];
    basicBlocks[current_bb].generateConstraints(
        next_bb, solver, c,
        versions
    ); // generate constraints for the current basic block so that
       // no UB occurs and it can reach the next basic block
  }
  basicBlocks[sample_walk[sample_walk.size() - 1]].generateConstraints(-1, solver, c, versions);
  std::vector<int> initialisation;
  std::vector<int> final_variable_versions;
  logFile << "Solver query: " << solver.to_smt2() << '\n';

  if (solver.check() == z3::unsat) {
    logFile << "UNSAT\n";
    // remove the output file and mapping file from the filesystem because no
    // model was found
    outputFile.close();
    mappingFile.close();
    std::remove((outputFileName).c_str());
    std::remove((mappingFileName).c_str());
    logFile.close();
    return 1;
  } else {
    isFormulaSatisfiable = true;
    logFile << "SAT\n";
    z3::model model = solver.get_model();
    logFile << "Model: " << model << '\n';

    extractParametersFromModel(model, c);
    initialisation = extractInitialisationsFromModel(model, c);
    final_variable_versions = extractFinalVariableVersions(model, c, versions);
    generateCodeForProcedure(basicBlocks, model, c, sno, uuid);
    outputFile.close();
    appendMappingToFile(initialisation, final_variable_versions);
  }
  // now, let's try to generate a couple more mappings (initialisation sets).
  //  First, let's regenerate the SMT query by repopulating the solver with the
  //  same constraints, except this time - the solver would use the coefficients
  //  and constants we already generated from the model
  solver.reset();
  solver.add(makeInitialisationsInteresting(c));
  if (enableRandomInitialisations) {
    solver.add(addRandomInitialisations(c));
  }
  versions.clear();
  for (int i = 0; i < sample_walk.size() - 1; i++) {
    int current_bb = sample_walk[i];
    int next_bb = sample_walk[i + 1];
    basicBlocks[current_bb].generateConstraints(next_bb, solver, c, versions);
  }
  basicBlocks[sample_walk[sample_walk.size() - 1]].generateConstraints(-1, solver, c, versions);

  for (int i = 0; i < NUMBER_OF_INITIALISATIONS_OF_EACH_WALK - 1; i++) {
    solver.add(differentInitialisationConstraint(initialisation, c)); // ensure that the initialisation is sufficiently
                                                                      // different from the previous one
    if (solver.check() == z3::unsat) {
      logFile << "UNSAT\n";
      outputFile.close();
      mappingFile.close();
      logFile.close();
      return 1;
    } else {
      isFormulaSatisfiable = true;
      logFile << "SAT\n";
      z3::model model = solver.get_model();
      extractParametersFromModel(model, c);
      initialisation = extractInitialisationsFromModel(model, c);
      final_variable_versions = extractFinalVariableVersions(model, c, versions);
      appendMappingToFile(initialisation, final_variable_versions);
    }
  }
  outputFile.close();
  mappingFile.close();
  logFile.close();
  return 0;
}
