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

#include <fstream>

#include "lib/chksum.hpp"
#include "lib/parsers.hpp"
#include "lib/program.hpp"
#include "lib/random.hpp"

ProgPlus::ProgPlus(std::string uuid, const int sno, const std::vector<std::string> &funPaths) :
    uuid(std::move(uuid)), sno(std::to_string(sno)) {
  // Parse all selected function files
  for (const auto &funPath: funPaths) {
    fs::path sexpPath = GetSexpressionPathForFunctionPath(funPath);
    functions.push_back(FunPlus::ParseFunSexpCode(sexpPath));
    fs::path mapPath = GetMappingPathForFunctionPath(funPath);
    mappings.push_back(FunPlus::ParseMappingCode(mapPath));
    Assert(functions.back() != nullptr, "The function for \"%s\" is nullptr", funPath.c_str());
  }
}

class CoefRepl : protected symir::SymIRVisitor {
public:
  explicit CoefRepl(symir::Funct *const host) : host(host) {
    Assert(host != nullptr, "The host function is given a nullptr");
    for (auto &sym: host->GetSymbols()) {
      // TODO: Check to ensure all symbols are Coef
      Assert(
          sym->IsSolved(),
          "The symbol \"%s\" in the host function is not solved, cannot replace it",
          sym->GetName().c_str()
      );
      symbols[dynamic_cast<symir::Coef *>(sym)] = false;
    }
  }

  [[nodiscard]] int GetNumCoefs() const { return static_cast<int>(symbols.size()); }

  bool ReplaceFirstCoef(
      const symir::Funct *guest, const std::vector<ArgPlus<int>> *init,
      const std::vector<ArgPlus<int>> *fina
  ) {
    this->succeeded = false;
    this->guest = guest;
    this->init = init;
    this->fina = fina;
    this->host->Accept(*this);
    this->guest = nullptr;
    this->init = nullptr;
    this->fina = nullptr;
    return this->succeeded;
  }

protected:
  void Visit(const symir::VarUse &v) override {
    if (succeeded) {
      return;
    }
    if (v.IsVector()) {
      for (const auto *c: v.GetAccess()) {
        c->Accept(*this);
        if (succeeded) {
          return;
        }
      }
    }
  }

  void Visit(const symir::Coef &c) override {
    if (succeeded) {
      return;
    }
    symir::Coef *p = const_cast<symir::Coef *>(&c);
    if (wasMutated(p)) {
      // If the coefficient is already replaced, we do not need to do anything
      return;
    }
    // Replace the coefficient with a call to the function
    Assert(
        p->GetType() == symir::SymIR::I32,
        "Unsupported type %s for the coefficient with name \"%s\"",
        symir::SymIR::GetTypeName(p->GetType()).c_str(), p->GetName().c_str()
    );
    int coefVal = p->GetI32Value();
    int chkVal = StatelessChecksum::Compute(*fina);
    std::ostringstream chkOss;
    chkOss << StatelessChecksum::GetCheckChksumName() << "(" << chkVal << ", " << guest->GetName()
           << "(";
    for (int i = 0; i < static_cast<int>(init->size()) - 1; ++i) {
      chkOss << (*init)[i].ToCxStr() << ", ";
    }
    chkOss << (*init)[init->size() - 1].ToCxStr() << "))";
    // To avoid UBs, we'd use an upper type to save the result: long long here
    long long diff = static_cast<long long>(coefVal) - static_cast<long long>(chkVal);
    if (diff >= static_cast<long long>(INT32_MIN) && diff <= static_cast<long long>(INT32_MAX)) {
      p->SetValue("(" + chkOss.str() + " + " + std::to_string(diff) + ")");
    } else {
      p->SetValue("(int) ((long long)" + chkOss.str() + " + " + std::to_string(diff) + "L)");
    }
    markMutated(p);
    succeeded = true;
  }

  void Visit(const symir::Term &t) override {
    if (succeeded) {
      return;
    }
    t.GetCoef()->Accept(*this);
    if (succeeded) {
      return;
    }
    if (t.GetVar() != nullptr) {
      t.GetVar()->Accept(*this);
    }
  }

  void Visit(const symir::Expr &e) override {
    if (succeeded) {
      return;
    }
    for (const auto *term: e.GetTerms()) {
      if (succeeded) {
        return;
      }
      term->Accept(*this);
    }
  }

  void Visit(const symir::Cond &c) override {
    if (succeeded) {
      return;
    }
    c.GetExpr()->Accept(*this);
  }

  void Visit(const symir::AssStmt &a) override {
    if (succeeded) {
      return;
    }
    a.GetExpr()->Accept(*this);
    if (succeeded) {
      return;
    }
    a.GetVar()->Accept(*this);
  }

  void Visit(const symir::RetStmt &r) override { /* Do Nothing */ }

  void Visit(const symir::Branch &b) override {
    if (succeeded) {
      return;
    }
    b.GetCond()->Accept(*this);
  }

  void Visit(const symir::Goto &g) override { /* Do Nothing */ }

  void Visit(const symir::ScaParam &p) override {
    Panic("Cannot reach here: ScaParam variables should not be replaced");
  }

  void Visit(const symir::VecParam &p) override {
    Panic("Cannot reach here: ArrParam variables should not be replaced");
  }

  void Visit(const symir::ScaLocal &l) override {
    Panic("Cannot reach here: Local variables should not be replaced");
  }

  void Visit(const symir::VecLocal &l) override {
    Panic("Cannot reach here: Local variables should not be replaced");
  }

  void Visit(const symir::Block &b) override {
    Panic("Cannot reach here: We directly manipulate statements when visiting a function");
  }

  void Visit(const symir::Funct &f) override {
    const auto statements = f.GetStmts();
    const auto rand = Random::Get().Uniform(0, static_cast<int>(statements.size()) - 1);
    const auto probability = Random::Get().UniformReal()();
    // Sample a statement from the list of statements for replacement
    for (int tries = 0; tries < 1000; tries++) {
      const int index = rand();
      const auto &stmt = statements[index];
      // Idea: 80% of the replacements should be in the conditional statements,
      // and the rest 20% in those assignment statements.
      double threshold;
      switch (stmt->GetIRId()) {
        case symir::SymIR::SIR_TGT_BRA:
          threshold = 0.8;
          break;
        case symir::SymIR::SIR_STMT_ASS:
          threshold = 0.2;
          break;
        default:
          threshold = 0;
          break;
      }
      if (probability < threshold) {
        stmt->Accept(*this);
        if (succeeded) {
          return;
        }
      }
    }
  }

private:
  bool wasMutated(symir::Coef *c) {
    auto it = symbols.find(c);
    Assert(
        it != symbols.end(),
        "The coefficient with name \"%s\" is not found in the host function's symbols",
        it->first->GetName().c_str()
    );
    return it->second;
  }

  void markMutated(symir::Coef *c) {
    auto it = symbols.find(c);
    Assert(
        it != symbols.end(),
        "The coefficient with name \"%s\" is not found in the host function's symbols",
        it->first->GetName().c_str()
    );
    it->second = true;
  }

private:
  symir::Funct *const host;
  // A map of symbols in the host function, where the key is the symbol pointer
  // and the value is a boolean indicating whether it is replaced or not.
  std::map<symir::Coef *, bool> symbols;

  // The result of the replacement for the guest function
  bool succeeded = false;
  const symir::Funct *guest = nullptr;
  const std::vector<ArgPlus<int>> *init = nullptr;
  const std::vector<ArgPlus<int>> *fina = nullptr;
};

void ProgPlus::Generate() const {
  const int numFuns = static_cast<int>(functions.size());

  // Now replace the mappings in the functions with the calls to the other functions
  for (int i = 0; i < numFuns - 1; ++i) {
    auto host = functions[i].get();
    CoefRepl repl(host);
    int numRepCoeffs = repl.GetNumCoefs();

    Log::Get().Out() << "[" << sno << "] Replacing function" << ": index=" << i
                     << ", name=" << host->GetName() << ", num_replaceable=" << numRepCoeffs
                     << std::endl;

    // Sample a function from i + 1 to the end
    auto rand = Random::Get().Uniform(i + 1, numFuns - 1);
    auto randU = Random::Get().UniformReal();

    // For each coefficient that can be replaced, we find a function to replace it
    for (int k = 0; k < numRepCoeffs; ++k) {
      if (randU() >= GlobalOptions::Get().ReplaceProba) {
        continue; // Skip this coefficient replacement with a probability
      }
      int j = rand();
      auto guest = functions[j].get();
      Assert(guest != nullptr, "The guest function is nullptr for index %d", j);
      auto guestMap = mappings[j];
      int index = Random::Get().Uniform(0, static_cast<int>(guestMap.first.size()) - 1)();
      std::vector<ArgPlus<int>> *init = &guestMap.first[index];
      std::vector<ArgPlus<int>> *fina = &guestMap.second[index];
      if (repl.ReplaceFirstCoef(guest, init, fina)) {
        Log::Get().Out() << "[" << sno << "]   var#" << k << " -> func#" << j << ": "
                         << guest->GetName() << std::endl;
      }
    }

    Log::Get().Out() << "[" << sno << "]   Done" << std::endl;
  }
}

void ProgPlus::GenerateCode(const fs::path &file, bool debug, bool staticModifier) const {
  std::ofstream oss(file);
  oss << StatelessChecksum::GetRawCode() << std::endl;
  oss << "#include <stdio.h>" << std::endl;
  oss << StatelessChecksum::GetCheckChksumCode(debug) << std::endl;
  auto rand = Random::Get().UniformReal();
  for (int i = static_cast<int>(functions.size()) - 1; i >= 0; --i) {
    // Output inline with 60% probability
    auto probability = rand();
    if (staticModifier) {
      oss << "static ";
    }
    if (probability < 0.6) {
      if (!staticModifier) {
        oss << "static inline ";
      } else {
        oss << "inline ";
      }
    }
    symir::SymCxLower lower(oss);
    lower.Lower(*functions[i]);
    oss << std::endl;
  }
  oss << "int main() {" << std::endl;
  int index = Random::Get().Uniform(0, static_cast<int>(mappings[0].first.size()) - 1)();
  const std::vector<ArgPlus<int>> *init = &(mappings[0].first[index]);
  const std::vector<ArgPlus<int>> *fina = &(mappings[0].second[index]);
  int checksum = StatelessChecksum::Compute(*fina);
  oss << "    printf(\"%d\\n\", " << StatelessChecksum::GetCheckChksumName() << "(" << checksum
      << ", " << functions[0]->GetName() << "(";
  for (size_t i = 0; i < init->size(); ++i) {
    oss << (*init)[i].ToCxStr();
    if (i != init->size() - 1) {
      oss << ", ";
    }
  }
  oss << ")));" << std::endl;
  oss << "    return 0;" << std::endl;
  oss << "}" << std::endl;
  oss.close();
}
