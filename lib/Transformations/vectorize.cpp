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
#include "lib/lang.hpp"

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
    auto copier = symir::StmtCopier(funBd, blockBd);
    symir::BlockBuilder::StmtID origStmt = copier.CopyStmt(stmt);
    symir::BlockBuilder::StmtID deadStmt = blockBd->SymAssStmt(
      this->getNewScaLocal(funBd, blockBd->GetLabel()),
      copier.CopyExpr(static_cast<const symir::AssStmt *>(stmt)->GetExpr())
    );
    return {deadStmt, origStmt};
  }

} // namespace
