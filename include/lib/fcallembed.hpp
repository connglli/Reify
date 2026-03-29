#ifndef REIFY_FCALLEMBED_HPP
#define REIFY_FCALLEMBED_HPP

#include "lib/lang.hpp"
#include "lib/argument.hpp"
#include "lib/symexec.hpp"
#include "lib/varstate.hpp"


class FCallStrategy {
public: 
  void initialize(
    const symir::Funct *guest,
    const std::vector<ArgPlus<int32_t>> *init,
    const std::vector<ArgPlus<int32_t>> *fina
  );
  void setTarget(const int32_t coefVal);

  std::string wrapChecksum(int32_t checksum, std::string call);

  virtual std::string getStrategyName() = 0;
  /// generate the string representing the call
  virtual std::string generateCall() = 0;
  /// generates the nessessary preamble that create the function arguments
  virtual void generatePreamble(VariableState *varStateQuery, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) = 0;
  /// generates the nessessary postamble that map the function call's return value back to the replaced coeff
  virtual void generatePostamble(VariableState *varStateQuery, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) = 0;

protected:
  const symir::Funct *guest;
  const std::vector<ArgPlus<int32_t>> *init;
  const std::vector<ArgPlus<int32_t>> *fina;
  int32_t coefVal;
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

  void setVarStateQuery(VariableState *varStateQuery) { this->varStateQuery = varStateQuery; }

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
  VariableState *varStateQuery;

  bool succeeded = false;
  std::map<symir::Coef *, bool> symbols;
  size_t current_block, current_stmt;
};

class LiteralFCallStrategy : public FCallStrategy {
public:
  explicit LiteralFCallStrategy():
    FCallStrategy() {};
  void generatePreamble(VariableState *varStateQuery, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  void generatePostamble(VariableState *varStateQuery, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  std::string generateCall() override;
  std::string getStrategyName() override {return "Literal Strategy"; }
};


class PrimeInterpFCallStrategy : public FCallStrategy {
public:
  explicit PrimeInterpFCallStrategy():
    FCallStrategy() {};
  void generatePreamble(VariableState *varStateQuery, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  void generatePostamble(VariableState *varStateQuery, symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) override;
  std::string generateCall() override;
  std::string getStrategyName() override {return "PrimeInterpolation Stratgey"; }
private:
  bool InsertedArgVars = false;
  // maps variable index to UnInitVar name and correction value
  std::map<size_t, std::pair<std::string, int32_t>> argVars;
};

class RandomFCallEmbedder : public FCallEmbedder {
public:
  explicit RandomFCallEmbedder(symir::Funct *const host):
    FCallEmbedder(host) {};
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
};

#endif // REIFY_FCALLEMBED_HPP
