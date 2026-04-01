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

#ifndef REIFY_FCALLEMBED_HPP
#define REIFY_FCALLEMBED_HPP

#include "lib/lang.hpp"
#include "lib/argument.hpp"
#include "lib/varstate.hpp"


class FCallStrategy {
public: 
  void initialize(
    const symir::Funct *guest,
    const std::vector<ArgPlus<int32_t>> *init,
    const std::vector<ArgPlus<int32_t>> *fina
  );
  void setTarget(const int32_t target);

  virtual std::string getStrategyName() const = 0;
  /// generate the string representing the call
  virtual std::string generateCall() = 0;
  /// generates the nessessary preamble that create the function arguments
  virtual void generatePreamble(std::vector<VariableStateQuery> *varStateQueries, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) = 0;
  /// generates the nessessary postamble that map the function call's return value back to the replaced coeff
  virtual void generatePostamble(std::vector<VariableStateQuery> *varStateQueries, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) = 0;

protected:
  void setMaxNrBlocks(size_t nrBlocks);

  /// wrap checksum function/macro around the function call string
  std::string wrapChecksum(int32_t checksum, std::string call) const;

  const symir::VarDef *getUnusedAssignVar(symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex);

  void appendVarState(VariableStateQuery *varStateQuery, size_t blockIndex, size_t stmtIndex);

  void randomlyFilterVarState(symir::FunctBuilder *funBd);

  void smartlyFilterVarState(symir::FunctBuilder *funBd);

protected:
  const symir::Funct *guest;
  const std::vector<ArgPlus<int32_t>> *init;
  const std::vector<ArgPlus<int32_t>> *fina;
  int32_t emplaceTargetValue;

  std::vector<size_t> argUsedMatrix;
  size_t nrBlocks;
  size_t nrStmts;

  std::map<size_t, std::string> varMap;
  std::vector<int32_t> varState;
  size_t nrVariables;
  size_t nrIterations;

  std::vector<int32_t> filteredVarState;
  std::vector<const symir::VarDef *> filteredVars;
  std::vector<std::vector<symir::Coef *>> filteredAccesses;
  size_t filteredNrVariables;
};

class FCallEmbedder : protected symir::SymIRVisitor {
public:
  explicit FCallEmbedder(symir::Funct *const host);

  void setStrategy(FCallStrategy *callGenStrategy) {
    Assert(
      callGenStrategy != nullptr,
      "The callGenStrategy passed to the setStragegy method is a nullptr"
    );
    this->callGenStrategy = callGenStrategy;
  }

  void setVarStateQueries(std::vector<VariableStateQuery> *varStateQueries) { this->varStateQueries = varStateQueries; }

  /// embeds the guest function with a coeff
  bool embedGuest(
    symir::Funct *guest,
    const std::vector<ArgPlus<int32_t>> *init,
    const std::vector<ArgPlus<int32_t>> *fina
  );

  std::unique_ptr<symir::Funct> finalize() { return hostBuilder->Build(); }

  void createBuilder() {
    this->hostBuilder = symir::FunctCopier(this->host).CopyAsBuilder();
  }

protected:
  bool wasMutated(symir::Coef *c);
  void markMutated(symir::Coef *c);

protected:
  symir::Funct *const host;
  std::unique_ptr<symir::FunctBuilder> hostBuilder;
  FCallStrategy *callGenStrategy;
  std::vector<VariableStateQuery> *varStateQueries;

  bool succeeded = false;
  std::map<symir::Coef *, bool> symbols;
  size_t current_block, current_stmt;
};

class LiteralFCallStrategy : public FCallStrategy {
public:
  explicit LiteralFCallStrategy():
    FCallStrategy() {};
  void generatePreamble(std::vector<VariableStateQuery> *varStateQueries, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  void generatePostamble(std::vector<VariableStateQuery> *varStateQueries, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  std::string generateCall() override;
  std::string getStrategyName() const override {return "Literal Strategy"; }
};


class PrimeInterpFCallStrategy : public FCallStrategy {
public:
  explicit PrimeInterpFCallStrategy():
    FCallStrategy() {};
  void generatePreamble(std::vector<VariableStateQuery> *varStateQueries, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  void generatePostamble(std::vector<VariableStateQuery> *varStateQueries, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  std::string generateCall() override;
  std::string getStrategyName() const override {return "PrimeInterpolation Stratgey"; }
private:
  // maps variable index to UnInitVar name and correction value
  std::map<size_t, std::pair<std::string, int32_t>> argVars;
};

class RandomFCallEmbedder : public FCallEmbedder {
public:
  explicit RandomFCallEmbedder(symir::Funct *const host):
    FCallEmbedder(host) {};
  void setBlockWhitelist(std::vector<size_t> indices) { this->blockIndicesWhitelist = indices; };
  void createPathBlockWhitelist();
private:
  void Visit(const symir::VarUse &v) override;
  void Visit(const symir::Coef &c) override;
  void Visit(const symir::Term &t) override;
  void Visit(const symir::ModExpr &e) override;
  void Visit(const symir::Expr &e) override;
  void Visit(const symir::Cond &c) override;
  void Visit(const symir::ModAssStmt &a) override;
  void Visit(const symir::AssStmt &a) override;
  void Visit(const symir::RetStmt &r) override;
  void Visit(const symir::Branch &b) override;
  void Visit(const symir::Goto &g) override;
  void Visit(const symir::ScaParam &p) override;
  void Visit(const symir::VecParam &p) override;
  void Visit(const symir::StructParam &p) override;
  void Visit(const symir::UnInitLocal&l) override;
  void Visit(const symir::ScaLocal &l) override;
  void Visit(const symir::VecLocal &l) override;
  void Visit(const symir::StructLocal &l) override;
  void Visit(const symir::StructDef &s) override;
  void Visit(const symir::Block &b) override;
  void Visit(const symir::Funct &f) override;

private:
  std::vector<size_t> blockIndicesWhitelist;
};

#endif // REIFY_FCALLEMBED_HPP
