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

#ifndef GRAPHFUZZ_PROGRAM_HPP
#define GRAPHFUZZ_PROGRAM_HPP

#include <memory>
#include <vector>

#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/dbgutils.hpp"
#include "lib/random.hpp"
#include "lib/strutils.hpp"

namespace fs = std::filesystem;

////////////////////////////////////////////////////////////
////// IR Definition
////////////////////////////////////////////////////////////

class MyIR {
public:
  template<typename MyIRType>
  static std::unique_ptr<MyIRType> Create(const std::string &code) {
    auto node = std::make_unique<MyIRType>();
    node->Parse(code);
    return node;
  }

public:
  virtual ~MyIR() = default;

  // Generate the C code for this node
  [[nodiscard]] virtual std::string GenerateCode() const = 0;

  // Parse the given code to fill out this node
  virtual void Parse(const std::string &code) = 0;
};

class Expression : public MyIR {
public:
  // Get the number of coefficients within this expression
  [[nodiscard]] virtual int GetNumCoeffs() const = 0;
  // Replace the first replaceable coefficent with the function call, return true for success
  [[nodiscard]] virtual bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) = 0;
};

// An Term is an expression in the form of c * v,
// where c is an coefficient and v a variable or 1.
class Term : public Expression {
public:
  [[nodiscard]] int GetNumCoeffs() const override { return 1; }

  [[nodiscard]] const std::string &GetCoeff() const { return c; }

  [[nodiscard]] const std::string &GetVar() const { return v; }

  [[nodiscard]] bool IsMutated() const { return mutated; }

  void SetCoeff(const std::string &new_coeff) {
    c = new_coeff;
    mutated = true;
  }

  void Parse(const std::string &code) override {
    std::vector<std::string> tokens = SplitStr(code, "*");
    Assert((tokens.size() == 1 || tokens.size() == 2), "Invalid Term expression: %s", code.c_str());
    c = StripStr(tokens[0]);
    v = tokens.size() > 1 ? StripStr(tokens[1]) : "1";
    mutated = false;
  }

  [[nodiscard]] std::string GenerateCode() const override { return "(" + c + ") * " + v; };

  [[nodiscard]] bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    if (IsMutated()) {
      return false;
    }
    // Replace the coefficient with a call to the function
    int coeff_val = std::stoi(GetCoeff());
    int chk_val = StatelessChecksum::Compute(finalization);
    std::ostringstream chk_oss;
    chk_oss << "check_checksum(" << chk_val << ", " << funName << "(";
    for (int i = 0; i < static_cast<int>(initialisation.size()) - 1; ++i) {
      chk_oss << initialisation[i] << ", ";
    }
    chk_oss << initialisation[initialisation.size() - 1] << "))";
    // To avoid UBs, we'd use an upper type to save the result: long long here
    long long diff = static_cast<long long>(coeff_val) - static_cast<long long>(chk_val);
    if (diff >= static_cast<long long>(INT32_MIN) && diff <= static_cast<long long>(INT32_MAX)) {
      SetCoeff(chk_oss.str() + " + " + std::to_string(diff));
    } else {
      SetCoeff("(int) ((long long)" + chk_oss.str() + " + " + std::to_string(diff) + "L)");
    }
    return true;
  }

private:
  std::string c, v;
  bool mutated = false;
};

// A TermSum is the sum of a series of terms: s = t1 + ... + tn.
class TermSum : public Expression {
public:
  [[nodiscard]] int GetNumCoeffs() const override {
    // Each term has exactly 1 coeff
    return static_cast<int>(terms.size());
  }

  [[nodiscard]] std::string GenerateCode() const override {
    std::vector<std::string> tmp(terms.size());
    std::ranges::transform(terms, tmp.begin(), [](const auto &t) { return t->GenerateCode(); });
    return JoinStr(tmp, " + ");
  }

  [[nodiscard]] bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    for (auto &term: terms) {
      if (term->RepFirstCoeff(funName, initialisation, finalization)) {
        return true;
      }
    }
    return false;
  }

  void Parse(const std::string &code) override {
    // We append it with a "+" to facilitate parsing
    std::istringstream iss(code + "+");
    std::string tm;
    while (std::getline(iss, tm, '+')) {
      terms.push_back(Term::Create<Term>(tm));
    }
  }

private:
  std::vector<std::unique_ptr<Term>> terms{};
};

class Statement : public MyIR {
public:
  enum Type { ASSIGN, IFGOTO, WHILE, FLUFF };

public:
  // Return the type of the statement
  [[nodiscard]] virtual Type GetStmtType() const = 0;
  // Return the number of coefficient within this statement
  [[nodiscard]] virtual int GetNumCoeffs() const = 0;
  // Replace the first replaceable coefficient with the function call, return true for success
  [[nodiscard]] virtual bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) = 0;
};

// An AssignStmt is an assign of an expression to a variable: v = expr;.
class AssignStmt : public Statement {
public:
  void Parse(const std::string &code) override {
    auto equal = code.find('=');
    auto semicolon = code.find(';');
    lhsVar = code.substr(0, equal);
    StripStr(lhsVar);
    // TODO: Currently, we only support TermSum
    rhsExpr = TermSum::Create<TermSum>(code.substr(equal + 1, semicolon - equal - 1));
  }

public:
  [[nodiscard]] Type GetStmtType() const override { return Type::ASSIGN; }

  [[nodiscard]] int GetNumCoeffs() const override { return rhsExpr->GetNumCoeffs(); }

  [[nodiscard]] bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    return rhsExpr->RepFirstCoeff(funName, initialisation, finalization);
  }

  [[nodiscard]] std::string GenerateCode() const override {
    return "    " + lhsVar + " = " + rhsExpr->GenerateCode() + ";";
  }

private:
  std::string lhsVar;
  std::unique_ptr<Expression> rhsExpr{};
};

// An IfGotoStmt is an if-cond-goto statement: if (...) goto BB;
class IfGotoStmt : public Statement {
public:
  void Parse(const std::string &code) override {
    const auto ifKeyword = code.find("if");
    const auto openParen = code.find('(', ifKeyword);
    const auto gtThEq = code.find(">=", openParen);
    const auto closeParen = code.find(')', gtThEq);
    const auto semicolon = code.find(';', closeParen);
    // TODO: Currently, we only support TermSum
    condExpr = Create<TermSum>(code.substr(openParen + 1, gtThEq - 1 - openParen - 1));
    gotoStmt = code.substr(closeParen + 1, semicolon - closeParen - 1);
  }

public:
  [[nodiscard]] Type GetStmtType() const override { return Type::IFGOTO; }

  [[nodiscard]] int GetNumCoeffs() const override { return condExpr->GetNumCoeffs(); }

  [[nodiscard]] bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    return condExpr->RepFirstCoeff(funName, initialisation, finalization);
  }

  [[nodiscard]] std::string GenerateCode() const override {
    return "    if (" + condExpr->GenerateCode() + " >= 0) " + gotoStmt + ";";
  }

private:
  std::string gotoStmt;
  std::unique_ptr<Expression> condExpr{};
};

// An WhileStmt is the while-clause of an do-while statement: "} while (...);"
class WhileStmt : public Statement {
public:
  void Parse(const std::string &code) override {
    const auto whileKeyword = code.find("while");
    const auto openParen = code.find('(', whileKeyword);
    const auto gtThEq = code.find(">=", openParen);
    // TODO: Currently, we only support TermSum
    condExpr = Create<TermSum>(code.substr(openParen + 1, gtThEq - 1 - openParen - 1));
  }

public:
  [[nodiscard]] Type GetStmtType() const override { return Type::WHILE; }

  [[nodiscard]] int GetNumCoeffs() const override { return condExpr->GetNumCoeffs(); }

  [[nodiscard]] bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    return condExpr->RepFirstCoeff(funName, initialisation, finalization);
  }

  [[nodiscard]] std::string GenerateCode() const override {
    return "    } while (" + condExpr->GenerateCode() + " >= 0);";
  }

private:
  std::unique_ptr<Expression> condExpr{};
};

// All other statements that are classified FluffStmt.
class FluffStmt : public Statement {
public:
  void Parse(const std::string &code) override { stmt = code; }

public:
  [[nodiscard]] Type GetStmtType() const override { return Type::FLUFF; }

  [[nodiscard]] int GetNumCoeffs() const override { return 0; }

  [[nodiscard]] bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) override {
    return false;
  }

  [[nodiscard]] std::string GenerateCode() const override { return stmt; }

private:
  std::string stmt;
};

// This represents a Function that are generated by fgen
class Function : public MyIR {
public:
  static std::unique_ptr<Function> Of(const fs::path &funPath, const fs::path &mapPath);

  [[nodiscard]] const std::string &GetName() { return name; }

  [[nodiscard]] int GetNumRepCoeffs() const {
    return static_cast<int>(numCoeffs * GlobalOptions::Get().ReplaceProba);
  }

  void ExtractMapping(
      std::vector<int> &foreignInitialisation, std::vector<int> &foreignFinalization
  ) const {
    // sample a random index from initialisations list
    int index = Random::Get().Uniform(0, static_cast<int>(initialisations.size()) - 1)();
    foreignInitialisation = initialisations[index];
    foreignFinalization = finalizations[index];
  }

  void ParseMapping(const std::string &mapping);

  void AddMapping(const std::vector<int> &initialisation, const std::vector<int> &finalization) {
    initialisations.push_back(initialisation);
    finalizations.push_back(finalization);
  }

  [[nodiscard]] bool RepFirstCoeff(
      const std::string &funName, const std::vector<int> &initialisation,
      const std::vector<int> &finalization
  ) const;

  [[nodiscard]] std::string GenerateCode() const override {
    std::ostringstream oss;
    for (const auto &statement: statements) {
      oss << statement->GenerateCode() << std::endl;
    }
    return oss.str();
  }

  void Parse(const std::string &code) override;

private:
  std::string name;
  std::vector<std::unique_ptr<Statement>> statements{};

  int numParams = 0; // The number of function parameters/arguments
  int numCoeffs = 0; // The number of coefficients in this function

  std::vector<std::vector<int>> initialisations{};
  std::vector<std::vector<int>> finalizations{};
};

class ProgGen {
public:
  ProgGen(std::string uuid, const int sno, const std::vector<std::string> &funPaths) :
      uuid(std::move(uuid)), sno(std::to_string(sno)) {
    // Parse all selected function files
    for (const auto &funPath: funPaths) {
      fs::path mapPath = GetMappingPathForFunctionPath(funPath);
      functions.push_back(Function::Of(funPath, mapPath));
    }
  }

  [[nodiscard]] std::string GetName() const { return uuid + "_" + sno; }

  void Generate() const;
  void GenerateCode(const fs::path &file, bool debug = false, bool staticModifier = false) const;

private:
  std::string uuid, sno;
  std::vector<std::unique_ptr<Function>> functions{};
};

#endif // GRAPHFUZZ_PROGRAM_HPP
