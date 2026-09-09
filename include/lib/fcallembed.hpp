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

#include "lib/argument.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/transformations.hpp"
#include "lib/varstate.hpp"

class FCallStrategy {
public:
  virtual ~FCallStrategy() = default;
  void Initialize(
      const symir::Funct *guest, const std::vector<ArgPlus<int32_t>> *init,
      const std::vector<ArgPlus<int32_t>> *fina
  );
  void SetTarget(const int32_t target);

  virtual std::string GetStrategyName() const = 0;
  /// generate the string representing the call
  virtual std::string GenerateCall() = 0;
  /// generates the nessessary preamble that create the function arguments
  virtual void GeneratePreamble(
      std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd,
      size_t blockIndex, size_t stmtIndex
  ) = 0;
  /// generates the nessessary postamble that map the function call's return value back to the
  /// replaced coeff
  virtual void GeneratePostamble(
      std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd,
      size_t blockIndex, size_t stmtIndex
  ) = 0;
  /// Any post processing that needs to be done
  virtual void
  Finalize(std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd) = 0;

protected:
  /// wrap checksum function/macro around the function call string
  std::string WrapChecksum(int32_t checksum, std::string call) const;

  void SetMaxNrBlocks(size_t nrBlocks);

  const symir::VarDef *
  GetUnusedAssignVar(symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex);

protected:
  const symir::Funct *guest = nullptr;
  const std::vector<ArgPlus<int32_t>> *init = nullptr;
  const std::vector<ArgPlus<int32_t>> *fina = nullptr;
  int32_t emplaceTargetValue = 0;

  std::vector<size_t> argUsedMatrix{};
  size_t nrBlocks = 0;
  size_t nrStmts = 0;
};

class FCallEmbedder : protected symir::SymIRVisitor {
public:
  explicit FCallEmbedder(symir::Funct *const host);
  virtual ~FCallEmbedder() = default;

  void SetStrategy(std::unique_ptr<FCallStrategy> callGenStrategy) {
    Assert(
        callGenStrategy != nullptr,
        "The callGenStrategy passed to the setStragegy method is a nullptr"
    );
    this->callGenStrategy = std::move(callGenStrategy);
    Log::Get().Out() << "Embed Strategy: " << this->callGenStrategy->GetStrategyName() << std::endl;
  }

  void SetVarStateQueries(std::vector<VariableStateQuery *> varStateQueries) {
    this->varStateQueries = varStateQueries;
  }

  /// embeds the guest function with a coeff
  bool EmbedGuest(
      symir::Funct *guest, const std::vector<ArgPlus<int32_t>> *init,
      const std::vector<ArgPlus<int32_t>> *fina
  );

  std::unique_ptr<symir::Funct> Finalize() {
    this->callGenStrategy->Finalize(this->varStateQueries, this->hostBuilder.get());
    return hostBuilder->Build();
  }

  void CreateBuilder() { this->hostBuilder = symir::FunctCopier(this->host).CopyAsBuilder(); }

protected:
  bool WasMutated(symir::Coef *c);
  void MarkMutated(symir::Coef *c);

protected:
  symir::Funct *const host;
  std::unique_ptr<symir::FunctBuilder> hostBuilder;
  std::unique_ptr<FCallStrategy> callGenStrategy;
  std::vector<VariableStateQuery *> varStateQueries;

  bool succeeded = false;
  std::map<symir::Coef *, bool> symbols;
  size_t current_block, current_stmt;
};

// Strategy That embedds function calls with Literal Arguments e.g. foo(1, 2, 3);
class LiteralFCallStrategy : public FCallStrategy {
public:
  explicit LiteralFCallStrategy() {};
  void GeneratePreamble(
      std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd,
      size_t blockIndex, size_t stmtIndex
  ) override {};
  void GeneratePostamble(
      std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd,
      size_t blockIndex, size_t stmtIndex
  ) override {};
  std::string GenerateCall() override;
  void
  Finalize(std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd) override {
  };

  std::string GetStrategyName() const override { return "Literal Strategy"; }
};

// Abstract Strategy That embedds function calls with random variable that are assigned in a block
// prior e.g. foo(1, arg_0, 3); How that block is handles is decided by inheritors of this class in
// the finalize method
class AbstractArgBlockStrategy : public FCallStrategy {
  void GeneratePreamble(
      std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd,
      size_t blockIndex, size_t stmtIndex
  ) override;
  void GeneratePostamble(
      std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd,
      size_t blockIndex, size_t stmtIndex
  ) override {};
  std::string GenerateCall() override;

protected:
  // maps variable index to UnInitVar name and correction value
  std::map<const std::string, symir::BlockBuilder *> argBlocks{};
  std::map<size_t, std::pair<std::string, int32_t>> argVars{};
};

class PrimeInterpFCallStrategy : public AbstractArgBlockStrategy {
public:
  explicit PrimeInterpFCallStrategy() : rewriteEngine(RewriteEngine::Empty()) {};

  void
  Finalize(std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd) override;

  std::string GetStrategyName() const override { return "PrimeInterpolation Stratgey"; }

private:
  RewriteEngine rewriteEngine;
};

class RewriteFCallStrategy : public AbstractArgBlockStrategy {
public:
  explicit RewriteFCallStrategy() : rewriteEngine(RewriteEngine::Default()) {}

  void
  Finalize(std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd) override;

  std::string GetStrategyName() const override { return "RewriteFCallStrategy Stratgey"; }

private:
  // maps variable index to UnInitVar name and correction value
  RewriteEngine rewriteEngine;
};

class RandomFCallEmbedder : public FCallEmbedder {
public:
  explicit RandomFCallEmbedder(symir::Funct *const host) : FCallEmbedder(host) {};

  void SetBlockWhitelist(std::vector<size_t> indices) { this->blockIndicesWhitelist = indices; };

  size_t GetNumBlockOnWhitelist(size_t numBlocks) {
    std::vector<bool> blkSeenMap = std::vector(numBlocks, false);
    for (size_t i = 0; i < this->blockIndicesWhitelist.size(); i++) {
      blkSeenMap[this->blockIndicesWhitelist[i]] = true;
    }
    size_t count = 0;
    for (size_t i = 0; i < blkSeenMap.size(); i++)
      if (blkSeenMap[i]) count++;
    return count;
  }

  void CreatePathBlockWhitelist();

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
  void Visit(const symir::ScaLocal &l) override;
  void Visit(const symir::VecLocal &l) override;
  void Visit(const symir::StructLocal &l) override;
  void Visit(const symir::StructDef &s) override;
  void Visit(const symir::Block &b) override;
  void Visit(const symir::Funct &f) override;

private:
  std::vector<size_t> blockIndicesWhitelist{};
};

#endif // REIFY_FCALLEMBED_HPP
