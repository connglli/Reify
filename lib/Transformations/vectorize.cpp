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

// Contains Transformation Rules that attempt to cause Vectorizations in the compiler

#include "lib/patternmatch.hpp"
#include "lib/Transformations/vectorize.hpp"
#include "lib/Transformations/utils.hpp"
#include "lib/lang.hpp"
#include "lib/logger.hpp"
#include <climits>

using namespace patternmatch;
namespace transformations::vectorize {

  bool DeadAssignFromCopy::match(const symir::Stmt *stmt) const {
    return patternmatch::match(stmt, m_AssStmt(m_WildCard<const symir::VarUse*>(), m_WildCard<const symir::Expr *>()));
  }
  
  std::vector<symir::BlockBuilder::StmtID> DeadAssignFromCopy::rewrite(
      symir::FunctBuilder *funBd,
      symir::BlockBuilder *blockBd,
      const symir::Stmt *stmt
    ) {

    Log::Get().Out() << "Running DeadAssignFromCopy" << std::endl;

    auto copier = symir::StmtCopier(funBd, blockBd);
    symir::BlockBuilder::StmtID origStmt = copier.CopyStmt(stmt);
    symir::BlockBuilder::StmtID deadStmt = blockBd->SymAssStmt(
      this->getNewScaLocal(funBd, blockBd->GetLabel()),
      copier.CopyExpr(static_cast<const symir::AssStmt *>(stmt)->GetExpr())
    );
    return {deadStmt, origStmt};
  }

  bool PartialUnrolling::match(const symir::Stmt *stmt) const {
    return patternmatch::match(
      stmt,
      m_AssStmt(m_WildCard<const symir::VarUse*>(), m_Expr(m_Length<const symir::Term *>(m_Range<size_t, size_t>(2, INT_MAX))))
    );
  }

  std::vector<symir::BlockBuilder::StmtID> PartialUnrolling::rewrite(
    symir::FunctBuilder *funBd,
    symir::BlockBuilder *blockBd,
    const symir::Stmt *stmt
  ) {

    Log::Get().Out() << "Running PartialUnrolling" << std::endl;

    const symir::AssStmt *assStmt = static_cast<const symir::AssStmt *>(stmt);
    const symir::VarUse *var = assStmt->GetVar();
    std::vector<symir::Coef *> access = utils::copyAccess(funBd, var);
    const symir::Expr *expr = assStmt->GetExpr();
    std::vector<const symir::Term *> terms = expr->GetTerms();
    size_t nrTerms = expr->NumTerms();


    symir::StmtCopier c = symir::StmtCopier(funBd, blockBd);

    // create an array that can hold all terms;
    const symir::VarDef *array = this->getNewVecLocal(funBd, blockBd->GetLabel(), { (int) nrTerms });
    Log::Get().Out() << "Creating array " << array->GetName() << " with " << nrTerms << " elements" << std::endl;

    // loop through all terms and create an array that holds all terms
    std::vector<symir::BlockBuilder::StmtID> res;
    res.reserve(2 + nrTerms);
    for (size_t i = 0; i < nrTerms; i++) {
      Log::Get().Out() << "Initalizing element " << i << " of " << array->GetName() << std::endl;
      res.push_back(blockBd->SymAssStmt(
        array,
        blockBd->SymAddExpr({ c.CopyTerm(terms[i]) } ),
        { funBd->SymI32Const(i) }
      ));
    }
    
    // Init original value to 0
    Log::Get().Out() << "Initalizing " << var->GetName() << " to 0" << std::endl;
    auto zero = funBd->SymI32Const(0);
    res.push_back(blockBd->SymAssStmt(
      var->GetDef(),
      blockBd->SymExpr(expr->GetOp(), {
        blockBd->SymCstTerm(zero, nullptr),
      }),
      access
    ));

    // create a for loop that sums over the array
    auto loopVar = this->getNewScaLocal(funBd, blockBd->GetLabel());
    auto one = funBd->SymI32Const(1);
    static size_t uid = 0;
    res.push_back(blockBd->SymForStmt(
      loopVar,
      blockBd->SymCond(symir::Cond::OP_LTZ, 
        blockBd->SymAddExpr({
          blockBd->SymMulTerm(
            one,
            loopVar, {}
          ),
          blockBd->SymCstTerm(
            funBd->SymI32Const(-nrTerms),
            nullptr, {}
          )
        })
      ),
      // int i = 0
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          zero,
          nullptr
        )
      }),
      // i += 1
      blockBd->SymAddExpr({
        blockBd->SymCstTerm(
          one,
          nullptr
        )
      }),
      // v = v + a[i]
      { 
        blockBd->SymAssStmt(
          var->GetDef(),
          blockBd->SymExpr(expr->GetOp(), {
            blockBd->SymMulTerm(one, var->GetDef(), access),
            blockBd->SymMulTerm(one, array, { 
              funBd->SymCoef("__loopVarCoef" + std::to_string(uid++) + loopVar->GetName(), loopVar->GetName()) 
            }) 
          }),
          access
        ) 
      }
    ));
    return res;
  }

} // namespace
