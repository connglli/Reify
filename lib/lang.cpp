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

#include "lib/lang.hpp"
#include <algorithm>
#include <iostream>
#include <ranges>

namespace symir {
  VarUse::VarUse(const VarDef *var, std::vector<Coef *> access) :
      SymIR(SIR_VAR_USE), var(var), access(std::move(access)) {
    Assert(this->var != nullptr, "No var to use: a nullptr is given");
    if (this->var->IsVector()) {
      Assert(
          this->var->GetVecNumDims() <= static_cast<int>(this->access.size()),
          "The number of access indices (%lu) "
          "should be at least the number of vector dimensions (%d) for the variable %s",
          this->access.size(), this->var->GetVecNumDims(), var->GetName().c_str()
      );
    } else if (this->var->GetType() == SymIR::Type::STRUCT) {
      Assert(
          this->access.size() >= 1,
          "The number of access indices (%lu) should be at least 1 for the struct variable %s",
          this->access.size(), var->GetName().c_str()
      );
    } else if (this->var->GetType() == SymIR::Type::ARRAY) {
      // Should be covered by IsVector(), but just in case
    } else {
      Panic("The variable %s is not a vector or a struct", var->GetName().c_str());
    }

    for (const auto *c: this->access) {
      Assert(c != nullptr, "The access coefficient cannot be a nullptr");
      Assert(
          c->GetType() == Type::I32, "The access coefficient should be an %s, but got %s",
          SymIR::GetTypeCName(Type::I32).c_str(), SymIR::GetTypeName(c->GetType()).c_str()
      );
    }

    // Resolve Type
    SymIR::Type currType = var->GetType();
    std::string currStruct = (currType == SymIR::Type::STRUCT) ? var->GetStructName() : "";
    int remainingDims = var->IsVector() ? var->GetVecNumDims() : 0;
    int accessIdx = 0;
    int totalAccess = static_cast<int>(this->access.size());

    if (remainingDims > 0) {
      int consumed = std::min(remainingDims, totalAccess);
      remainingDims -= consumed;
      accessIdx += consumed;
      if (remainingDims == 0) {
        currType = var->GetBaseType();
        if (currType == SymIR::Type::STRUCT) {
          currStruct = var->GetStructName();
        }
      }
    }

    while (accessIdx < totalAccess) {
      if (currType == SymIR::Type::STRUCT) {
        Assert(!currStruct.empty(), "Current type is struct but name is empty");
        const auto *c = this->access[accessIdx];
        Assert(c->IsSolved(), "Accessing struct field with non-constant coefficient");
        // If we can't look up struct definition easily here, we just assume checking is done
        // elsewhere. But for type resolution, we need it. Given limitations, we assume this
        // constructor is used for scalar element access (as in SymReturn). If we can't resolve, we
        // default to I32? Or maybe we leave it as STRUCT if we can't look inside? Since VarUse
        // doesn't have FunctBuilder, we are stuck. We will assume I32 if we are drilling into a
        // struct.
        currType = SymIR::Type::I32;
        break;
      } else {
        break;
      }
    }
    setType(currType);
  }

  VarUse::VarUse(const VarDef *var, std::vector<Coef *> access, SymIR::Type type) :
      SymIR(SIR_VAR_USE), var(var), access(std::move(access)) {
    Assert(this->var != nullptr, "No var to use: a nullptr is given");
    if (this->var->IsVector()) {
      Assert(
          this->var->GetVecNumDims() <= static_cast<int>(this->access.size()),
          "The number of access indices (%lu) "
          "should be at least the number of vector dimensions (%d) for the variable %s",
          this->access.size(), this->var->GetVecNumDims(), var->GetName().c_str()
      );
    } else if (this->var->GetType() == SymIR::STRUCT) {
      Assert(
          this->access.size() >= 1,
          "The number of access indices (%lu) should be at least 1 for the struct variable %s",
          this->access.size(), var->GetName().c_str()
      );
    } else {
      Panic("The variable %s is not a vector or a struct", var->GetName().c_str());
    }

    for (const auto *c: this->access) {
      Assert(c != nullptr, "The access coefficient cannot be a nullptr");
      Assert(
          c->GetType() == Type::I32, "The access coefficient should be an %s, but got %s",
          SymIR::GetTypeCName(Type::I32).c_str(), SymIR::GetTypeName(c->GetType()).c_str()
      );
    }
    setType(type);
  }

  SymIRBuilder::TermID BlockBuilder::SymTerm(
      Term::Op op, Coef *coef, const VarDef *var, const std::vector<Coef *> &access
  ) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    TermID tid = numCreatedTerms++;
    if (op == Term::Op::OP_CST) {
      createdTerms[tid] = std::make_unique<Term>(op, coef, nullptr);
    } else {
      SymIR::Type currType = var->GetType();
      SymIR::Type currBaseType = var->GetBaseType();
      std::string currStruct =
          (currType == SymIR::Type::STRUCT)
              ? var->GetStructName()
              : (currBaseType == SymIR::Type::STRUCT ? var->GetStructName() : "");
      int remainingDims = var->IsVector() ? var->GetVecNumDims() : 0;

      for (size_t i = 0; i < access.size(); i++) {
        const auto *c = access[i];

        if (remainingDims > 0) {
          remainingDims--;
          if (remainingDims == 0) {
            currType = currBaseType;
          }
        } else if (currType == SymIR::Type::STRUCT) {
          int idx = c->GetI32Value();
          const auto *sDef = GetParent()->FindStruct(currStruct);
          Assert(sDef, "Struct %s not found", currStruct.c_str());
          const auto &field = sDef->GetField(idx);
          currType = field.type;
          currBaseType = field.baseType;
          if (currType == SymIR::Type::STRUCT) {
            currStruct = field.structName;
          } else if (currType == SymIR::Type::ARRAY) {
            remainingDims = field.shape.size();
            if (currBaseType == SymIR::Type::STRUCT) {
              currStruct = field.structName;
            }
          }
        }
      }

      // If we have ARRAY type with all dimensions consumed, resolve to base type
      if (currType == SymIR::Type::ARRAY && remainingDims == 0) {
        currType = currBaseType;
      }

      // Terms should NEVER have STRUCT type - they must be scalars for arithmetic operations
      Assert(
          currType != SymIR::Type::STRUCT && currType != SymIR::Type::ARRAY,
          "Term cannot be created with STRUCT or ARRAY type. Variable %s with access path of "
          "length %lu resulted in type %s. This indicates generateVarAccess() did not drill down "
          "to a scalar type.",
          var->GetName().c_str(), access.size(), SymIR::GetTypeSName(currType).c_str()
      );


      if (var->IsVector() || var->GetType() == SymIR::Type::STRUCT ||
          var->GetType() == SymIR::Type::ARRAY) {
        createdTerms[tid] =
            std::make_unique<Term>(op, coef, std::make_unique<VarUse>(var, access, currType));
      } else {
        createdTerms[tid] = std::make_unique<Term>(op, coef, std::make_unique<VarUse>(var));
      }
    }
    return tid;
  }

  SymIRBuilder::ExprID BlockBuilder::SymExpr(Expr::Op op, const std::vector<TermID> &termIds) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    ExprID eid = numCreatedExprs++;
    std::vector<std::unique_ptr<Term>> terms_;
    for (const auto tid: termIds) {
      auto it = createdTerms.find(tid);
      Assert(it != createdTerms.end(), "Term with ID \"%lu\" does not exist", tid);
      terms_.push_back(std::move(it->second));
      createdTerms.erase(it);
    }
    createdExprs[eid] = std::make_unique<Expr>(op, std::move(terms_));
    return eid;
  }

  SymIRBuilder::ExprID BlockBuilder::SymCond(Cond::Op op, ExprID eid) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    CondID cid = numCreatedConds++;
    auto it = createdExprs.find(eid);
    Assert(it != createdExprs.end(), "Expr with ID \"%lu\" does not exist", eid);
    createdConds[cid] = std::make_unique<Cond>(op, std::move(it->second));
    createdExprs.erase(it);
    return cid;
  }

  const AssStmt *
  BlockBuilder::SymAssign(const VarDef *var, ExprID eid, const std::vector<Coef *> &access) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    auto it = createdExprs.find(eid);
    Assert(it != createdExprs.end(), "Expr with ID \"%lu\" does not exist", eid);

    SymIR::Type currType = var->GetType();
    SymIR::Type currBaseType = var->GetBaseType();
    std::string currStruct =
        (currType == SymIR::Type::STRUCT)
            ? var->GetStructName()
            : (currBaseType == SymIR::Type::STRUCT ? var->GetStructName() : "");
    int remainingDims = var->IsVector() ? var->GetVecNumDims() : 0;

    for (const auto *c: access) {
      if (remainingDims > 0) {
        remainingDims--;
        if (remainingDims == 0) {
          currType = currBaseType;
        }
      } else if (currType == SymIR::Type::STRUCT) {
        int idx = c->GetI32Value();
        const auto *sDef = GetParent()->FindStruct(currStruct);
        Assert(sDef, "Struct %s not found", currStruct.c_str());
        const auto &field = sDef->GetField(idx);
        currType = field.type;
        currBaseType = field.baseType;
        if (currType == SymIR::Type::STRUCT) {
          currStruct = field.structName;
        } else if (currType == SymIR::Type::ARRAY) {
          remainingDims = field.shape.size();
          if (currBaseType == SymIR::Type::STRUCT) {
            currStruct = field.structName;
          }
        }
      }
    }

    // After processing all accesses, if we still have an ARRAY type and no remaining dims,
    // that means we've consumed all array dimensions and should use the base type
    if (currType == SymIR::Type::ARRAY && remainingDims == 0) {
      currType = currBaseType;
    }

    if (var->IsVector() || var->GetType() == SymIR::Type::STRUCT ||
        var->GetType() == SymIR::Type::ARRAY) {
      stmts.push_back(std::make_unique<AssStmt>(
          std::make_unique<VarUse>(var, access, currType), std::move(it->second)
      ));
    } else {
      stmts.push_back(
          std::make_unique<AssStmt>(std::make_unique<VarUse>(var), std::move(it->second))
      );
    }

    createdExprs.erase(it);
    return dynamic_cast<const AssStmt *>(stmts.back().get());
  }

  std::vector<std::vector<int>> BlockBuilder::GatherVecAccesses(const VarDef *var) const {
    Assert(
        var->IsVector() || var->GetType() == SymIR::Type::ARRAY, "The variable %s is not a vector",
        var->GetName().c_str()
    );

    std::vector<std::vector<int>> accesses(var->GetVecNumEls());
    const std::function<void(int, int)> gather = [&](int dim, int offset) {
      if (dim == var->GetVecNumDims()) {
        return;
      }
      int dimLen = var->GetVecDimLen(dim);
      int dimSize = var->GetVecNumEls(dim);
      for (int i = 0; i < dimLen; i++) {
        const auto startEl = offset + i * dimSize;
        const auto endEl = offset + (i + 1) * dimSize;
        for (auto loc = startEl; loc < endEl; loc += 1) {
          accesses[loc].push_back(i);
        }
        gather(dim + 1, startEl);
      }
    };
    gather(0, 0);

    return accesses;
  }

  const RetStmt *BlockBuilder::SymReturn() {
    Assert(isActive(), "The BlockBuilder is no longer active");
    std::vector<std::unique_ptr<VarUse>> uses;

    std::function<void(
        const VarDef *, std::vector<Coef *>, SymIR::Type, std::string, const std::vector<int> &,
        size_t
    )>
        expand = [&](const VarDef *var, std::vector<Coef *> access, SymIR::Type type,
                     std::string sName, const std::vector<int> &shape, size_t shapeIdx) {
          if (shapeIdx < shape.size()) {
            // Expand Array Dimension
            int dimLen = shape[shapeIdx];
            for (int i = 0; i < dimLen; ++i) {
              auto nextAcc = access;
              nextAcc.push_back(GetParent()->SymI32Const(i));
              expand(var, nextAcc, type, sName, shape, shapeIdx + 1);
            }
          } else if (type == SymIR::Type::STRUCT) {
            // Expand Struct Fields
            const auto *sDef = GetParent()->FindStruct(sName);
            Assert(sDef, "Struct %s not found", sName.c_str());
            for (int i = 0; i < (int) sDef->GetFields().size(); ++i) {
              const auto &field = sDef->GetField(i);
              auto nextAcc = access;
              nextAcc.push_back(GetParent()->SymI32Const(i));
              expand(var, nextAcc, field.baseType, field.structName, field.shape, 0);
            }
          } else {
            // Scalar leaf
            if (access.empty()) {
              uses.push_back(std::make_unique<VarUse>(var));
            } else {
              uses.push_back(std::make_unique<VarUse>(var, access, type));
            }
          }
        };

    for (const auto *p: GetParent()->GetParams()) {
      expand(
          p, {}, p->GetBaseType(),
          (p->GetBaseType() == SymIR::Type::STRUCT ? p->GetStructName() : ""), p->GetVecShape(), 0
      );
    }
    stmts.push_back(std::make_unique<RetStmt>(std::move(uses)));
    return dynamic_cast<const RetStmt *>(stmts.back().get());
  }

  const Branch *
  BlockBuilder::SymBranch(const std::string &truLab, const std::string &falLab, CondID cid) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    auto it = createdConds.find(cid);
    Assert(it != createdConds.end(), "Cond with ID \"%lu\" does not exist", cid);
    target = std::make_unique<Branch>(std::move(it->second), truLab, falLab);
    createdConds.erase(it);
    return dynamic_cast<const Branch *>(target.get());
  }

  const Goto *BlockBuilder::SymGoto(const std::string &label) {
    Assert(isActive(), "The BlockBuilder is no longer active");
    target = std::make_unique<Goto>(label);
    return dynamic_cast<const Goto *>(target.get());
  }

  std::unique_ptr<Block> BlockBuilder::Build() {
    Assert(SymIRBuilderGeneric::isActive(), "The BlockBuilder is no longer active");
    deactivate();
    return std::make_unique<Block>(label, std::move(stmts), std::move(target));
  }

  const StructDef *
  FunctBuilder::SymStruct(const std::string &name, const std::vector<StructDef::Field> &fields) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !structMap.contains(name), "Struct with the same name \"%s\" is already defined",
        name.c_str()
    );
    structs.push_back(std::make_unique<StructDef>(name, fields));
    const auto s = structs.back().get();
    structMap[name] = s;
    return s;
  }

  Coef *FunctBuilder::SymCoef(const std::string &name, SymIR::Type type) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !symMap.contains(name), "Coefficients with the same name \"%s\" is already defined",
        name.c_str()
    );
    symbols.push_back(std::make_unique<Coef>(name, type));
    SymDef *s = symbols.back().get();
    symMap[name] = s;
    return dynamic_cast<Coef *>(s);
  }

  Coef *FunctBuilder::SymCoef(const std::string &name, const std::string &value, SymIR::Type type) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !symMap.contains(name), "Coefficients with the same name \"%s\" is already defined",
        name.c_str()
    );
    symbols.push_back(std::make_unique<Coef>(name, value, type));
    SymDef *s = symbols.back().get();
    symMap[name] = s;
    return dynamic_cast<Coef *>(s);
  }

  const ScaParam *
  FunctBuilder::SymScaParam(const std::string &name, SymIR::Type type, bool isVolatile) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    params.push_back(std::make_unique<ScaParam>(name, type));
    const auto v = params.back().get();
    if (isVolatile) {
      v->SetVolatile();
    }
    paramMap[name] = v;
    return dynamic_cast<ScaParam *>(v);
  }

  const VecParam *FunctBuilder::SymVecParam(
      const std::string &name, const std::vector<int> &shape, SymIR::Type type,
      std::string structName, bool isVolatile
  ) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    params.push_back(std::make_unique<VecParam>(name, shape, type, structName));
    const auto v = params.back().get();
    if (isVolatile) {
      v->SetVolatile();
    }
    paramMap[name] = v;
    return dynamic_cast<VecParam *>(v);
  }

  const StructParam *
  FunctBuilder::SymStructParam(const std::string &name, const std::string &structName) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        structMap.contains(structName), "Struct with name \"%s\" is not defined", structName.c_str()
    );
    params.push_back(std::make_unique<StructParam>(name, structName));
    const auto v = params.back().get();
    paramMap[name] = v;
    return dynamic_cast<StructParam *>(v);
  }

  const ScaLocal *FunctBuilder::SymScaLocal(
      const std::string &name, Coef *coef, SymIR::Type type, bool isVolatile
  ) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    locals.push_back(std::make_unique<ScaLocal>(name, coef, type));
    const auto v = locals.back().get();
    if (isVolatile) {
      v->SetVolatile();
    }
    localMap[name] = v;
    return dynamic_cast<const ScaLocal *>(v);
  }

  const VecLocal *FunctBuilder::SymVecLocal(
      const std::string &name, const std::vector<int> &shape, const std::vector<Coef *> coefs,
      SymIR::Type type, std::string structName, bool isVolatile
  ) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    locals.push_back(std::make_unique<VecLocal>(name, shape, coefs, type, structName));
    const auto v = locals.back().get();
    if (isVolatile) {
      v->SetVolatile();
    }
    localMap[name] = v;
    return dynamic_cast<const VecLocal *>(v);
  }

  const StructLocal *FunctBuilder::SymStructLocal(
      const std::string &name, const std::string &structName, const std::vector<Coef *> coefs
  ) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !paramMap.contains(name), "Parameters with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        !localMap.contains(name), "Locals with the same name \"%s\" is already defined",
        name.c_str()
    );
    Assert(
        structMap.contains(structName), "Struct with name \"%s\" is not defined", structName.c_str()
    );
    locals.push_back(std::make_unique<StructLocal>(name, structName, coefs));
    const auto v = locals.back().get();
    localMap[name] = v;
    return dynamic_cast<const StructLocal *>(v);
  }

  const Block *
  FunctBuilder::SymBlock(const std::string &label, const BlockBuilder::BlockBody &body) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    BlockBuilder *b = OpenBlock(label);
    body(b);
    return CloseBlock(b);
  }

  BlockBuilder *FunctBuilder::OpenBlock(const std::string &label) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    Assert(
        !blockMap.contains(label), "Blocks with the same label \"%s\" already exists", label.c_str()
    );
    for (auto &b: createdBlocks) {
      Assert(b.first != label, "Blocks with the same label \"%s\" already exists", label.c_str());
    }
    createdBlocks[label] = std::make_unique<BlockBuilder>(this, label);
    return createdBlocks[label].get();
  }

  const Block *FunctBuilder::CloseBlock(BlockBuilder *builder) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    const std::string &label = builder->GetLabel();
    const auto it = createdBlocks.find(label);
    Assert(
        it != createdBlocks.end(), "Blocks with the same label \"%s\" does not exists",
        label.c_str()
    );
    blocks.push_back(builder->Build());
    blockMap[label] = blocks.back().get();
    createdBlocks.erase(it);
    return blocks.back().get();
  }

  const Block *FunctBuilder::CloseBlockAt(BlockBuilder *builder, const Block *atBlk) {
    Assert(isActive(), "The FunctBuilder is no longer active");
    const std::string &label = builder->GetLabel();
    const auto it = createdBlocks.find(label);
    Assert(
        it != createdBlocks.end(), "Blocks with the same label \"%s\" does not exists",
        label.c_str()
    );
    Assert(atBlk != nullptr, "The given block to insert before cannot be null");
    const auto atPos =
        std::ranges::find_if(blocks, [=](const auto &b) { return b.get() == atBlk; });
    Assert(
        atPos != blocks.end(), "The given block with label \"%s\" is not part of the function",
        atBlk->GetLabel().c_str()
    );
    blocks.insert(atPos, builder->Build());
    blockMap[label] = ((atPos - 1))->get();
    createdBlocks.erase(it);
    return ((atPos - 1))->get();
  }

  std::unique_ptr<Funct> FunctBuilder::Build() {
    Assert(isActive(), "The FunctBuilder is no longer active");
    deactivate();
    return std::make_unique<Funct>(
        name, retType, std::move(structs), std::move(params), std::move(locals), std::move(symbols),
        std::move(blocks)
    );
  }

  std::unique_ptr<Funct> FunctCopier::Copy() {
    src->Accept(*this);
    Assert(builder != nullptr, "The FunctCopier failed to create a function builder");
    auto fun = builder->Build();
    builder = nullptr;
    return fun;
  }

  std::unique_ptr<FunctBuilder> FunctCopier::CopyAsBuilder() {
    src->Accept(*this);
    Assert(builder != nullptr, "The FunctCopier failed to create a function builder");
    return std::move(builder);
  }

  void FunctCopier::Visit(const VarUse &v) {
    for (auto &c: v.GetAccess()) {
      c->Accept(*this);
    }
  }

  void FunctCopier::Visit(const Coef &c) {
    if (auto coef = builder->FindSymbol(c.GetName()); coef != nullptr) {
      Assert(
          typeid(*coef) == typeid(Coef),
          "Symbol \"%s\" is already defined and is not a coefficient", c.GetName().c_str()
      );
      pushCoef(dynamic_cast<Coef *>(coef));
    } else if (c.IsSolved()) {
      pushCoef(builder->SymCoef(c.GetName(), c.GetValue(), c.GetType()));
    } else {
      pushCoef(builder->SymCoef(c.GetName(), c.GetType()));
    }
  }

  void FunctCopier::Visit(const Term &t) {
    t.GetCoef()->Accept(*this);
    const VarDef *var = nullptr;
    std::vector<Coef *> access{};
    if (t.GetOp() != Term::Op::OP_CST) {
      const auto name = t.GetVar()->GetName();
      var = builder->FindVar(name);
      Assert(var != nullptr, "Variable \"%s\" does not exist", name.c_str());
      t.GetVar()->Accept(*this);
      for (size_t i = 0; i < t.GetVar()->GetAccess().size(); i++) {
        access.insert(access.begin(), popCoef());
      }
    }
    pushTerm(currentBlock->SymTerm(t.GetOp(), popCoef(), var, access));
  }

  void FunctCopier::Visit(const Expr &e) {
    const auto &terms = e.GetTerms();
    std::vector<TermID> termIds;
    for (const auto &t: terms) {
      t->Accept(*this);
      termIds.push_back(popTerm());
    }
    pushExpr(currentBlock->SymExpr(e.GetOp(), termIds));
  }

  void FunctCopier::Visit(const Cond &c) {
    c.GetExpr()->Accept(*this);
    auto exprId = popExpr();
    pushCond(currentBlock->SymCond(c.GetOp(), exprId));
  }

  void FunctCopier::Visit(const AssStmt &a) {
    const auto use = a.GetVar();
    const auto name = use->GetName();
    const auto var = builder->FindVar(name);
    Assert(var != nullptr, "Variable \"%s\" does not exist", name.c_str());

    use->Accept(*this);
    std::vector<Coef *> access{};
    for (size_t i = 0; i < use->GetAccess().size(); i++) {
      access.insert(access.begin(), popCoef());
    }

    a.GetExpr()->Accept(*this);
    auto exprId = popExpr();
    currentBlock->SymAssign(var, exprId, access);
  }

  void FunctCopier::Visit(const RetStmt &r) { currentBlock->SymReturn(); }

  void FunctCopier::Visit(const Branch &b) {
    b.GetCond()->Accept(*this);
    auto condId = popCond();
    currentBlock->SymBranch(b.GetTrueTarget(), b.GetFalseTarget(), condId);
  }

  void FunctCopier::Visit(const Goto &g) { currentBlock->SymGoto(g.GetTarget()); }

  void FunctCopier::Visit(const ScaParam &p) {
    builder->SymScaParam(p.GetName(), p.GetType(), p.IsVolatile());
  }

  void FunctCopier::Visit(const VecParam &p) {
    std::string sName = (p.GetBaseType() == SymIR::STRUCT) ? p.GetStructName() : "";
    builder->SymVecParam(p.GetName(), p.GetVecShape(), p.GetBaseType(), sName, p.IsVolatile());
  }

  void FunctCopier::Visit(const StructParam &p) {
    builder->SymStructParam(p.GetName(), p.GetStructName());
  }

  void FunctCopier::Visit(const ScaLocal &l) {
    l.GetCoef()->Accept(*this);
    builder->SymScaLocal(l.GetName(), popCoef(), l.GetType(), l.IsVolatile());
  }

  void FunctCopier::Visit(const VecLocal &l) {
    std::vector<Coef *> coefs{};
    for (auto &c: l.GetCoefs()) {
      c->Accept(*this);
      coefs.push_back(popCoef());
    }
    std::string sName = (l.GetBaseType() == SymIR::STRUCT) ? l.GetStructName() : "";
    builder->SymVecLocal(
        l.GetName(), l.GetVecShape(), coefs, l.GetBaseType(), sName, l.IsVolatile()
    );
  }

  void FunctCopier::Visit(const StructLocal &l) {
    std::vector<Coef *> coefs{};
    for (auto &c: l.GetCoefs()) {
      c->Accept(*this);
      coefs.push_back(popCoef());
    }
    builder->SymStructLocal(l.GetName(), l.GetStructName(), coefs);
  }

  void FunctCopier::Visit(const StructDef &s) { builder->SymStruct(s.GetName(), s.GetFields()); }

  void FunctCopier::Visit(const Block &b) {
    Assert(currentBlock == nullptr, "The FunctCopier already has a current block");
    if (beforeBlockOpenHook) {
      beforeBlockOpenHook(builder.get(), b.GetLabel());
    }
    currentBlock = builder->OpenBlock(b.GetLabel());
    if (afterBlockOpenedHook) {
      afterBlockOpenedHook(builder.get(), currentBlock);
    }
    for (const auto &s: b.GetStmts()) {
      s->Accept(*this);
    }
    if (beforeBlockCloseHook) {
      beforeBlockCloseHook(builder.get(), currentBlock);
    }
    const auto *nb = builder->CloseBlock(currentBlock);
    if (afterBlockClosedHook) {
      afterBlockClosedHook(builder.get(), nb);
    }
    currentBlock = nullptr;
  }

  void FunctCopier::Visit(const Funct &f) {
    Assert(builder == nullptr, "The FunctCopier already has a builder");
    Assert(currentBlock == nullptr, "The FunctCopier already has a current block");
    Assert(coefStack.empty(), "The FunctCopier has a non-empty coefficient stack");
    Assert(termStack.empty(), "The FunctCopier has a non-empty term stack");
    Assert(exprStack.empty(), "The FunctCopier has a non-empty expression stack");
    Assert(condStack.empty(), "The FunctCopier has a non-empty condition stack");
    builder = std::make_unique<FunctBuilder>(f.GetName(), f.GetRetType());
    for (const auto &s: f.GetStructs()) {
      s->Accept(*this);
    }
    for (const auto &p: f.GetParams()) {
      p->Accept(*this);
    }
    for (const auto &l: f.GetLocals()) {
      l->Accept(*this);
    }
    for (const auto &b: f.GetBlocks()) {
      b->Accept(*this);
    }
    Assert(
        currentBlock == nullptr,
        "The FunctCopier finished unexpectedly: It still already has a current block"
    );
    Assert(
        coefStack.empty(), "The FunctCopier finished unexpectedly: It still has a non-empty "
                           "coefficient stack"
    );
    Assert(
        termStack.empty(), "The FunctCopier finished unexpectedly: It still has a non-empty term "
                           "stack"
    );
    Assert(
        exprStack.empty(), "The FunctCopier finished unexpectedly: It still has a non-empty "
                           "expression stack"
    );
    Assert(
        condStack.empty(), "The FunctCopier finished unexpectedly: It still has a non-empty "
                           "condition stack"
    );
  }
} // namespace symir
