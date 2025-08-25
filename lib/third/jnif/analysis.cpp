//
// Created by Luigi on 22.10.17.
//

#include <iostream>

#include "jnif/jnif.hpp"
#include "lib/dbgutils.hpp"
#include "lib/logger.hpp"

namespace jnif {

  void BasicBlock::addTarget(BasicBlock *target) {
    JnifError::checks(cfg == target->cfg, "invalid owner for basic block");

    targets.push_back(target);
    target->ins.push_back(this);
  }

  static void addBasicBlock2(
      InstList::Iterator eit, InstList::Iterator &beginBb, int &bbid, ControlFlowGraph &cfg
  ) {
    if (beginBb != eit) {
      std::stringstream ss;
      ss << "BB" << bbid;
      string name = ss.str();

      cfg.addBasicBlock(beginBb, eit, name);

      beginBb = eit;
      bbid++;
    }
  }

  static void buildBasicBlocks(InstList &instList, ControlFlowGraph &cfg) {
    // setBranchTargets(instList);

    int bbid = 0;
    InstList::Iterator beginBb = instList.begin();

    for (InstList::Iterator it = instList.begin(); it != instList.end(); ++it) {
      Inst *inst = *it;

      if (inst->isLabel() && (inst->label()->isBranchTarget || inst->label()->isTryStart)) {
        addBasicBlock2(it, beginBb, bbid, cfg);
      }

      if (inst->isBranch() || inst->isExit()) {
        InstList::Iterator eit = it;
        ++eit;
        addBasicBlock2(eit, beginBb, bbid, cfg);
      }
    }
  }

  static void addTarget2(BasicBlock *bb, const Inst *inst, ControlFlowGraph &cfg) {
    JnifError::asserts(inst->isLabel(), "Expected label instruction");
    int labelId = inst->label()->id;
    BasicBlock *tbbid = cfg.findBasicBlockOfLabel(labelId);
    bb->addTarget(tbbid);
  }

  static void buildCfg(InstList &instList, ControlFlowGraph &cfg) {
    buildBasicBlocks(instList, cfg);

    for (BasicBlock *bb: cfg) {
      if (bb->start == instList.end()) {
        JnifError::asserts(
            bb->name == ControlFlowGraph::EntryName || bb->name == ControlFlowGraph::ExitName, ""
        );
        JnifError::asserts(bb->exit == instList.end(), "");
        continue;
      }

      InstList::Iterator e = bb->exit;
      --e;
      JnifError::asserts(e != instList.end(), "");

      Inst *last = *e;

      if (bb->start == instList.begin()) {
        cfg.entry->addTarget(bb);
      }

      if (last->isJump()) {
        addTarget2(bb, last->jump()->label2, cfg);

        if (last->opcode != Opcode::GOTO) {
          JnifError::asserts(bb->next != NULL, "next bb is null");
          bb->addTarget(bb->next);
        }
        bb->last = last;
      } else if (last->isTableSwitch()) {
        addTarget2(bb, last->ts()->def, cfg);

        for (Inst *target: last->ts()->targets) {
          addTarget2(bb, target, cfg);
        }
        bb->last = last;
      } else if (last->isLookupSwitch()) {
        addTarget2(bb, last->ls()->defbyte, cfg);

        for (Inst *target: last->ls()->targets) {
          addTarget2(bb, target, cfg);
        }
        bb->last = last;
      } else if (last->isExit()) {
        bb->addTarget(cfg.exit);
        bb->last = last;
      } else {
        JnifError::asserts(bb->next != nullptr, "next bb is null");
        bb->addTarget(bb->next);
      }
    }
  }

  ControlFlowGraph::ControlFlowGraph(InstList &instList) :
      entry(addConstBb(instList, ".Entry")), exit(addConstBb(instList, ".Exit")),
      instList(instList) {
    buildCfg(instList, *this);

    // std::vector<BasicBlock*>::iterator it = basicBlocks.begin();
    // JnifError::asserts((*it)->name == EntryName, "Invalid entry");
    // it++;
    // JnifError::asserts((*it)->name == ExitName, "Invalid entry");

    // basicBlocks.push_back(*it);
    // basicBlocks.erase(it);
  }

  ControlFlowGraph::~ControlFlowGraph() {
    for (auto bb: *this) {
      delete bb;
    }
  }

  BasicBlock *ControlFlowGraph::addBasicBlock(
      InstList::Iterator start, InstList::Iterator end, const string &name
  ) {
    BasicBlock *const bb = new BasicBlock(start, end, name, this);

    if (basicBlocks.size() > 0) {
      BasicBlock *prevbb = basicBlocks.back();
      prevbb->next = bb;
    }

    basicBlocks.push_back(bb);

    return bb;
  }

  BasicBlock *ControlFlowGraph::findBasicBlockOfLabel(int labelId) const {
    for (BasicBlock *bb: *this) {
      if (bb->start == instList.end()) {
        JnifError::asserts(bb->name == EntryName || bb->name == ExitName, "");
        JnifError::asserts(bb->exit == instList.end(), "");
        continue;
      }

      Inst *inst = *bb->start;
      if (inst->isLabel() && inst->label()->id == labelId) {
        return bb;
      }
    }

    throw Exception(
        "Invalid label id: ", labelId, " for the instruction list: ", ", in cfg: ", *this, instList
    );
  }

  ControlFlowGraph::D ControlFlowGraph::dominance(BasicBlock *) {
    ControlFlowGraph &cfg = *this;
    map<BasicBlock *, set<BasicBlock *>> ds;

    for (BasicBlock *bb: cfg) {
      ds[bb].insert(cfg.basicBlocks.begin(), cfg.basicBlocks.end());
    }

    bool changed = true;
    while (changed) {
      changed = false;

      for (BasicBlock *bb: cfg) {
        set<BasicBlock *> ns;
        for (BasicBlock *p: bb->targets) {
          if (ns.empty()) {
            ns = ds[p];
          } else {
            for (BasicBlock *bbp: ns) {
              if (ds[p].count(bbp) == 0) {
                ns.erase(bbp);
              }
            }
          }
        }
        ns.insert(bb);

        if (ns != ds[bb]) {
          changed = true;
          ds[bb] = ns;
        }
      }
    }

    return ds;
  }

  class JsrRetNotSupported {};

  template<class TFrame>
  class SmtBuilder {
  public:
    SmtBuilder(TFrame &frame, const ConstPool &cp) : frame(frame), cp(cp) {}

    void exec(Inst &inst) {
      switch (inst.opcode) {
        case Opcode::nop:
          break;
        case Opcode::aconst_null:
          frame.pushNull(&inst);
          break;
        case Opcode::iconst_m1:
        case Opcode::iconst_0:
        case Opcode::iconst_1:
        case Opcode::iconst_2:
        case Opcode::iconst_3:
        case Opcode::iconst_4:
        case Opcode::iconst_5:
        case Opcode::bipush:
        case Opcode::sipush:
          frame.pushInt(&inst);
          break;
        case Opcode::lconst_0:
        case Opcode::lconst_1:
          frame.pushLong(&inst);
          break;
        case Opcode::fconst_0:
        case Opcode::fconst_1:
        case Opcode::fconst_2:
          frame.pushFloat(&inst);
          break;
        case Opcode::dconst_0:
        case Opcode::dconst_1:
          frame.pushDouble(&inst);
          break;
        case Opcode::ldc:
        case Opcode::ldc_w:
          ldc(inst);
          break;
        case Opcode::ldc2_w:
          ldc2(inst);
          break;
        case Opcode::iload:
          iload(inst.var()->lvindex, &inst);
          break;
        case Opcode::iload_0:
          iload(0, &inst);
          break;
        case Opcode::iload_1:
          iload(1, &inst);
          break;
        case Opcode::iload_2:
          iload(2, &inst);
          break;
        case Opcode::iload_3:
          iload(3, &inst);
          break;
        case Opcode::lload:
          lload(inst.var()->lvindex, &inst);
          break;
        case Opcode::lload_0:
          lload(0, &inst);
          break;
        case Opcode::lload_1:
          lload(1, &inst);
          break;
        case Opcode::lload_2:
          lload(2, &inst);
          break;
        case Opcode::lload_3:
          lload(3, &inst);
          break;
        case Opcode::fload:
          fload(inst.var()->lvindex, &inst);
          break;
        case Opcode::fload_0:
          fload(0, &inst);
          break;
        case Opcode::fload_1:
          fload(1, &inst);
          break;
        case Opcode::fload_2:
          fload(2, &inst);
          break;
        case Opcode::fload_3:
          fload(3, &inst);
          break;
        case Opcode::dload:
          dload(inst.var()->lvindex, &inst);
          break;
        case Opcode::dload_0:
          dload(0, &inst);
          break;
        case Opcode::dload_1:
          dload(1, &inst);
          break;
        case Opcode::dload_2:
          dload(2, &inst);
          break;
        case Opcode::dload_3:
          dload(3, &inst);
          break;
        case Opcode::aload:
          aload(inst.var()->lvindex, &inst);
          break;
        case Opcode::aload_0:
          aload(0, &inst);
          break;
        case Opcode::aload_1:
          aload(1, &inst);
          break;
        case Opcode::aload_2:
          aload(2, &inst);
          break;
        case Opcode::aload_3:
          aload(3, &inst);
          break;
        case Opcode::iaload:
        case Opcode::baload:
        case Opcode::caload:
        case Opcode::saload:
          frame.popIntegral(&inst);
          frame.popArray(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::laload:
          frame.popIntegral(&inst);
          frame.popArray(&inst);
          frame.pushLong(&inst);
          break;
        case Opcode::faload:
          frame.popIntegral(&inst);
          frame.popArray(&inst);
          frame.pushFloat(&inst);
          break;
        case Opcode::daload: {
          frame.popIntegral(&inst);
          Type arrayType = frame.popArray(&inst);
          frame.pushDouble(&inst);
          break;
        }
        case Opcode::aaload:
          aaload(inst);
          break;
        case Opcode::istore:
          istore(inst.var()->lvindex, &inst);
          break;
        case Opcode::lstore:
          lstore(inst.var()->lvindex, &inst);
          break;
        case Opcode::fstore:
          fstore(inst.var()->lvindex, &inst);
          break;
        case Opcode::dstore:
          dstore(inst.var()->lvindex, &inst);
          break;
        case Opcode::astore:
          astore(inst.var()->lvindex, &inst);
          break;
        case Opcode::istore_0:
          istore(0, &inst);
          break;
        case Opcode::istore_1:
          istore(1, &inst);
          break;
        case Opcode::istore_2:
          istore(2, &inst);
          break;
        case Opcode::istore_3:
          istore(3, &inst);
          break;
        case Opcode::lstore_0:
          lstore(0, &inst);
          break;
        case Opcode::lstore_1:
          lstore(1, &inst);
          break;
        case Opcode::lstore_2:
          lstore(2, &inst);
          break;
        case Opcode::lstore_3:
          lstore(3, &inst);
          break;
        case Opcode::fstore_0:
          fstore(0, &inst);
          break;
        case Opcode::fstore_1:
          fstore(1, &inst);
          break;
        case Opcode::fstore_2:
          fstore(2, &inst);
          break;
        case Opcode::fstore_3:
          fstore(3, &inst);
          break;
        case Opcode::dstore_0:
          dstore(0, &inst);
          break;
        case Opcode::dstore_1:
          dstore(1, &inst);
          break;
        case Opcode::dstore_2:
          dstore(2, &inst);
          break;
        case Opcode::dstore_3:
          dstore(3, &inst);
          break;
        case Opcode::astore_0:
          astore(0, &inst);
          break;
        case Opcode::astore_1:
          astore(1, &inst);
          break;
        case Opcode::astore_2:
          astore(2, &inst);
          break;
        case Opcode::astore_3:
          astore(3, &inst);
          break;
        case Opcode::iastore:
        case Opcode::bastore:
        case Opcode::castore:
        case Opcode::sastore:
          frame.popIntegral(&inst);
          xastore(&inst);
          break;
        case Opcode::lastore:
          frame.popLong(&inst);
          xastore(&inst);
          break;
        case Opcode::fastore:
          frame.popFloat(&inst);
          xastore(&inst);
          break;
        case Opcode::dastore:
          frame.popDouble(&inst);
          xastore(&inst);
          break;
        case Opcode::aastore:
          frame.popRef(&inst);
          xastore(&inst);
          break;
        case Opcode::pop:
          frame.popOneWord(&inst);
          break;
        case Opcode::pop2:
          frame.popTwoWord(&inst);
          break;
        case Opcode::dup: {
          const Type &t1 = frame.popOneWord(&inst);
          frame.push(t1, &inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::dup_x1: {
          const Type &t1 = frame.popOneWord(&inst);
          const Type &t2 = frame.popOneWord(&inst);
          frame.push(t1, &inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::dup_x2: {
          const Type &t1 = frame.pop(&inst);
          const Type &t2 = frame.pop(&inst);
          const Type &t3 = frame.pop(&inst);
          frame.push(t1, &inst);
          frame.push(t3, &inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::dup2: {
          const Type &t1 = frame.pop(&inst);
          const Type &t2 = frame.pop(&inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::dup2_x1: {
          const Type &t1 = frame.pop(&inst);
          const Type &t2 = frame.pop(&inst);
          const Type &t3 = frame.pop(&inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          frame.push(t3, &inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::dup2_x2: {
          const Type &t1 = frame.pop(&inst);
          const Type &t2 = frame.pop(&inst);
          const Type &t3 = frame.pop(&inst);
          const Type &t4 = frame.pop(&inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          frame.push(t4, &inst);
          frame.push(t3, &inst);
          frame.push(t2, &inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::swap: {
          const Type &t1 = frame.pop(&inst);
          const Type &t2 = frame.pop(&inst);
          frame.push(t1, &inst);
          frame.push(t2, &inst);
          break;
        }
        case Opcode::iadd:
        case Opcode::fadd:
        case Opcode::isub:
        case Opcode::fsub:
        case Opcode::imul:
        case Opcode::fmul:
        case Opcode::idiv:
        case Opcode::fdiv:
        case Opcode::irem:
        case Opcode::frem:
        case Opcode::ishl:
        case Opcode::ishr:
        case Opcode::iushr:
        case Opcode::iand:
        case Opcode::ior:
        case Opcode::ixor: {
          const Type &t1 = frame.pop(&inst);
          frame.pop(&inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::ladd:
        case Opcode::lsub:
        case Opcode::lmul:
        case Opcode::ldiv:
        case Opcode::lrem:
        case Opcode::land:
        case Opcode::lor:
        case Opcode::lxor:
          frame.popLong(&inst);
          frame.popLong(&inst);
          frame.pushLong(&inst);
          break;
        case Opcode::lshl:
        case Opcode::lshr:
        case Opcode::lushr:
          frame.popIntegral(&inst);
          frame.popLong(&inst);
          frame.pushLong(&inst);
          break;
        case Opcode::dadd:
        case Opcode::dsub:
        case Opcode::dmul:
        case Opcode::ddiv:
        case Opcode::drem: {
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pushDouble(&inst);
          break;
        }
        case Opcode::ineg:
        case Opcode::fneg: {
          const Type &t1 = frame.pop(&inst);
          frame.push(t1, &inst);
          break;
        }
        case Opcode::lneg: {
          frame.popLong(&inst);
          frame.pushLong(&inst);
          break;
        }
        case Opcode::dneg: {
          frame.popDouble(&inst);
          frame.pushDouble(&inst);
          break;
        }
        case Opcode::iinc:
          iinc(inst.iinc()->index, &inst);
          break;
        case Opcode::i2l:
          frame.popIntegral(&inst);
          frame.pushLong(&inst);
          break;
        case Opcode::i2f:
          frame.popIntegral(&inst);
          frame.pushFloat(&inst);
          break;
        case Opcode::i2d:
          frame.popIntegral(&inst);
          frame.pushDouble(&inst);
          break;
        case Opcode::l2i:
          frame.popLong(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::l2f:
          frame.popLong(&inst);
          frame.pushFloat(&inst);
          break;
        case Opcode::l2d:
          frame.popLong(&inst);
          frame.pushDouble(&inst);
          break;
        case Opcode::f2i:
          frame.popFloat(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::f2l:
          frame.popFloat(&inst);
          frame.pushLong(&inst);
          break;
        case Opcode::f2d:
          frame.popFloat(&inst);
          frame.pushDouble(&inst);
          break;
        case Opcode::d2i:
          frame.popDouble(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::d2l:
          frame.popDouble(&inst);
          frame.pushLong(&inst);
          break;
        case Opcode::d2f:
          frame.popDouble(&inst);
          frame.pushFloat(&inst);
          break;
        case Opcode::i2b:
        case Opcode::i2c:
        case Opcode::i2s:
          frame.popIntegral(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::lcmp:
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::fcmpl:
        case Opcode::fcmpg:
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::dcmpl:
        case Opcode::dcmpg:
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pop(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::ifeq:
        case Opcode::ifne:
        case Opcode::iflt:
        case Opcode::ifge:
        case Opcode::ifgt:
        case Opcode::ifle:
          frame.pop(&inst);
          break;
        case Opcode::if_icmpeq:
        case Opcode::if_icmpne:
        case Opcode::if_icmplt:
        case Opcode::if_icmpge:
        case Opcode::if_icmpgt:
        case Opcode::if_icmple:
          frame.popIntegral(&inst);
          frame.popIntegral(&inst);
          break;
        case Opcode::if_acmpeq:
        case Opcode::if_acmpne:
          frame.pop(&inst);
          frame.pop(&inst);
          break;
        case Opcode::GOTO:
          break;
        case Opcode::jsr:
          throw JsrRetNotSupported();
          break;
        case Opcode::ret:
          throw JsrRetNotSupported();
          break;
        case Opcode::tableswitch:
        case Opcode::lookupswitch:
          frame.pop(&inst);
          break;
        case Opcode::ireturn:
          frame.pop(&inst);
          break;
        case Opcode::lreturn:
          frame.pop(&inst);
          frame.pop(&inst);
          break;
        case Opcode::freturn:
          frame.pop(&inst);
          break;
        case Opcode::dreturn:
          frame.pop(&inst);
          frame.pop(&inst);
          break;
        case Opcode::areturn:
          frame.pop(&inst);
          break;
        case Opcode::RETURN:
          break;
        case Opcode::getstatic: {
          const Type &t = fieldType(inst);
          frame.pushType(t, &inst);
          break;
        }
        case Opcode::putstatic: {
          const Type &t = fieldType(inst);
          frame.popType(t, &inst);
          break;
        }
        case Opcode::getfield: {
          const Type &t = fieldType(inst);
          frame.popRef(&inst);
          frame.pushType(t, &inst);
          break;
        }
        case Opcode::putfield: {
          const Type &t = fieldType(inst);
          frame.popType(t, &inst);
          frame.popRef(&inst);
          break;
        }
        case Opcode::invokevirtual:
          invokeMethod(inst.invoke()->methodRefIndex, true, false, &inst);
          break;
        case Opcode::invokespecial:
          invokeSpecial(inst.invoke()->methodRefIndex, &inst);
          break;
        case Opcode::invokestatic:
          invokeStatic(inst.invoke()->methodRefIndex, &inst);
          break;
        case Opcode::invokeinterface:
          invokeInterface(inst.invokeinterface()->interMethodRefIndex, true, &inst);
          break;
        case Opcode::NEW:
          newinst(*inst.type());
          break;
        case Opcode::newarray:
          newarray(inst);
          break;
        case Opcode::anewarray:
          anewarray(inst);
          break;
        case Opcode::arraylength:
          frame.pop(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::athrow:
          athrow(inst);
          break;
        case Opcode::checkcast:
          checkcast(inst);
          break;
        case Opcode::instanceof:
          frame.popRef(&inst);
          frame.pushInt(&inst);
          break;
        case Opcode::monitorenter:
        case Opcode::monitorexit:
          frame.popRef(&inst);
          break;
        case Opcode::wide:
          wide(inst);
          break;
        case Opcode::multianewarray:
          multianewarray(inst);
          break;
        case Opcode::ifnull:
        case Opcode::ifnonnull:
          frame.pop(&inst);
          break;
        case Opcode::goto_w:
        case Opcode::jsr_w:
        case Opcode::breakpoint:
        case Opcode::impdep1:
        case Opcode::impdep2:
          throw Exception("goto_w, jsr_w breakpoint not implemented");
          break;
        case Opcode::invokedynamic: {
          ConstPool::Index callSite = inst.indy()->callSite();
          const ConstPool::InvokeDynamic &dyn = cp.getInvokeDynamic(callSite);

          string name;
          string desc;
          cp.getNameAndType(dyn.nameAndTypeIndex, &name, &desc);

          // cerr << name << endl;
          // cerr << desc << endl;

          invoke("invokedynamic call site", name, desc, false, false, &inst);

          // Error::raise("invoke dynamic instances not implemented");
          break;
        }
        default:
          throw Exception("unknown opcode not implemented: ", inst.opcode);
      }
    }

  private:
    void newinst(TypeInst &inst) {
      const string &className = cp.getClassName(inst.type()->classIndex);
      const Type &t = TypeFactory::fromConstClass(className);
      t.init = false;
      t.typeId = Type::nextTypeId;
      Type::nextTypeId++;

      t.uninit.newinst = &inst;
      JnifError::checks(!t.isArray(), "New with array: ", t);
      frame.push(t, &inst);
    }

    void newarray(Inst &inst) {
      frame.popIntegral(&inst);
      frame.pushArray(getArrayBaseType(inst.newarray()->atype), 1, &inst);
    }

    void anewarray(Inst &inst) {
      frame.popIntegral(&inst);
      const string &className = cp.getClassName(inst.type()->classIndex);
      const Type &t = TypeFactory::fromConstClass(className);
      frame.pushArray(t, t.getDims() + 1, &inst);
    }

    void aaload(Inst &inst) {
      frame.popIntegral(&inst);
      Type arrayType = frame.popArray(&inst);
      if (arrayType.isNull()) {
        frame.pushNull(&inst);
      } else {
        Type elementType = arrayType.elementType();
        JnifError::checks(elementType.isObject(), "Not an object:", elementType);
        frame.push(elementType, &inst);
      }
    }

    void ldc(Inst &inst) {
      ConstPool::Tag tag = cp.getTag(inst.ldc()->valueIndex);
      switch (tag) {
        case ConstPool::INTEGER:
          frame.pushInt(&inst);
          break;
        case ConstPool::FLOAT:
          frame.pushFloat(&inst);
          break;
        case ConstPool::CLASS:
          frame.pushRef(cp.getClassName(inst.ldc()->valueIndex), &inst);
          break;
        case ConstPool::STRING:
          frame.pushRef("java/lang/String", &inst);
          break;
        default:
          throw Exception("Invalid tag entry: ", tag);
      }
    }

    void ldc2(Inst &inst) {
      ConstPool::Tag tag = cp.getTag(inst.ldc()->valueIndex);
      switch (tag) {
        case ConstPool::LONG:
          frame.pushLong(&inst);
          break;
        case ConstPool::DOUBLE:
          frame.pushDouble(&inst);
          break;
        default:
          throw Exception("Invalid constant for ldc2_w");
      }
    }

    void athrow(Inst &inst) {
      Type t = frame.popRef(&inst);
      frame.clearStack();
      frame.push(t, &inst);
    }

    void checkcast(Inst &inst) {
      frame.popRef(&inst);
      const string &className = cp.getClassName(inst.type()->classIndex);
      frame.push(TypeFactory::fromConstClass(className), &inst);
    }

    void istore(int lvindex, Inst *inst) {
      frame.popIntegral(inst);
      frame.setIntVar(lvindex, inst);
    }

    void lstore(int lvindex, Inst *inst) {
      frame.popLong(inst);
      frame.setLongVar(lvindex, inst);
    }

    void fstore(int lvindex, Inst *inst) {
      frame.popFloat(inst);
      frame.setFloatVar(lvindex, inst);
    }

    void dstore(int lvindex, Inst *inst) {
      frame.popDouble(inst);
      frame.setDoubleVar(lvindex, inst);
    }

    void astore(int lvindex, Inst *inst) {
      Type refType = frame.popRef(inst);
      JnifError::checks(
          !refType.isTop(), "astore: Setting variable index ", lvindex, " to Top", refType,
          " in frame ", frame
      );

      frame.setRefVar(lvindex, refType, inst);
    }

    void iload(u4 lvindex, Inst *inst, int offset = 0) {
      const Type &type = frame.getVar(lvindex, inst);
      JnifError::checks(
          type.isIntegral(), "iload: ", type, " at index ", lvindex, ":offset:", offset, " for "
      );
      frame.pushInt(inst);
    }

    void fload(u4 lvindex, Inst *inst) {
      const Type &type = frame.getVar(lvindex, inst);
      JnifError::checks(type.isFloat(), "fload: ", type, " @ ", lvindex);
      frame.pushFloat(inst);
    }

    void lload(u4 lvindex, Inst *inst) {
      const Type &type = frame.getVar(lvindex, inst);
      JnifError::checks(type.isLong(), "lload: ", type, " @ ", lvindex);
      frame.pushLong(inst);
    }

    void dload(u4 lvindex, Inst *inst) {
      const Type &type = frame.getVar(lvindex, inst);
      JnifError::checks(type.isDouble(), "dload: ", type, " @ ", lvindex);
      frame.pushDouble(inst);
    }

    void aload(u4 lvindex, Inst *inst) {
      const Type &type = frame.getVar(lvindex, inst);
      JnifError::checks(
          type.isObject() || type.isNull() || type.isUninitThis(), "Bad ref var at index[", lvindex,
          "]: ", type, " @ frame: ", frame
      );
      frame.pushType(type, inst);
    }

    void xastore(Inst *inst) {
      frame.popIntegral(inst);
      frame.popArray(inst);
    }

    void iinc(u2 index, Inst *inst) {
      Type type = frame.getVar(index, inst);
      JnifError::checks(type.isInt());
      frame.setIntVar(index, inst);
    }

    void invokeMethod(u2 methodRefIndex, bool popThis, bool isSpecial, Inst *inst) {
      string className, name, desc;
      cp.getMethodRef(methodRefIndex, &className, &name, &desc);
      invoke(className, name, desc, popThis, isSpecial, inst);
    }

    void invokeSpecial(u2 methodRefIndex, Inst *inst) {
      ConstPool::Tag tag = cp.getTag(methodRefIndex);
      JnifError::checks(
          tag == ConstPool::METHODREF || tag == ConstPool::INTERMETHODREF,
          "INVOKESPECIAL index must be either a method or an interface method symbolic reference"
      );
      if (tag == ConstPool::METHODREF) {
        invokeMethod(methodRefIndex, true, true, inst);
      } else {
        invokeInterface(methodRefIndex, true, inst);
      }
    }

    void invokeInterface(u2 interMethodRefIndex, bool popThis, Inst *inst) {
      string className, name, desc;
      cp.getInterMethodRef(interMethodRefIndex, &className, &name, &desc);
      invoke(className, name, desc, popThis, false, inst);
    }

    void invokeStatic(u2 methodRefIndex, Inst *inst) {
      ConstPool::Tag tag = cp.getTag(methodRefIndex);
      JnifError::checks(
          tag == ConstPool::METHODREF || tag == ConstPool::INTERMETHODREF,
          "INVOKESPECIAL index must be either a method or an interface method symbolic reference"
      );
      if (tag == ConstPool::METHODREF) {
        invokeMethod(methodRefIndex, false, false, inst);
      } else {
        invokeInterface(methodRefIndex, false, inst);
      }
    }

    void invoke(
        const string &className, const string &name, const string &desc, bool popThis,
        bool isSpecial, Inst *inst
    ) {
      const char *d = desc.c_str();
      std::vector<Type> argsType;
      const Type &returnType = TypeFactory::fromMethodDesc(d, &argsType);

      for (int i = argsType.size() - 1; i >= 0; i--) {
        const Type &argType = argsType[i];
        JnifError::checks(argType.isOneOrTwoWord(), "Invalid arg type in method");
        frame.popType(argType, inst);
      }

      if (popThis) {
        Type t = frame.popRef(inst);
        // JnifError::checks(t.iso)
        if (isSpecial && name == "<init>") {
          JnifError::checks(t.typeId > 0, "inv typeId: ", t.typeId, t);
          JnifError::checks(
              !t.init, "Object is already init: ", t, ", ", className, ".", name, desc,
              ", frame: ", frame
          );

          t.init = true;
          frame.init(t);
        }
      }

      if (!returnType.isVoid()) {
        JnifError::asserts(returnType.isOneOrTwoWord(), "Ret type: ", returnType);
        frame.pushType(returnType, inst);
      }
    }

    Type fieldType(Inst &inst) {
      string className, name, desc;
      cp.getFieldRef(inst.field()->fieldRefIndex, &className, &name, &desc);

      const char *d = desc.c_str();
      const Type &t = TypeFactory::fromFieldDesc(d);

      return t;
    }

    void multianewarray(Inst &inst) {
      u1 dims = inst.multiarray()->dims;
      JnifError::checks(dims >= 1, "invalid dims: ", dims);

      for (int i = 0; i < dims; i++) {
        frame.popIntegral(&inst);
      }

      string arrayClassName = cp.getClassName(inst.multiarray()->classIndex);
      const char *d = arrayClassName.c_str();
      Type arrayType = TypeFactory::fromFieldDesc(d);

      frame.pushType(arrayType, &inst);
    }

    void wide(Inst &inst) {
      u2 lvindex = inst.wide()->var.lvindex;
      switch (inst.wide()->subOpcode) {
        case Opcode::iload:
          iload(lvindex, &inst);
          break;
        case Opcode::lload:
          lload(lvindex, &inst);
          break;
        case Opcode::fload:
          fload(lvindex, &inst);
          break;
        case Opcode::dload:
          dload(lvindex, &inst);
          break;
        case Opcode::aload:
          aload(lvindex, &inst);
          break;
        case Opcode::istore:
          istore(lvindex, &inst);
          break;
        case Opcode::fstore:
          fstore(lvindex, &inst);
          break;
        case Opcode::lstore:
          lstore(lvindex, &inst);
          break;
        case Opcode::dstore:
          dstore(lvindex, &inst);
          break;
        case Opcode::astore:
          astore(lvindex, &inst);
          break;
        case Opcode::iinc:
          iinc(inst.wide()->iinc.index, &inst);
          break;
        default:
          throw Exception("Unsupported wide opcode: ", inst.wide()->subOpcode);
      }
    }

    const Type &getArrayBaseType(int atype) {
      switch (atype) {
        case NewArrayInst::NEWARRAYTYPE_BOOLEAN:
          return TypeFactory::booleanType();
        case NewArrayInst::NEWARRAYTYPE_CHAR:
          return TypeFactory::charType();
        case NewArrayInst::NEWARRAYTYPE_BYTE:
          return TypeFactory::byteType();
        case NewArrayInst::NEWARRAYTYPE_SHORT:
          return TypeFactory::shortType();
        case NewArrayInst::NEWARRAYTYPE_INT:
          return TypeFactory::intType();
        case NewArrayInst::NEWARRAYTYPE_FLOAT:
          return TypeFactory::floatType();
        case NewArrayInst::NEWARRAYTYPE_LONG:
          return TypeFactory::longType();
        case NewArrayInst::NEWARRAYTYPE_DOUBLE:
          return TypeFactory::doubleType();
      }

      throw Exception("invalid atype: ", atype);
    }

    TFrame &frame;
    const ConstPool &cp;
  };

  // static std::ostream& operator<<(std::ostream& os, std::set<Inst*> ls) {
  //     os << "{";
  //     for (Inst* t : ls) {
  //         os << t->_offset << " ";
  //     }
  //     os << "}";
  //     return os;
  // }

  // Compute the correct frames for each basic block and save them in the class file.
  // This procedure follows JVM Spec: Section 4.10.2.2. See for example Java 8:
  // https://docs.oracle.com/javase/specs/jvms/se8/html/jvms-4.html#jvms-4.10.2.2
  class ComputeFrames {
  public:
    bool isAssignable(const Type &subt, const Type &supt) {
      if (subt == supt) {
        return true;
      }

      if (supt.isTop()) {
        return true;
      }

      if (subt.isNull() && supt.isObject()) {
        return true;
      }

      if (subt.isIntegral() && supt.isInt()) {
        JnifError::asserts(!subt.isInt(), "Invalid subt");
        return true;
      }

      return false;
    }

    string
    getCommonSuperClass(const string &classLeft, const string &classRight, IClassPath *classPath) {
      if (classLeft == "java/lang/Object" || classRight == "java/lang/Object") {
        return "java/lang/Object";
      }

      return classPath->getCommonSuperClass(classLeft, classRight);
    }

    bool assign(Type &t, Type o, IClassPath *classPath) {
      if (!isAssignable(t, o) && !isAssignable(o, t)) {
        if (t.isClass() && o.isClass()) {
          string clazz1 = t.getClassName();
          string clazz2 = o.getClassName();

          string res = getCommonSuperClass(clazz1, clazz2, classPath);

          Type superClass = TypeFactory::objectType(res);
          JnifError::asserts(
              (superClass == t) == (res == clazz1), "Invalid super class: ", superClass, t, o
          );

          if (superClass == t) {
            return false;
          }

          t = superClass;
          return true;
        }

        if (t.isArray() && o.isArray()) {
          if (t.getDims() != o.getDims()) {
            t = TypeFactory::objectType("java/lang/Object");
            return true;
          }

          Type st = t.stripArrayType();
          Type so = o.stripArrayType();

          // bool change =
          assign(st, so, classPath);
          //				JnifError::asserts(change, "Assigning types between ", t,
          //						" (with stripped array type ", st, ") and ", o,
          //						" (with stripped array type ", so,
          //						") should have change the assign result.");

          //				JnifError::asserts(!st.isTop(), "Assigning types between ", t,
          //						" and ", o,
          //						" should have not change assign result to Top.");
          if (st.isTop()) {
            t = TypeFactory::objectType("java/lang/Object");
            return true;
          }

          t = TypeFactory::arrayType(st, t.getDims());
          return true;
        }

        if ((t.isClass() && o.isArray()) || (t.isArray() && o.isClass())) {
          t = TypeFactory::objectType("java/lang/Object");
          return true;
        }

        //			JnifError::raise("We arrived here, and we returning top: ", t, " and ",
        //					o);

        t = TypeFactory::topType();
        return true;
      }

      if (isAssignable(t, o)) {
        if (t == o) {
          return false;
        }

        t = o;
        return true;
      }

      JnifError::asserts(isAssignable(o, t), "Invalid assign type: ", t, " <> ", o);

      return false;
    }

    bool includes(const std::set<Inst *> &xs, const std::set<Inst *> &ys) {
      for (Inst *y: ys) {
        if (xs.find(y) == xs.end()) {
          return false;
        }
      }

      return true;
    }

    bool join(Frame &frame, Frame &how, IClassPath *classPath, Method *method = NULL) {
      JnifError::checks(
          frame.stack.size() == how.stack.size(), "Different stack sizes: ", frame.stack.size(),
          " != ", how.stack.size(), ": #", frame, " != #", how, "Method: ", method
      );

      std::set<Inst *> ls;
      const Frame::T defType = std::make_pair(TypeFactory::topType(), ls);
      if (frame.lva.size() < how.lva.size()) {
        frame.lva.resize(how.lva.size(), defType);
      } else if (how.lva.size() < frame.lva.size()) {
        how.lva.resize(frame.lva.size(), defType);
      }

      JnifError::asserts(
          frame.lva.size() == how.lva.size(), "%ld != %ld", frame.lva.size(), how.lva.size()
      );

      bool change = false;

      for (u4 i = 0; i < frame.lva.size(); i++) {
        bool assignChanged = assign(frame.lva[i].first, how.lva[i].first, classPath);

        std::set<Inst *> &xs = frame.lva[i].second;
        std::set<Inst *> &ys = how.lva[i].second;

        if (!includes(xs, ys)) {
          // std::cerr << "lva" << " xs: " << xs << "ys: " << ys << std::endl;
          xs.insert(ys.begin(), ys.end());
          ys.insert(xs.begin(), xs.end());
          change = true;
        }

        change = change || assignChanged;
      }

      std::list<Frame::T>::iterator i = frame.stack.begin();
      std::list<Frame::T>::iterator j = how.stack.begin();

      for (; i != frame.stack.end(); i++, j++) {
        bool assignChanged = assign(i->first, j->first, classPath);

        std::set<Inst *> &xs = i->second;
        std::set<Inst *> &ys = j->second;
        if (!includes(xs, ys)) {
          // std::cerr << "stack" << " xs: " << xs << "ys: " << ys << std::endl;
          xs.insert(ys.begin(), ys.end());
          ys.insert(xs.begin(), xs.end());
          change = true;
        }

        change = change || assignChanged;
      }

      return change;
    }

    Type getExceptionType(const ConstPool &cp, ConstPool::Index catchIndex) {
      if (catchIndex != ConstPool::NULLENTRY) {
        const string &className = cp.getClassName(catchIndex);
        return TypeFactory::fromConstClass(className);
      } else {
        return TypeFactory::objectType("java/lang/Throwable");
      }
    }

    void visitCatch(
        const CodeAttr::ExceptionHandler &ex, InstList &instList, const ClassFile &cf,
        const CodeAttr *code, IClassPath *classPath, const ControlFlowGraph *cfg, Frame frame,
        Method *method
    ) {
      int handlerPcId = ex.handlerpc->label()->id;
      BasicBlock *handlerBb = cfg->findBasicBlockOfLabel(handlerPcId);

      //		Type exType = [&]() {
      //			if (ex.catchtype != ConstPool::NULLENTRY) {
      //				const String& className = cf.getClassName(ex.catchtype);
      //				return Type::fromConstClass(className);
      //			} else {
      //				return Type::objectType("java/lang/Throwable");
      //			}
      //		}();
      const Type &exType = getExceptionType(cf, ex.catchtype);

      frame.clearStack();
      frame.push(exType, nullptr);

      computeState(*handlerBb, frame, instList, cf, code, classPath, method);
    }

    bool contains(const CodeAttr::ExceptionHandler &ex, const Inst *inst) {
      int start = ex.startpc->label()->_offset;
      int end = ex.endpc->label()->_offset;
      int off = inst->_offset;

      return start <= off && off < end;
    }

    int s = 0;

    void computeState(
        BasicBlock &bb, Frame &how, InstList &instList, const ClassFile &cf, const CodeAttr *code,
        IClassPath *classPath, Method *method
    ) {
      s++;
      // std::cerr << s << " " << std::flush;
      if (bb.start == instList.end()) {
        JnifError::asserts(
            bb.name == ControlFlowGraph::ExitName && bb.exit == instList.end(), "exit bb"
        );
        s--;
        return;
      }

      JnifError::asserts(how.valid, "how valid");
      // JnifError::asserts(bb.in.valid == bb.out.valid, "in.valid != out.valid");

      Log::Get().Out() << "Computing state for basic block: name=" << bb.name
                       << ", offset=" << bb.start->_offset << std::endl;
      Log::Get().Out() << "  IN State (before merge): " << bb.in << ", valid=" << bb.in.valid
                       << std::endl;

      bool change;
      if (!bb.in.valid) {
        bb.in = how;
        change = true;
      } else {
        change = join(bb.in, how, classPath, method);
      }

      Log::Get().Out() << "  IN State (after merge): " << bb.in << ", valid=" << bb.in.valid
                       << ", changed=" << change << std::endl;

      if (change) {
        Frame out = bb.in;

        SmtBuilder<Frame> builder(out, cf);
        for (InstList::Iterator it = bb.start; it != bb.exit; ++it) {
          Inst *inst = *it;
          builder.exec(*inst);
          // prepareCatchHandlerFrame(inst, out);

          // join(typeFactory, outp, out, classPath, method);

          for (const CodeAttr::ExceptionHandler &ex: code->exceptions) {
            if (contains(ex, inst)) {
              visitCatch(ex, instList, cf, code, classPath, bb.cfg, out, method);
            }
          }
        }

        bb.out = out;
        Frame h = bb.out;

        Log::Get().Out() << "  OUT State: " << bb.out << ", valid=" << bb.out.valid << std::endl;

        for (BasicBlock *nid: bb) {
          computeState(*nid, h, instList, cf, code, classPath, method);
        }
      }
      s--;
    }
  };

  class FrameGenerator {
  public:
    FrameGenerator(ClassFile &cf, IClassPath *classPath) :
        _attrIndex(ConstPool::NULLENTRY), _cf(cf), _classPath(classPath) {}

    void setCpIndex(Type &type, InstList &instList) {
      if (type.isObject()) {
        const string &className = type.getClassName();

        ConstPool::Index index = _cf.putClass(className.c_str());
        type.setCpIndex(index);

        if (!type.init) {
          JnifError::asserts(type.uninit.newinst->isType(), "It is not type");
          LabelInst *l = instList.addLabel(type.uninit.newinst);
          type = TypeFactory::uninitType(-1, l);
        }
      }
    }

    void setCpIndex(Frame &frame, InstList &instList) {
      for (Frame::T &t: frame.lva) {
        setCpIndex(t.first, instList);
      }

      for (Frame::T &t: frame.stack) {
        setCpIndex(t.first, instList);
      }
    }

    void computeFrames(CodeAttr *code, Method *method) {
      Log::Get().Out() << "Computing frames for method: index=" << method
                       << ", name=" << _cf.getUtf8(method->nameIndex) << std::endl;
      for (auto it = code->attrs.begin(); it != code->attrs.end(); it++) {
        Attr *attr = *it;
        if (attr->kind == ATTR_SMT) {
          code->attrs.attrs.erase(it);
          break;
        }
      }

      if (_attrIndex == ConstPool::NULLENTRY) {
        _attrIndex = _cf.putUtf8("StackMapTable");
      }

      Frame initFrame;

      u4 lvindex;
      if (method->isStatic()) {
        lvindex = 0;
      } else {
        //|| method->isClassInit()
        string className = _cf.getThisClassName();
        if (method->isInit()) {
          Type u = TypeFactory::uninitThisType();
          u.init = false;
          u.typeId = Type::nextTypeId;
          u.className = className;
          Type::nextTypeId++;
          initFrame.setVar2(0, u, nullptr);
        } else {
          initFrame.setRefVar(0, className, nullptr);
        }

        lvindex = 1;
      }

      const char *methodDesc = _cf.getUtf8(method->descIndex);
      std::vector<Type> argsType;
      TypeFactory::fromMethodDesc(methodDesc, &argsType);

      for (const Type &t: argsType) {
        initFrame.setVar(&lvindex, t, nullptr);
      }
      Log::Get().Out() << "Frame INIT: " << initFrame << std::endl;

      ControlFlowGraph *cfgp = new ControlFlowGraph(code->instList);
      code->cfg = cfgp;

      ControlFlowGraph &cfg = *cfgp;

      initFrame.valid = true;
      BasicBlock *bbe = cfg.entry;
      bbe->in = initFrame;
      bbe->out = initFrame;

      BasicBlock *to = *cfg.entry->begin();
      ComputeFrames comp;
      comp.computeState(*to, initFrame, code->instList, _cf, code, _classPath, method);

      u4 maxStack = code->maxStack;
      u4 maxLocals = code->maxLocals;
      if (!code->instList.hasBranches() && !code->hasTryCatch()) {
        for (BasicBlock *bb: cfg) {
          if (maxStack < bb->in.maxStack) {
            maxStack = bb->in.maxStack;
          }

          if (maxStack < bb->out.maxStack) {
            maxStack = bb->out.maxStack;
          }

          if (maxLocals < bb->in.lva.size()) {
            maxLocals = bb->in.lva.size();
          }

          if (maxLocals < bb->out.lva.size()) {
            maxLocals = bb->out.lva.size();
          }
        }

        code->maxStack = maxStack;
        code->maxLocals = maxLocals;
        return;
      }

      SmtAttr *smt = new SmtAttr(_attrIndex, &_cf);

      int totalOffset = -1;

      Frame *f = &cfg.entry->out;
      f->cleanTops();

      class Ser {
      public:
        bool isSame(Frame &current, Frame &prev) {
          return current.lva == prev.lva && current.stack.size() == 0;
        }

        bool isSameLocals1StackItem(Frame &current, Frame &prev) {
          return current.lva == prev.lva && current.stack.size() == 1;
        }

        int isChopAppend(Frame &current, Frame &prev) {
          int diff = current.lva.size() - prev.lva.size();

          for (u4 i = 0; i < std::min(current.lva.size(), prev.lva.size()); ++i) {
            if (current.lva.at(i) != prev.lva.at(i)) {
              return 0;
            }
          }

          bool emptyStack = current.stack.size() == 0;
          bool res = diff != 0 && diff >= -3 && diff <= 3 && emptyStack;
          return res ? diff : 0;
        }
      } s;

      for (BasicBlock *bb: cfg) {
        if (maxStack < bb->in.maxStack) {
          maxStack = bb->in.maxStack;
        }

        if (maxStack < bb->out.maxStack) {
          maxStack = bb->out.maxStack;
        }

        if (maxLocals < bb->in.lva.size()) {
          maxLocals = bb->in.lva.size();
        }

        if (maxLocals < bb->out.lva.size()) {
          maxLocals = bb->out.lva.size();
        }

        if (bb->start != code->instList.end()) {
          Inst *start = *bb->start;
          // FIX: It looks like we need a stack map for every label, not only for branch targets.
          // if (start->isLabel() &&
          //     (start->label()->isBranchTarget || start->label()->isCatchHandler)) {
          if (start->isLabel()) {
            Log::Get().Out() << "Generating frames for basic block: name=" << bb->name
                             << ", offset=" << bb->start->label()->offset
                             << ", isBranchTarget=" << start->label()->isBranchTarget
                             << ", isCatchHandler=" << start->label()->isCatchHandler << std::endl;

            Frame &current = bb->in;

            // The current from is not valid. It might be dead code
            // that we missed to analyze.
            if (!current.valid) {
              std::cout
                  << "WARNING: Invalid frame detected. This is perhaps due to the bytecode "
                     "containing dead basic blocks. Replace the invalid frame with Frame INIT. "
                     "However, this is dangerous as the Java bytecode verify does not define "
                     "how to verify dead code. It is undefined behavior."
                  << std::endl;
              current = initFrame;
            }

            current.cleanTops();

            setCpIndex(current, code->instList);

            SmtAttr::Entry e;

            e.label = start;

            totalOffset += 1;
            int offsetDelta = start->label()->offset - totalOffset;
            std::string frameType = "";

            int diff;

            if (s.isSame(current, *f)) {
              if (offsetDelta <= 63) {
                e.frameType = offsetDelta;
                frameType = "same_frame";
              } else {
                e.frameType = 251;
                e.same_frame_extended.offset_delta = offsetDelta;
                frameType = "same_frame_extended";
              }
            } else if (s.isSameLocals1StackItem(current, *f)) {
              if (offsetDelta <= 63) {
                e.frameType = 64 + offsetDelta;
                const Type &t = current.stack.front().first;
                e.sameLocals_1_stack_item_frame.stack.push_back(t);
                frameType = "same_locals_1_stack_item_frame";
              } else {
                e.frameType = 247;
                const Type &t = current.stack.front().first;
                e.same_locals_1_stack_item_frame_extended.stack.push_back(t);
                e.same_locals_1_stack_item_frame_extended.offset_delta = offsetDelta;
                frameType = "same_locals_1_stack_item_frame_extended";
              }
            } else if ((diff = s.isChopAppend(current, *f)) != 0) {
              JnifError::asserts(diff != 0 && diff >= -3 && diff <= 3);

              e.frameType = 251 + diff;
              if (diff > 0) {
                e.append_frame.offset_delta = offsetDelta;
                int size = current.lva.size();
                // list<Type> ts;
                for (int i = 0; i < diff; i++) {
                  Type t = current.lva[size - diff + i].first;
                  // ts.push_front(t);
                  e.append_frame.locals.push_back(t);
                }
                // for (const Type& t : ts) {
                // e.append_frame.locals.push_back(t);
                // }
                // e.append_frame.locals.re
                frameType = "append_frame";
              } else {
                e.chop_frame.offset_delta = offsetDelta;
                frameType = "chop_frame";
              }
            } else {
              e.frameType = 255;
              e.full_frame.offset_delta = offsetDelta;
              std::vector<Type> lva(current.lva.size(), TypeFactory::topType());
              for (size_t i = 0; i < current.lva.size(); i++) {
                lva[i] = current.lva[i].first;
              }

              e.full_frame.locals = lva;

              std::list<Frame::T> rs = current.stack;
              rs.reverse();
              for (const Frame::T &t: rs) {
                e.full_frame.stack.push_back(t.first);
                // e.full_frame.stack.push_front(t);
              }

              frameType = "full_frame";
            }

            Log::Get().Out() << "Frame " << "@" << start->label()->offset << ": type=" << frameType
                             << ", " << current << std::endl;

            totalOffset += offsetDelta;
            smt->entries.push_back(e);
            f = &bb->in;
          }
        }
      }

      code->attrs.add(smt);

      code->maxStack = maxStack;
      code->maxLocals = maxLocals;
    }

  private:
    ConstPool::Index _attrIndex;
    ClassFile &_cf;
    IClassPath *_classPath;
  };

  static void setLink(const std::set<Inst *> &is, Inst *j) {
    JnifError::asserts(j != nullptr, "j cannot be null");
    for (Inst *i: is) {
      JnifError::asserts(i != nullptr, "i cannot be null");
      j->consumes.insert(i);
      i->produces.insert(j);
    }
  }

  Type Frame::getVar(u4 lvindex, Inst *inst) {
    JnifError::asserts(inst != nullptr, "Inst cannot be null for getVar");

    const T *t = &lva.at(lvindex);
    setLink(t->second, inst);

    return t->first;
  }

  Type Frame::pop(Inst *inst) {
    JnifError::checks(stack.size() > 0, "Trying to pop in an empty stack.");
    JnifError::asserts(inst != nullptr, "Inst cannot be null");

    const T *t = &stack.front();
    setLink(t->second, inst);
    Type ret = t->first;
    stack.pop_front();

    return ret;
  }

  Type Frame::popOneWord(Inst *inst) {
    Type t = pop(inst);
    JnifError::checks(
        t.isOneWord() || t.isTop(), "Type is not one word type: ", t, ", frame: ", *this
    );
    return t;
  }

  Type Frame::popTwoWord(Inst *inst) {
    Type t1 = pop(inst);
    Type t2 = pop(inst);

    JnifError::checks(
        (t1.isOneWord() && t2.isOneWord()) || (t1.isTwoWord() && t2.isTop()),
        "Invalid types on top of the stack for pop2: ", t1, t2, *this
    );
    // Error::check(t2.isTop(), "Type is not Top type: ", t2, t1, *this);

    return t1;
  }

  Type Frame::popIntegral(Inst *inst) {
    Type t = popOneWord(inst);
    JnifError::asserts(t.isIntegral(), "Invalid integral type on top of stack: ", t);
    return t;
  }

  Type Frame::popFloat(Inst *inst) {
    Type t = popOneWord(inst);
    JnifError::asserts(t.isFloat(), "invalid float type on top of the stack");
    return t;
  }

  Type Frame::popLong(Inst *inst) {
    Type t = popTwoWord(inst);
    JnifError::checks(t.isLong(), "invalid long type on top of the stack");
    return t;
  }

  Type Frame::popDouble(Inst *inst) {
    Type t = popTwoWord(inst);
    JnifError::checks(t.isDouble(), "Invalid double type on top of the stack: ", t);

    return t;
  }

  void Frame::popType(const Type &type, Inst *inst) {
    if (type.isIntegral()) {
      popIntegral(inst);
    } else if (type.isFloat()) {
      popFloat(inst);
    } else if (type.isLong()) {
      popLong(inst);
    } else if (type.isDouble()) {
      popDouble(inst);
    } else if (type.isObject()) {
      popRef(inst);
    } else {
      throw Exception("invalid pop type: ", type);
    }
  }

  void Frame::push(const Type &t, Inst *inst) {
    std::set<Inst *> ls;
    if (inst != nullptr) {
      ls.insert(inst);
    }

    stack.push_front(std::make_pair(t, ls));

    if (maxStack < stack.size()) {
      JnifError::asserts(maxStack + 1 == stack.size(), "Invalid inc maxStack/size");
      maxStack++;
    }
  }

  void Frame::pushType(const Type &type, Inst *inst) {
    if (type.isIntegral()) {
      pushInt(inst);
    } else if (type.isFloat()) {
      pushFloat(inst);
    } else if (type.isLong()) {
      pushLong(inst);
    } else if (type.isDouble()) {
      pushDouble(inst);
    } else if (type.isNull()) {
      pushNull(inst);
    } else if (type.isObject() || type.isUninitThis()) {
      push(type, inst);
    } else {
      throw Exception("invalid push type: ", type);
    }
  }

  void Frame::setVar(u4 *lvindex, const Type &t, Inst *inst) {
    JnifError::asserts(t.isOneOrTwoWord(), "Setting var on non one-two word uninit this");

    if (t.isOneWord()) {
      _setVar(*lvindex, t, inst);
      (*lvindex)++;
    } else {
      _setVar(*lvindex, t, inst);
      _setVar(*lvindex + 1, TypeFactory::topType(), inst);
      (*lvindex) += 2;
    }
  }

  void Frame::setVar2(u4 lvindex, const Type &t, Inst *inst) { setVar(&lvindex, t, inst); }

  void Frame::setRefVar(u4 lvindex, const Type &type, Inst *inst) {
    JnifError::checks(
        type.isObject() || type.isNull() || type.isUninitThis(), "Type must be object type: ", type
    );
    setVar(&lvindex, type, inst);
  }

  void Frame::cleanTops() {
    JnifError::asserts(!topsErased, "tops already erased: ", topsErased);

    for (u4 i = 0; i < lva.size(); i++) {
      T t = lva[i];
      if (t.first.isTwoWord()) {
        T u = lva[i + 1];
        Type top = u.first;

        // workaround!!!
        if (top.isTop()) {
          JnifError::asserts(
              top.isTop(), "Not top for two word: index: ", i, ", top: ", top, " for ", t.first,
              " in ", *this
          );
          lva.erase(lva.begin() + i + 1);
        }
      }
    }

    for (int i = lva.size() - 1; i >= 0; i--) {
      T t = lva[i];
      if (t.first.isTop()) {

        lva.erase(lva.begin() + i);
      } else {
        return;
      }
    }
  }

  void Frame::join(Frame &how, IClassPath *classPath) {
    ComputeFrames().join(*this, how, classPath);
  }

  void Frame::init(const Type &t) {
    for (Frame::T &p: lva) {
      Type &tr = p.first;
      if (tr.typeId == t.typeId) {
        //						JnifError::checks(!tr.init,
        //								"Object is already init in lva: ", tr, ", ",
        //								className, ".", name, desc, ", ", t);
        JnifError::checks(tr.className != "", "empty clsname lva");
        JnifError::checks(
            tr.className == t.className, "!= clsname lva", tr.className, " !=! ", t.className
        );

        tr.init = true;
        tr.tag = TYPE_OBJECT;
      }
    }

    for (Frame::T &p: stack) {
      Type &tr = p.first;
      if (tr.typeId == t.typeId) {
        //						JnifError::checks(!tr.init,
        //								"Object is already init in stack: ", tr, ", ",
        //								className, ".", name, desc, ", ", t);
        JnifError::checks(tr.className != "", "empty clsname stack");
        JnifError::checks(tr.className == t.className, "!= clsname stack");

        tr.init = true;
        tr.tag = TYPE_OBJECT;
      }
    }
  }

  void Frame::_setVar(u4 lvindex, const Type &t, Inst *inst) {
    JnifError::checks(lvindex < 256 * 256, "Index too large for LVA: ", lvindex);

    if (lvindex >= lva.size()) {
      lva.resize(lvindex + 1, std::make_pair(TypeFactory::topType(), std::set<Inst *>()));
    }

    std::set<Inst *> ls;
    if (inst != nullptr) {
      ls.insert(inst);
    }

    lva[lvindex] = std::make_pair(t, ls);
  }

  namespace model {


    void ClassFile::computeFrames(IClassPath *classPath) {
      computeSize(); // Fix every existing attributes

      FrameGenerator fg(*this, classPath);

      for (Method &method: methods) {
        CodeAttr *code = method.codeAttr();

        if (code != nullptr) {
          bool hasJsrOrRet = code->instList.hasJsrOrRet();
          if (hasJsrOrRet) {
            throw Exception(
                "JSR or RET have been deprecated since Java 7; do not use them in your code"
            );
          }

          fg.computeFrames(code, &method);
        }
      }

      computeSize(); // Since we may add new attributes, fix them again
    }

  } // namespace model
} // namespace jnif
