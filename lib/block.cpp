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

#include "lib/block.hpp"

#include <iosfwd>
#include <iostream>
#include <random>
#include <sstream>
#include <z3++.h>

#include "global.hpp"
#include "lib/naming.hpp"
#include "lib/utils.hpp"

void BB::Generate() {
  // We define a variable for each statement using other variables
  assignmentOrder = SampleKDistinct(f.NumVars(), NUM_ASSIGNMENTS_PER_BB);
  for (int stmtIndex = 0; stmtIndex < assignmentOrder.size(); stmtIndex++) {
    int varIndex = assignmentOrder[stmtIndex];

    // Sample the variables which will be used in the RHS of the assignment statement
    defUsedVars[varIndex] = SampleKDistinct(f.NumVars(), NUM_VARIABLES_PER_ASSIGNMENT);

    // Check every element present in vector is less than f.NumVars()
    for (auto var: defUsedVars[varIndex]) {
      if (var >= f.NumVars()) {
        throw std::invalid_argument("Error: variable index out of bounds");
      }
    }

    // We assign a coefficient to it for each variable that contributes to the current variable being defined
    for (int i = 0; i < defUsedVars[varIndex].size(); i++) {
      // Statement index is the index of the assignment statement in a
      // basic block, and i is the index of the variable in the RHS of
      // the assignment statement
      std::string coefficientKey = NameCoeff(blkNo, stmtIndex, i);
      f.InitParam(coefficientKey);
    }

    // We also assign a constant to the current variable being defined
    std::string constantKey = NameConst(blkNo, stmtIndex);
    f.InitParam(constantKey);
  }

  // Our conditional is controlled by all variables defined in this basic block.
  // In case we need more variables beyond the number of variables we already defined,
  // let's refer to some variables defined in other basic blocks.
  condUsedVars = assignmentOrder;
  for (int i = 0; i < NUM_VARIABLES_IN_CONDITIONAL - assignmentOrder.size(); i++) {
    int randomVariable = std::rand() % f.NumVars();
    while (std::find(condUsedVars.begin(), condUsedVars.end(), randomVariable) != condUsedVars.end()) {
      randomVariable = std::rand() % f.NumVars();
    }
    condUsedVars.push_back(randomVariable);
  }
  // logFile << "size of conditional variables: " << condUsedVars.size() << std::endl;

  // Define a specific target that our conditional controls
  if (successors.size() > 1) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(successors.begin(), successors.end(), gen);
    // Assign a coefficient to each variable contributing to the conditional
    for (int i = 0; i < NUM_VARIABLES_IN_CONDITIONAL; i++) {
      // 0 is the statement index here (even I'm not sure why i chose
      // this as a parameter but i don't want to break the code so
      // let's just not touch this), and i is the index of the
      // variable in the actual conditional.
      // (... perhaps because it is the first conditional statement)
      std::string coefficientKey = NameCondCoeff(blkNo, 0, i);
      f.InitParam(coefficientKey);
    }
    // target block is a parameter here if we ever
    // wanted to support more that 2 potential targets
    // for the same basic block
    std::string constantKey = NameCondConst(blkNo, successors[0]);
    f.InitParam(constantKey);
  }
}

///////////////////////////////////////////////////////////////////
/////// Constraint Generation
///////////////////////////////////////////////////////////////////

// void BB::pushAfterSearch(std::vector<z3::expr> &coeffs, std::string name, z3::context &ctx) {
//   // If we've already extracted the value from the model, we use that value,
//   //  otherwise it's an uninterpreted constant which the solver needs to figure
//   //  out the value for
//     if (parameters.find(name) != parameters.end() && parameters[name].has_value()) {
//       coeffs.push_back(ctx.int_val(parameters[name].value()));
//     } else if (parameters.find(name) == parameters.end() ||
//                (parameters.find(name) != parameters.end() && parameters[name] == std::nullopt)) {
//       coeffs.push_back(ctx.int_const((name).c_str()));
//       parameters[name] = std::nullopt;
//     } else {
//       assert(false && "This should not happen");
//     }
// }

z3::expr BB::createParameterExpr(std::string name, z3::context &ctx) {
  // If we've already extracted the value from the model, we use that value,
  //  otherwise it's an uninterpreted constant which the solver needs to figure
  //  out the value for
  if (f.ParamDefined(name)) {
    return ctx.int_val(f.GetParamVal(name));
  } else {
    f.InitParam(name);
    return ctx.int_const((name).c_str());
  }
}

z3::expr BB::makeCoefficientsInteresting(const std::vector<z3::expr> &coeffs, z3::context &c) {
  // z3::expr allZero = c.bool_const(("true"));
  // for(auto coeff: coeffs)
  // {
  //     allZero = allZero && (coeff == 0);
  // }
  // return !allZero;
  if (!ENABLE_INTERESTING_COEFFS) {
    return c.bool_val(true);
  }
  return AtMostKZeroes(c, coeffs, coeffs.size() / 2);
}

z3::expr BB::boundCoefficients(z3::context &c, const std::vector<z3::expr> &coeffs) {
  z3::expr allAssignedConstraint = c.bool_val(true);
  for (const auto &coeff: coeffs) {
    allAssignedConstraint = allAssignedConstraint && (coeff >= LOWER_COEFF_BOUND && coeff <= UPPER_COEFF_BOUND);
  }
  return allAssignedConstraint;
}

void BB::GenerateConstraints(
    int target, z3::solver &solver, z3::context &c, std::unordered_map<std::string, int> &versions
) {
  // TODO: Remove redundant code

  // Ensure all terms in each variable definition are free of Undefined Behaviour
  int stmtIndex = 0;
  for (const auto &[varIndex, dependencies]: defUsedVars) {
    std::string varName = NameVar(varIndex);

    // Define integer variables and coefficients
    std::vector<z3::expr> vars;   // [var_i]
    std::vector<z3::expr> coeffs; // [a_i] or [b_i]
    std::vector<z3::expr> terms;  // [a_i * var_i] or [b_i] for the final addition

    // UB checks for RHS of the assignment statement. Each term is either a
    // multiplication of a coefficient and a variable, or a constant.
    // All should be between LOWER_BOUND and UPPER_BOUND

    // Taking care of coefficients and the multiplication
    for (int i = 0; i < dependencies.size(); i++) {
      std::string depVarName = NameVar(dependencies[i]);
      std::string depVarVer = std::to_string(versions[depVarName]);
      vars.push_back(c.int_const((depVarName + "_" + depVarVer).c_str()));
      solver.add(vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
      coeffs.push_back(createParameterExpr(NameCoeff(blkNo, stmtIndex, i), c));
      terms.push_back(coeffs.back() * vars.back()); // a_i * var_i
    }
    solver.add(boundCoefficients(c, coeffs));
    solver.add(makeCoefficientsInteresting(coeffs, c));

    // Taking care of the constant
    coeffs.push_back(createParameterExpr(NameConst(blkNo, stmtIndex), c));
    terms.push_back(coeffs.back());
    solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);

    // Taking care of the terms and their addition
    z3::expr sum = terms[0];
    z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
    if (ENABLE_SAFETY_CHECKS) {
      solver.add(term_constraint);
    }
    for (int i = 1; i < terms.size(); i++) {
      // Addition is left associative, so in an addition of k terms, the subexpressions with the first 1, 2,
      // ...k terms added should all be between LOWER_BOUND and UPPER_BOUND as well
      z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
      if (ENABLE_SAFETY_CHECKS) {
        solver.add(constraint);
      }
      sum = sum + terms[i];
      term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
      if (ENABLE_SAFETY_CHECKS) {
        solver.add(term_constraint);
      }
    }
    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
    if (ENABLE_SAFETY_CHECKS) {
      solver.add(constraint);
    }

    versions[varName]++;
    std::string varVerNext = std::to_string(++versions[varName]);

    // Unrolled the entire graph and tracking the versions of each variable
    // because they change adter assignment statements LHS of the assignment
    // statement's version number is changed
    z3::expr assignment = c.int_const((varName + "_" + varVerNext).c_str());
    solver.add(assignment == sum);
    // TODO: Redundant? We've already make coeffs interesting,
    //       except for the constant. The prior one can be safely removed?
    solver.add(makeCoefficientsInteresting(coeffs, c));

    stmtIndex++;
  }

  // logFile << successors.size() << std::endl;
  // logFile << "Target: " << target << std::endl;
  if (successors.size() <= 1) {
    return;
  }

  // Now, we need to generate the conditional constraint
  // Define integer variables and coefficients
  std::vector<z3::expr> vars;
  std::vector<z3::expr> coeffs;
  std::vector<z3::expr> terms;

  // Coefficients and the multiplication
  for (int i = 0; i < condUsedVars.size(); i++) {
    std::string depVarName = NameVar(condUsedVars[i]);
    std::string depVarVer = std::to_string(versions[depVarName]);
    vars.push_back(c.int_const((depVarName + "_" + depVarVer).c_str()));
    solver.add(vars.back() >= LOWER_BOUND && vars.back() <= UPPER_BOUND);
    coeffs.push_back(createParameterExpr(NameCondCoeff(blkNo, 0, i), c));
    terms.push_back(coeffs.back() * vars.back());
  }
  solver.add(boundCoefficients(c, coeffs));
  solver.add(makeCoefficientsInteresting(coeffs, c));

  // The constant
  coeffs.push_back(createParameterExpr(NameCondConst(blkNo, successors[0]), c));
  terms.push_back(coeffs.back());
  solver.add(coeffs.back() >= LOWER_BOUND && coeffs.back() <= UPPER_BOUND);

  // Terms and their addition
  z3::expr sum = terms[0];
  z3::expr term_constraint = (terms[0] <= UPPER_BOUND) && (terms[0] >= LOWER_BOUND);
  if (ENABLE_SAFETY_CHECKS) {
    solver.add(term_constraint);
  }
  for (int i = 1; i < terms.size(); i++) {
    z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
    if (ENABLE_SAFETY_CHECKS) {
      solver.add(constraint);
    }
    sum = sum + terms[i];
    term_constraint = (terms[i] <= UPPER_BOUND) && (terms[i] >= LOWER_BOUND);
    if (ENABLE_SAFETY_CHECKS) {
      solver.add(term_constraint);
    }
  }
  z3::expr constraint = (sum <= UPPER_BOUND) && (sum >= LOWER_BOUND);
  if (ENABLE_SAFETY_CHECKS) {
    solver.add(constraint);
  }

  // The jump: We always jump to the first successor for sum >= 0
  if (target == successors[0]) {
    solver.add(sum >= 0);
  } else {
    solver.add(sum < 0);
  }
}

///////////////////////////////////////////////////////////////////
/////// Code Generation
///////////////////////////////////////////////////////////////////


std::string BB::generateConditionalConstraint(int blkNo, int target, std::vector<int> condUsedVars) const {
  std::ostringstream constraint;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
  constraint << "    if (";
  int ctr = 0;
  for (auto i: condUsedVars) {
    std::string coefficientKey = NameCondCoeff(blkNo, 0, ctr);
    int coefficient;
    if (f.ParamDefined(coefficientKey)) {
      coefficient = f.GetParamVal(coefficientKey);
    } else {
      coefficient = dist(gen);
      f.DefineParam(coefficientKey, coefficient); // Store it in parameters
    }
    constraint << coefficient << " * " << NameVar(i);
    constraint << " + ";
    ++ctr;
  }
  std::string constantKey = NameCondConst(blkNo, target);
  int constant;
  if (f.ParamDefined(constantKey)) {
    constant = f.GetParamVal(constantKey);
  } else {
    constant = dist(gen);
    f.DefineParam(constantKey, constant);
  }
  constraint << constant;
  constraint << " >= 0) goto " << NameLabel(target) << ";" << std::endl;

  // Output the constraint
  return constraint.str();
}

std::string BB::generateUnconditionalGoto(int target) const {
  std::ostringstream code;
  code << "    goto " << NameLabel(target) << ";" << std::endl;
  return code.str();
}

std::string BB::GenerateCode() const {
  std::ostringstream code;

  // Generate the label for the basic block
  code << NameLabel(blkNo) << ":" << std::endl;
  // code << "printf(\" at basic block"  << NameLabel(blkNo) <<
  // "\\n\");\n";
  if (needPassCounter) {
    code << NamePassCounter() << "++;" << std::endl;
  }
  // code<< "    printf(\"starting off at
  // "<<NameLabel(blkNo)<<"\\n\");\n";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(LOWER_BOUND, UPPER_BOUND);
  int stmtIndex = 0;
  for (const auto &[varIndex, dependencies]: defUsedVars) {
    code << "    " << NameVar(varIndex) << " = ";
    for (size_t i = 0; i < dependencies.size(); ++i) {
      // Look up the coefficient in the parameters map
      std::string coefficientKey = NameCoeff(blkNo, stmtIndex, i);
      int coefficient;
      if (f.ParamDefined(coefficientKey)) {
        coefficient = f.GetParamVal(coefficientKey);
      } else {
        // Dead code that the model didn't find an interpretation for so we can do whatever,
        // assign a random value to these coefficients
        // TODO: Perhaps adding some checks to ensure that, if we are in the sampled execution,
        //       all our parameters should already be computed.
        coefficient = dist(gen);
        f.DefineParam(coefficientKey, coefficient); // Store it in parameters
      }
      // Multiply the dependency by the coefficient
      code << coefficient << " * " << NameVar(dependencies[i]);
      if (i < dependencies.size() - 1) {
        code << " + "; // Example operation
      }
    }
    // Add a constant term
    std::string constantKey = NameConst(blkNo, stmtIndex);
    int constant;
    if (f.ParamDefined(constantKey)) {
      constant = f.GetParamVal(constantKey);
    } else {
      constant = dist(gen);
      f.DefineParam(constantKey, constant); // Store it in parameters
    }
    code << " + " << constant << ";" << std::endl;
    ++stmtIndex;
  }
  if (needPassCounter) {
    code << "    if(" << NamePassCounter() << " >= " << passCounterValue << ")" << std::endl;
    code << "    {" << std::endl;
    code << "        goto " << NameLabel(f.GetNumBBs() - 1) << ";" << std::endl;
    code << "    }" << std::endl;
  }
  if (successors.size() > 1) {
    code << generateConditionalConstraint(blkNo, successors[0], condUsedVars);
    code << generateUnconditionalGoto(successors[1]);
  } else if (successors.size() == 1) {
    code << generateUnconditionalGoto(successors[0]);
  } else {
    // print the values of all the variables, separated by commas
    //  code << "    return 0;";
    code << "    return computeStatelessChecksum(";
    for (int i = 0; i < f.NumVars(); ++i) {
      code << NameVar(i);
      if (i < f.NumVars() - 1) {
        code << ",";
      }
    }
    code << ");" << std::endl;
  }
  return code.str();
}
