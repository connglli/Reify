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

#include <flint/ulong_extras.h>
#include <flint/nmod.h>
#include <flint/nmod_mat.h>
#include <lib/random.hpp>
#include <lib/samputils.hpp>
#include <utility>

#include "global.hpp"
#include "lib/chksum.hpp"
#include "lib/fcallembed.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include "lib/transformations.hpp"
#include "lib/varstate.hpp"

namespace {
  void retargetBlock(symir::FunctBuilder *funBd, const symir::Block *blk, std::map<const std::string, symir::BlockBuilder *> argBlocks) {
    const symir::Target *target = blk->GetTarget();
    if (target == nullptr) return;

    if (target->GetIRId() == symir::SymIR::SIR_TGT_GOTO) {
      const std::string goto_target = static_cast<const symir::Goto *>(target)->GetTarget();
      if (!argBlocks.contains(goto_target)) return;
      const std::string targetBlkLabel = argBlocks[goto_target]->GetLabel();
      symir::BlockBuilder *blkBd = symir::BlockCopier(funBd, blk).CopyAsBuilder();
      blkBd->RemoveTarget();

      Log::Get().Out() << "Setting goto target of block " << blk->GetLabel() 
                       << " from " << goto_target
                       << " to " << targetBlkLabel << std::endl;

      blkBd->SymGoto(targetBlkLabel);
      funBd->ReplaceOrCloseBlock(blkBd);

    } else if (target->GetIRId() == symir::SymIR::SIR_TGT_BRA) {
      const symir::Branch *branch_target = static_cast<const symir::Branch *>(target);
      if (
        !argBlocks.contains(branch_target->GetTrueTarget()) 
        && !argBlocks.contains(branch_target->GetFalseTarget())
      ) return;
      symir::BlockBuilder *blkBd = symir::BlockCopier(funBd, blk).CopyAsBuilder();
      blkBd->RemoveTarget();
      std::string trueLabel = branch_target->GetTrueTarget();
      std::string falseLabel = branch_target->GetFalseTarget();
      if (argBlocks.contains(trueLabel)) {
        trueLabel = argBlocks[trueLabel]->GetLabel();
      }
      if (argBlocks.contains(falseLabel)) {
        falseLabel = argBlocks[falseLabel]->GetLabel();
      }

      Log::Get().Out() << "Setting goto target of block " << blk->GetLabel() 
                       << " from true: " << branch_target->GetTrueTarget() << " false: " << branch_target->GetFalseTarget()
                       << " to true: " << trueLabel << " false: " << falseLabel << std::endl;

      blkBd->SymBranch(trueLabel, falseLabel, symir::StmtCopier(funBd, blkBd).CopyCond(branch_target->GetCond()));
      funBd->ReplaceOrCloseBlock(blkBd);

    } else {
      Panic("Each block must have a target but target IR has Id: %d", target->GetIRId());
    }
  }
} // namespace

// ==================== FCallStrategy Base Implementations ====================
void FCallStrategy::initialize(
  const symir::Funct *guest,
  const std::vector<ArgPlus<int32_t>> *init,
  const std::vector<ArgPlus<int32_t>> *fina
) {
  this->guest = guest;
  this->init = init;
  this->fina = fina;
}

void FCallStrategy::setTarget(const int32_t target) {
  this->emplaceTargetValue = target;
}

void FCallStrategy::setMaxNrBlocks(size_t nrBlocks) {
  Log::Get().Out() << "Set max nrBlocks to " << nrBlocks << std::endl;
  this->argUsedMatrix.clear();
  this->argUsedMatrix.resize(nrBlocks);
  this->nrStmts = 1;
  this->nrBlocks = nrBlocks;
}


std::string FCallStrategy::wrapChecksum(int32_t checksum, std::string call) const {
  std::ostringstream res;
  res << StatelessChecksum::GetCheckChksumName()
      <<"(" 
      << checksum 
      << ", "
      << call
      << ")";
  return res.str();
}

const symir::VarDef *FCallStrategy::getUnusedAssignVar(symir::FunctBuilder *funBd, size_t blockIndex, size_t stmtIndex) {
  Assert(blockIndex < this->nrBlocks, "blockIndex must be less then the set max number of blocks");

  if (stmtIndex + 1 > nrStmts) {
    this->argUsedMatrix.resize(this->nrBlocks * (stmtIndex + 1));
    nrStmts = stmtIndex + 1;
  }

  // How many argument variables are already in use at this stmt
  size_t idx = stmtIndex * this->nrBlocks + blockIndex;
  Assert(
    idx < this->argUsedMatrix.size(),
    "attempted to get a variable count out of bound of matrix\n" 
    "{stmtIndex: %ld, nrBlocks: %ld, blockIndex: %ld, idx: %ld, matrixSize: %ld}",
    stmtIndex, this->nrBlocks, blockIndex, idx, this->argUsedMatrix.size()
  );
  size_t argUsed = this->argUsedMatrix[stmtIndex * this->nrBlocks + blockIndex];

  const symir::VarDef *loc = funBd->FindVar("arg_" + std::to_string(argUsed));
  if (loc == nullptr) {
    loc = funBd->SymScaLocal("arg_" + std::to_string(argUsed), nullptr);
  }

  Assert(loc != nullptr, "creation or search for local has failed");
  Log::Get().Out() << "AssignVariable: " << loc->GetName() << std::endl;;

  this->argUsedMatrix[stmtIndex * this->nrBlocks + blockIndex] += 1;
  return loc;
}

// ==================== FCallEmbedder Base Implementations ====================
FCallEmbedder::FCallEmbedder(symir::Funct *const host): host(host) {
  Assert(
    host != nullptr,
    "The host function passed to the constructur is a nullptr"
  );
  for (auto &sym: host->GetSymbols()) {
    // Check to ensure all symbols are Coef
    Assert(
      sym->IsSolved(),
      "The symbol \"%s\" in the host function is not solved, cannot replace it",
      sym->GetName().c_str()
    );
    this->symbols[dynamic_cast<symir::Coef *>(sym)] = false;
  }
  this->createBuilder();
}

bool FCallEmbedder::embedGuest(
  symir::Funct *guest,
  const std::vector<ArgPlus<int32_t>> *init,
  const std::vector<ArgPlus<int32_t>> *fina
) {
  Assert(this->callGenStrategy != nullptr, "No embedding strategy choosen");
  Assert(guest != nullptr, "No valid guest to embed");
  Assert(init != nullptr, "No valid init to embed");
  Assert(fina != nullptr, "No valid fina to embed");

  this->callGenStrategy->initialize(guest, init, fina);
  this->succeeded = false;
  this->host->Accept(*this);

  return this->succeeded;
}

bool FCallEmbedder::wasMutated(symir::Coef *c) {
  auto it = symbols.find(c);
  Assert(
      it != symbols.end(),
      "The coefficient with name \"%s\" is not found in the host function's symbols",
      it->first->GetName().c_str()
  );
  return it->second;
}
void FCallEmbedder::markMutated(symir::Coef *c) {
  auto it = symbols.find(c);
  Assert(
      it != symbols.end(),
      "The coefficient with name \"%s\" is not found in the host function's symbols",
      it->first->GetName().c_str()
  );
  it->second = true;
}

// ==================== LiteralFCallStrategy Implementations ====================
std::string LiteralFCallStrategy::generateCall() {
  Assert(this->guest, "guest is not initialized");
  Assert(this->init, "init is not initialized");
  Assert(this->fina, "fina is not initialized");

  // Build call
  std::ostringstream fcall;
  fcall << this->guest->GetName() 
        << "(";
  const auto &params = this->guest->GetParams();
  for (int32_t i = 0; i < static_cast<int32_t>(init->size()); ++i) {
    const auto &p = params[i];
    const auto &arg = (*init)[i];
    fcall << arg.GetTypeCastStr(p) 
          << arg.ToCxStr();
    if (i < static_cast<int32_t>(init->size()) - 1) {
      fcall << ", ";
    }
  }
  fcall << ")";

  // Handle checksum
  int32_t checksum = StatelessChecksum::Compute(*this->fina);
  std::string chk_call = this->wrapChecksum(checksum, fcall.str());

  // Correct call result such that it matches the emplacedTargetValue
  // To avoid UBs, we'd use an upper type to save the result: long long here
  long long diff = static_cast<long long>(this->emplaceTargetValue)
                 - static_cast<long long>(checksum);
  if (
      diff >= static_cast<long long>(INT32_MIN) 
      && diff <= static_cast<long long>(INT32_MAX)
    ) {
    return "(" + chk_call + " + " + std::to_string(diff) + ")";
  } else {
    return "(int) ((long long)" + chk_call + " + " + std::to_string(diff) + "L)";
  }
}

// ==================== PrimeInterpFCallStrategy Implementations ====================
void AbstractArgBlockStrategy::generatePreamble(
  std::vector<VariableStateQuery *> varStateQueries,
  symir::FunctBuilder *funBd,
  size_t blockIndex,
  size_t stmtIndex
) {
  Assert(this->guest, "guest is not initialized");
  Assert(this->init, "init is not initialized");

  Log::Get().OpenSection("Generating Preamble");

  if (this->nrBlocks == 0) this->setMaxNrBlocks(funBd->GetBlocks().size());

  const symir::Block *targetBlock= funBd->GetBlocks()[blockIndex];
  const std::string targetLabel = targetBlock->GetLabel();
  Log::Get().Out() << "targeting block: " << targetLabel << "at stmt: " << stmtIndex << std::endl;
  symir::BlockBuilder *headerBlockBd;
  if (!argBlocks.contains(targetLabel)) {
    Log::Get().Out() << "Creating new block " << targetBlock->GetLabel() + "_header" << std::endl;
    this->argBlocks[targetLabel] = funBd->OpenBlock(targetBlock->GetLabel() + "_header");
  } else {
    Log::Get().Out() << "Reusing block " << targetBlock->GetLabel() + "_header" << std::endl;
  }
  headerBlockBd = this->argBlocks[targetBlock->GetLabel()];

  auto randDouble = Random::Get().UniformReal();
  size_t flattIndex = 0;
  for (size_t initIdx = 0; initIdx < this->init->size(); initIdx++) {
    const auto &arg = (*this->init)[initIdx];
    for (size_t argIdx = 0; argIdx < arg.getSize(); argIdx++) {
      flattIndex += 1;
      if (randDouble() <= 1 - GlobalOptions::Get().InitReplaceProba) continue;

      Log::Get().Out() << "Replacing the flattend " << flattIndex << "-th argument" << std::endl;

      const symir::VarDef *loc = this->getUnusedAssignVar(funBd, blockIndex, 0);
      int val = arg.IsScalar() ? arg.GetValue() : arg.GetValue(argIdx);

      Log::Get().Out() << loc->GetName() << " <- " << val << std::endl;
      headerBlockBd->CommitStmt(
        headerBlockBd->SymAssStmt(
          loc, 
          headerBlockBd->SymAddExpr({
            headerBlockBd->SymCstTerm(
              funBd->SymI32Const(val),
              nullptr
            )
          })
        )
      );
      this->argVars[flattIndex - 1] = std::make_pair(loc->GetName(), 0);
      Log::Get().Out() << std::endl;
    }
  }
  Log::Get().CloseSection();
}

std::string AbstractArgBlockStrategy::generateCall() {
  Assert(this->guest, "guest is not initialized");
  Assert(this->init, "init is not initialized");
  Assert(this->fina, "fina is not initialized");

  int32_t checksum = StatelessChecksum::Compute(*this->fina);
  std::ostringstream fcall;
  fcall << this->guest->GetName() 
        << "(";

  const auto &params = this->guest->GetParams();
  size_t flattenedIndex = 0;
  for (int32_t i = 0; i < static_cast<int32_t>(init->size()); ++i) {
    const auto &p = params[i];
    const auto &arg = (*this->init)[i];

    std::map<size_t, std::pair<std::string, int32_t> *> replacers;
    for (size_t argIdx = 0; argIdx < arg.getSize(); argIdx++) {
      if (this->argVars.contains(flattenedIndex)) {
        replacers[argIdx] = &this->argVars[i];
      }
      flattenedIndex += 1;
    }
    fcall << arg.GetTypeCastStr(p) 
          << arg.ToCxStrWithReplaced(replacers);

    if (i < static_cast<int32_t>(this->init->size()) - 1) {
      fcall << ", ";
    }
  }
  this->argVars.clear();
  fcall << ")";
  std::string chk_call = this->wrapChecksum(checksum, fcall.str());
  // To avoid UBs, we'd use an upper type to save the result: long long here
  long long diff = static_cast<long long>(this->emplaceTargetValue)
                 - static_cast<long long>(checksum);
  if (
      diff >= static_cast<long long>(INT32_MIN) 
      && diff <= static_cast<long long>(INT32_MAX)
    ) {
    return "(" + chk_call + " + " + std::to_string(diff) + ")";
  } else {
    return "(int) ((long long)" + chk_call + " + " + std::to_string(diff) + "L)";
  }
}

void PrimeInterpFCallStrategy::finalize(std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd) {
  // needs variable state
  Log::Get().OpenSection("PrimeInterpFCallStrategy::finalize for " + funBd->GetName());

  // delete all header blocks that do not contain any stmt
  for (auto it = this->argBlocks.cbegin(); it != this->argBlocks.cend();) {
    if (it->second->GetNumberOfCommitedStmt() == 0) {
      it = this->argBlocks.erase(it);
    } else {
      ++it;
    }
  }

  // loop through all blocks and if the target has a header change their target to the header.
  for (auto &blk : funBd->GetBlocks()) {
    retargetBlock(funBd, blk, this->argBlocks);
  }

  std::map<std::string, size_t> labelToIdx;
  size_t idx = 0;
  for (auto &blk : funBd->GetBlocks()) {
    labelToIdx[blk->GetLabel()] = idx++;
  }

  Assert(varStateQueries.size() > 0, "must have atleast on Variable State Query");

  for (auto const &[blkLabel, headerBlockBd] : this->argBlocks) {

    // set the target of the header to the block
    const symir::Block* blk = funBd->FindBlock(blkLabel);
    Assert(blk != nullptr, "Unable to find block %s in %s", blkLabel.c_str(), funBd->GetName().c_str());
    Assert(blk->GetLabel() == blkLabel, "function builders internal Map must be broken (%s != %s)", blk->GetLabel().c_str(), blkLabel.c_str());
    headerBlockBd->SymGoto(blkLabel);
    Log::Get().Out() << "Setting target of header block " << headerBlockBd->GetLabel()
                     << " to " << blkLabel << std::endl;
    
    // Get the Variable State or the original function
    struct VariableState totalVariableState;
    totalVariableState.nrVariables = 0;
    for (auto &varStateQuery : varStateQueries) {
      struct VariableState vs = varStateQuery->query(labelToIdx[blkLabel], 0);
      if (totalVariableState.nrVariables == 0) {
        totalVariableState.nrVariables = vs.nrVariables;
        totalVariableState.varMap = vs.varMap;
      }
      Assert(totalVariableState.nrVariables == vs.nrVariables, "Query returns more or less variables then there are in varMap");
      for (size_t i = 0; i < vs.varState.size(); i++) {
        totalVariableState.varState.push_back(vs.varState[i]);
      }
    }

    std::vector<symir::BlockBuilder *> headerBlockBds = { headerBlockBd };
    transformations::primitive::Guard rule = transformations::primitive::Guard();
    this->rewriteEngine.runAsPass(funBd, headerBlockBds, totalVariableState, rule);

    for (auto blockBd : headerBlockBds) {
      funBd->CloseBlockAt(blockBd, blk);
    }
  }

  // avoid causing problems by calling this function twice;
  this->argBlocks.clear();
  Log::Get().CloseSection();
}

void RevOptFCallStrategy::finalize(std::vector<VariableStateQuery *> varStateQueries, symir::FunctBuilder *funBd) {
  // needs variable state
  Log::Get().OpenSection("RevOptFCallStrategy::finalize for " + funBd->GetName());

  // delete all header blocks that do not contain any stmt
  for (auto it = this->argBlocks.cbegin(); it != this->argBlocks.cend();) {
    if (it->second->GetNumberOfCommitedStmt() == 0) {
      it = this->argBlocks.erase(it);
    } else {
      ++it;
    }
  }

  // loop through all blocks and if the target has a header change their target to the header.
  for (auto &blk : funBd->GetBlocks()) {
    retargetBlock(funBd, blk, this->argBlocks);
  }

  std::map<std::string, size_t> labelToIdx;
  size_t idx = 0;
  for (auto &blk : funBd->GetBlocks()) {
    labelToIdx[blk->GetLabel()] = idx++;
  }

  Assert(varStateQueries.size() > 0, "must have atleast on Variable State Query");

  for (auto const &[blkLabel, headerBlockBd] : this->argBlocks) {

    // set the target of the header to the block
    const symir::Block* blk = funBd->FindBlock(blkLabel);
    Assert(blk != nullptr, "Unable to find block %s in %s", blkLabel.c_str(), funBd->GetName().c_str());
    Assert(blk->GetLabel() == blkLabel, "function builders internal Map must be broken (%s != %s)", blk->GetLabel().c_str(), blkLabel.c_str());
    headerBlockBd->SymGoto(blkLabel);
    Log::Get().Out() << "Setting target of header block " << headerBlockBd->GetLabel()
                     << " to " << blkLabel << std::endl;

    // Get the Variable State or the original function
    struct VariableState totalVariableState;
    totalVariableState.nrVariables = 0;
    for (auto &varStateQuery : varStateQueries) {
      struct VariableState vs = varStateQuery->query(labelToIdx[blkLabel], 0);
      if (totalVariableState.nrVariables == 0) {
        totalVariableState.nrVariables = vs.nrVariables;
        totalVariableState.varMap = vs.varMap;
      }
      Assert(totalVariableState.nrVariables == vs.nrVariables, "Query returns more or less variables then there are in varMap");
      for (size_t i = 0; i < vs.varState.size(); i++) {
        totalVariableState.varState.push_back(vs.varState[i]);
      }
    }

    std::vector<symir::BlockBuilder *> headerBlockBds = { headerBlockBd };
    this->rewriteEngine.run(funBd, headerBlockBds, totalVariableState, 100);

    for (auto blockBd : headerBlockBds) {
      funBd->CloseBlockAt(blockBd, blk);
    }
  }

  // avoid causing problems by calling this function twice;
  this->argBlocks.clear();
  Log::Get().CloseSection();
}

// ==================== RandomFCallEmbedder Implementations ====================
void RandomFCallEmbedder::createPathBlockWhitelist() {
  this->blockIndicesWhitelist.clear();
  for (size_t i = 0; i < this->varStateQueries.size(); i++) {
    const auto indices = this->varStateQueries[0]->getPathBlocksIndices();
    this->blockIndicesWhitelist.resize(this->blockIndicesWhitelist.size() + indices.size());
    for (size_t j = 0; j < indices.size(); j++) {
      this->blockIndicesWhitelist.push_back(indices[j]);
    }
  }
}

void RandomFCallEmbedder::Visit(const symir::VarUse &v) {
  if (this->succeeded) {
    return;
  }
  if (v.IsVector()) {
    for (const auto *c: v.GetAccess()) {
      c->Accept(*this);
      if (this->succeeded) {
        return;
      }
    }
  }
}

void RandomFCallEmbedder::Visit(const symir::Coef &c) {
  if (this->succeeded) {
    return;
  }
  auto *pc = const_cast<symir::Coef *>(&c);
  if (wasMutated(pc)) {
    // If the coefficient is already replaced, we do not need to do anything
    return;
  }
  // Replace the coefficient with a call to the function
  Assert(
    pc->GetType() == symir::SymIR::I32,
    "Unsupported type %s for the coefficient with name \"%s\"",
    symir::SymIR::GetTypeName(pc->GetType()).c_str(), pc->GetName().c_str()
  );

  this->callGenStrategy->setTarget(pc->GetI32Value());

  this->callGenStrategy->generatePreamble(this->varStateQueries, this->hostBuilder.get(), this->current_block, this->current_stmt);
  this->hostBuilder->FindSymbol(pc->GetName())->SetValue(this->callGenStrategy->generateCall());
  this->callGenStrategy->generatePostamble(this->varStateQueries, this->hostBuilder.get(), this->current_block, this->current_stmt);

  markMutated(pc);
  this->succeeded = true;
 }

 void RandomFCallEmbedder::Visit(const symir::Term &t) {
  if (this->succeeded) {
    return;
  }
  t.GetCoef()->Accept(*this);
  if (this->succeeded) {
    return;
  }
  if (t.GetVar() != nullptr) {
    t.GetVar()->Accept(*this);
  }
}

void RandomFCallEmbedder::Visit(const symir::ModExpr &e) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::Expr &e) {
  if (this->succeeded) {
    return;
  }
  for (const auto *term: e.GetTerms()) {
    if (this->succeeded) {
      return;
    }
    term->Accept(*this);
  }
}

void RandomFCallEmbedder::Visit(const symir::Cond &c) {
 if (this->succeeded) {
   return;
 }
 c.GetExpr()->Accept(*this);
}
void RandomFCallEmbedder::Visit(const symir::ModAssStmt &a) { Panic("Should not travel through an already linked function"); }
void RandomFCallEmbedder::Visit(const symir::AssStmt &a) {
  if (this->succeeded) {
    return;
  }
  a.GetExpr()->Accept(*this);
  if (this->succeeded) {
    return;
  }
  a.GetVar()->Accept(*this);
}
void RandomFCallEmbedder::Visit(const symir::RetStmt &r) { /* Do Nothing */ }

void RandomFCallEmbedder::Visit(const symir::Branch &b) {
 if (this->succeeded) {
   return;
 }
 b.GetCond()->Accept(*this);
}
void RandomFCallEmbedder::Visit(const symir::Goto &g)        { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::ScaParam &p)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::VecParam &p)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructParam &p) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::ScaLocal &l)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::VecLocal &l)    { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructLocal &l) { /* Do Nothing */ }
void RandomFCallEmbedder::Visit(const symir::StructDef &s)   { /* Do Nothing */ }

void RandomFCallEmbedder::Visit(const symir::Block &b) {
  const auto stmts = b.GetStmts();
  const auto rand = Random::Get().Uniform(0, static_cast<int32_t>(stmts.size()) - 1);
  const auto randDouble = Random::Get().UniformReal();

  for (int32_t tries = 0; tries < 10; tries++) {
    const int32_t index = rand();
    const auto stmt = stmts[index];
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
    if (randDouble() < threshold) {
      this->current_stmt = index;
      stmt->Accept(*this);
      if (this->succeeded) {
        return;
      }
    }
  }
}

void RandomFCallEmbedder::Visit(const symir::Funct &f) {
  const auto blocks = f.GetBlocks();
  // We could remove duplicates from the path but not doing so gives a bias to blocks inside loops which is not totally undesirable.
  const auto rand = Random::Get().Uniform(0, static_cast<int32_t>(this->blockIndicesWhitelist.size()) - 1);
  // Sample a statement from the list of statements for replacement
  for (int32_t tries = 0; tries < 100; tries++) {
    const size_t index = this->blockIndicesWhitelist[rand()];
    Assert(index < blocks.size(), "whitelisted index out of bounds");
    this->current_block = index;
    blocks[index]->Accept(*this);
    if (this->succeeded) {
      return;
    }
  }
}
