/*
 * printer.cpp
 *
 *  Created on: Apr 4, 2014
 *      Author: luigi
 */
#include "jnif/jnif.hpp"

#include <iomanip>

using namespace std;

static constexpr const char *reset = "\x1b[0m";
static constexpr const char *red = "\x1b[31m";
static constexpr const char *green = "\x1b[32m";
static constexpr const char *yellow = "\x1b[33m";
static constexpr const char *blue = "\x1b[34m";
static constexpr const char *magenta = "\x1b[35m";
static constexpr const char *cyan = "\x1b[36m";

static constexpr const char *cpindex = magenta;
static constexpr const char *flags = cpindex;
static constexpr const char *keyword = blue;

namespace jnif {


  std::ostream &operator<<(std::ostream &os, const Exception &ex) {
    os << "Error: JNIF Exception: " << ex.message << " @ " << endl;
    os << ex.stackTrace;

    return os;
  }

  const char *yesNo(bool value, const char *str) { return value ? str : ""; }

  std::ostream &operator<<(std::ostream &os, const Frame &frame) {
    os << "{";
    for (u4 i = 0; i < frame.lva.size(); i++) {
      os << (i == 0 ? "" : " ") << i << ":" << frame.lva[i].first;
    }
    os << "}[";
    int i = 0;
    for (auto t: frame.stack) {
      os << (i == 0 ? "" : " | ") << t.first;
      i++;
    }
    return os << "]";
  }

  std::ostream &operator<<(std::ostream &os, BasicBlock &bb) {
    os << "    " << yellow << bb.name << reset;

    auto p = [&os](const std::vector<BasicBlock *> &bs, const char *arrow) {
      os << "{";
      bool f = true;
      for (BasicBlock *bbt: bs) {
        if (!f) {
          os << " ";
        }
        os << arrow << bbt->name;
        f = false;
      }
      os << "}";
    };

    os << " ";
    p(bb.targets, "->");
    os << " ";
    p(bb.ins, "<-");

    os << " " << bb.in << " ~> " << bb.out;

    InstList::Iterator it = bb.start;

    if (it == bb.exit) {
      os << endl;
      return os;
    }

    Inst &first = **it;
    if (first.kind == KIND_LABEL && first.label()->isTarget()) {
      os << "L" << first.label()->id << " " << yesNo(first.label()->isBranchTarget, "B") << " "
         << yesNo(first.label()->isTryStart, "TS")
         << " "
         // << yesNo(first.label()->isTryEnd, "TE") << " "
         << yesNo(first.label()->isCatchHandler, "C");
    }

    os << endl;

    for (; it != bb.exit; ++it) {
      Inst &inst = **it;

      if (inst.kind == KIND_LABEL) {
        continue;
      }

      os << inst;
      if (bb.last == *it) {
        os << " <--";
      }

      os << endl;
    }

    return os;
  }

  std::ostream &operator<<(std::ostream &os, const ControlFlowGraph &cfg) {
    for (BasicBlock *bb: cfg) {
      os << *bb;
    }

    os << endl;

    return os;
  }

} // namespace jnif

namespace jnif {
  namespace model {

    const char *ConstNames[] = {
        "**** 0 ****",        // 0
        "Utf8",               // 1
        "**** 2 ****",        // 2
        "Integer",            // 3
        "Float",              // 4
        "Long",               // 5
        "Double",             // 6
        "Class",              // 7
        "String",             // 8
        "Fieldref",           // 9
        "Methodref",          // 10
        "InterfaceMethodref", // 11
        "NameAndType",        // 12
        "**** 13 ****",       // 13
        "**** 14 ****",       // 14
        "MethodHandle",       // 15
        "MethodType",         // 16
        "**** 17 ****",       // 17
        "InvokeDynamic",      // 18
    };

    const char *OPCODES[] = {"nop",           "aconst_null",  "iconst_m1",
                             "iconst_0",      "iconst_1",     "iconst_2",
                             "iconst_3",      "iconst_4",     "iconst_5",
                             "lconst_0",      "lconst_1",     "fconst_0",
                             "fconst_1",      "fconst_2",     "dconst_0",
                             "dconst_1",      "bipush",       "sipush",
                             "ldc",           "ldc_w",        "ldc2_w",
                             "iload",         "lload",        "fload",
                             "dload",         "aload",        "iload_0",
                             "iload_1",       "iload_2",      "iload_3",
                             "lload_0",       "lload_1",      "lload_2",
                             "lload_3",       "fload_0",      "fload_1",
                             "fload_2",       "fload_3",      "dload_0",
                             "dload_1",       "dload_2",      "dload_3",
                             "aload_0",       "aload_1",      "aload_2",
                             "aload_3",       "iaload",       "laload",
                             "faload",        "daload",       "aaload",
                             "baload",        "caload",       "saload",
                             "istore",        "lstore",       "fstore",
                             "dstore",        "astore",       "istore_0",
                             "istore_1",      "istore_2",     "istore_3",
                             "lstore_0",      "lstore_1",     "lstore_2",
                             "lstore_3",      "fstore_0",     "fstore_1",
                             "fstore_2",      "fstore_3",     "dstore_0",
                             "dstore_1",      "dstore_2",     "dstore_3",
                             "astore_0",      "astore_1",     "astore_2",
                             "astore_3",      "iastore",      "lastore",
                             "fastore",       "dastore",      "aastore",
                             "bastore",       "castore",      "sastore",
                             "pop",           "pop2",         "dup",
                             "dup_x1",        "dup_x2",       "dup2",
                             "dup2_x1",       "dup2_x2",      "swap",
                             "iadd",          "ladd",         "fadd",
                             "dadd",          "isub",         "lsub",
                             "fsub",          "dsub",         "imul",
                             "lmul",          "fmul",         "dmul",
                             "idiv",          "ldiv",         "fdiv",
                             "ddiv",          "irem",         "lrem",
                             "frem",          "drem",         "ineg",
                             "lneg",          "fneg",         "dneg",
                             "ishl",          "lshl",         "ishr",
                             "lshr",          "iushr",        "lushr",
                             "iand",          "land",         "ior",
                             "lor",           "ixor",         "lxor",
                             "iinc",          "i2l",          "i2f",
                             "i2d",           "l2i",          "l2f",
                             "l2d",           "f2i",          "f2l",
                             "f2d",           "d2i",          "d2l",
                             "d2f",           "i2b",          "i2c",
                             "i2s",           "lcmp",         "fcmpl",
                             "fcmpg",         "dcmpl",        "dcmpg",
                             "ifeq",          "ifne",         "iflt",
                             "ifge",          "ifgt",         "ifle",
                             "if_icmpeq",     "if_icmpne",    "if_icmplt",
                             "if_icmpge",     "if_icmpgt",    "if_icmple",
                             "if_acmpeq",     "if_acmpne",    "goto",
                             "jsr",           "ret",          "tableswitch",
                             "lookupswitch",  "ireturn",      "lreturn",
                             "freturn",       "dreturn",      "areturn",
                             "return",        "getstatic",    "putstatic",
                             "getfield",      "putfield",     "invokevirtual",
                             "invokespecial", "invokestatic", "invokeinterface",
                             "invokedynamic", "new",          "newarray",
                             "anewarray",     "arraylength",  "athrow",
                             "checkcast",     "instanceof",   "monitorenter",
                             "monitorexit",   "wide",         "multianewarray",
                             "ifnull",        "ifnonnull",    "goto_w",
                             "jsr_w",         "breakpoint",   "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "RESERVED",
                             "RESERVED",      "RESERVED",     "impdep1",
                             "impdep2"};

    std::ostream &operator<<(std::ostream &os, const Version &version) {
      os << version.majorVersion() << "." << version.minorVersion();
      os << " (supported by JDK " << version.supportedByJdk() << ")";
      return os;
    }

    std::ostream &operator<<(std::ostream &os, const ConstPool::Tag &tag) {
      return os << ConstNames[tag];
    }

    ostream &operator<<(ostream &os, Opcode opcode) {
      //	JnifError::asserts(opcode >= 0, "");

      os << OPCODES[(int) opcode];
      return os;
    }

    std::ostream &operator<<(std::ostream &os, const Inst &inst) {
      int offset = inst._offset;

      if (inst.kind == KIND_LABEL) {
        if (inst.label()->isTarget()) {
          os << "label " << inst.label()->id << "" << yesNo(inst.label()->isBranchTarget, "B")
             << " " << yesNo(inst.label()->isTryStart, "TS")
             << " "
             // << yesNo(inst.label()->isTryEnd, "TE") << "  "
             << yesNo(inst.label()->isCatchHandler, "C");
        }
        return os;
      }

      os << "    " << setw(4) << offset << ": "; // << "#" << setw(2) << inst.id << ": ";
      os << green << "(" << setw(3) << (int) inst.opcode << ") " << reset;
      os << cyan << OPCODES[(int) inst.opcode] << reset << " ";

      os << "CS: ";
      for (const Inst *ii: inst.consumes) {
        os << ii->_offset << " ";
      }
      os << "PS: ";
      for (const Inst *ii: inst.produces) {
        os << ii->_offset << " ";
      }

      const ConstPool &cf = *inst.constPool;

      switch (inst.kind) {
        case KIND_ZERO:
          break;
        case KIND_BIPUSH:
          os << int(inst.push()->value);
          break;
        case KIND_SIPUSH:
          os << int(inst.push()->value);
          break;
        case KIND_LDC:
          os << "#" << int(inst.ldc()->valueIndex);
          break;
        case KIND_VAR:
          os << int(inst.var()->lvindex);
          break;
        case KIND_IINC:
          os << int(inst.iinc()->index) << " " << int(inst.iinc()->value);
          break;
        case KIND_JUMP:
          os << "label: " << inst.jump()->label2->label()->id;
          break;
        case KIND_TABLESWITCH:
          os << "default: " << inst.ts()->def->label()->id << ", from: " << inst.ts()->low << " "
             << inst.ts()->high << ":";

          for (int i = 0; i < inst.ts()->high - inst.ts()->low + 1; i++) {
            Inst *l = inst.ts()->targets[i];
            os << " " << l->label()->id;
          }
          break;
        case KIND_LOOKUPSWITCH:
          os << inst.ls()->defbyte->label()->id << " " << inst.ls()->npairs << ":";

          for (u4 i = 0; i < inst.ls()->npairs; i++) {
            u4 k = inst.ls()->keys[i];
            Inst *l = inst.ls()->targets[i];
            os << " " << k << " -> " << l->label()->id;
          }
          break;
        case KIND_FIELD: {
          string className, name, desc;
          cf.getFieldRef(inst.field()->fieldRefIndex, &className, &name, &desc);

          os << className << name << desc;
          break;
        }
        case KIND_INVOKE: {
          ConstPool::Index mid = inst.invoke()->methodRefIndex;
          os << "#" << mid << " ";

          string className, name, desc;
          // New in Java 8, invokespecial can be either a method ref or
          // inter method ref.
          if (cf.getTag(mid) == ConstPool::INTERMETHODREF) {
            cf.getInterMethodRef(inst.invoke()->methodRefIndex, &className, &name, &desc);
          } else {
            cf.getMethodRef(inst.invoke()->methodRefIndex, &className, &name, &desc);
          }

          os << className << "." << name << ": " << desc;
          break;
        }
        case KIND_INVOKEINTERFACE: {
          string className, name, desc;
          cf.getInterMethodRef(
              inst.invokeinterface()->interMethodRefIndex, &className, &name, &desc
          );

          os << className << "." << name << ": " << desc << "("
             << int(inst.invokeinterface()->count) << ")";
          break;
        }
        case KIND_INVOKEDYNAMIC: {
          os << int(inst.indy()->callSite()) << "";
          break;
        }

        case KIND_TYPE: {
          int ci = inst.type()->classIndex;
          string className = cf.getClassName(ci);
          os << className << cpindex << "#" << ci << reset;
          break;
        }
        case KIND_NEWARRAY:
          os << int(inst.newarray()->atype);
          break;
        case KIND_MULTIARRAY: {
          string className = cf.getClassName(inst.multiarray()->classIndex);
          os << className << " " << inst.multiarray()->dims;
          break;
        }
        case KIND_PARSE4TODO:
          throw Exception("FrParse4__TODO__Instr not implemented");
          break;
        case KIND_RESERVED:
          throw Exception("FrParseReservedInstr not implemented");
          break;
        case KIND_FRAME:
          //	os << "Frame " << inst.frame.frame;
          break;
        default:
          throw Exception("print inst: unknown inst kind!");
      }

      return os;
    }

    std::ostream &operator<<(std::ostream &os, const InstList &instList) {
      for (Inst *inst: instList) {
        os << *inst << endl;
      }

      return os;
    }

    std::ostream &operator<<(std::ostream &os, const Type &type) {
      for (u4 i = 0; i < type.getDims(); i++) {
        os << "[]";
      }

      if (type.isTop()) {
        os << "Top";
      } else if (type.isIntegral()) {
        os << "I";
      } else if (type.isFloat()) {
        os << "F";
      } else if (type.isLong()) {
        os << "L";
      } else if (type.isDouble()) {
        os << "D";
      } else if (type.isNull()) {
        os << "Null";
      } else if (type.isUninitThis()) {
        os << "Uninitialized this:" << type.className;
      } else if (type.isObject()) {
        string s = type.getClassName();
        size_t i = s.find_last_of('/');
        os << (i != string::npos ? s.substr(i + 1) : s) << "";
      } else if (type.isUninit()) {
        u2 offset = type.uninit.label->label()->offset;
        os << "Uninitialized offset: new offset=" << type.uninit.offset
           << ", delta offset=" << offset;
      } else {
        os << "UNKNOWN TYPE!!!";

        throw Exception("Invalid type on write: ", type);
      }

      return os;
    }

    class AccessFlagsPrinter {
    public:
      AccessFlagsPrinter(u2 value, const char *sep = " ") : value(value), sep(sep) {}

      static void check(
          Method::Flags accessFlags, const char *name, ostream &out, AccessFlagsPrinter self,
          bool &empty
      ) {
        if (self.value & accessFlags) {
          out << keyword << (empty ? "" : self.sep) << name << reset;
          empty = false;
        }
      }

      friend ostream &operator<<(ostream &out, AccessFlagsPrinter self) {
        bool empty = true;

        check(Method::PUBLIC, "public", out, self, empty);
        check(Method::PRIVATE, "private", out, self, empty);
        check(Method::PROTECTED, "protected", out, self, empty);
        check(Method::STATIC, "static", out, self, empty);
        check(Method::FINAL, "final", out, self, empty);
        check(Method::SYNCHRONIZED, "synchronized", out, self, empty);
        check(Method::BRIDGE, "bridge", out, self, empty);
        check(Method::VARARGS, "varargs", out, self, empty);
        check(Method::NATIVE, "native", out, self, empty);
        check(Method::ABSTRACT, "abstract", out, self, empty);
        check(Method::STRICT, "strict", out, self, empty);
        check(Method::SYNTHETIC, "synthetic", out, self, empty);

        out << flags << "|" << self.value << reset;

        return out;
      }

    private:
      const u2 value;
      const char *const sep;
    };

    std::ostream &operator<<(std::ostream &os, const Method &m) {
      const ConstPool &cp = m.constPool;
      os << AccessFlagsPrinter(m.accessFlags) << " ";
      os << cp.getUtf8(m.nameIndex) << cpindex << "#" << m.nameIndex << reset << "";
      os << cp.getUtf8(m.descIndex) << cpindex << "#" << m.descIndex << reset << endl;

      if (m.hasCode()) {
        CodeAttr *c = m.codeAttr();
        if (c->cfg != NULL) {
          os << *c->cfg;
        } else {
          os << c->instList;
        }
      }

      return os;
    }

    class ClassPrinter : private Error<Exception> {
    public:
      ClassPrinter(const ClassFile &cf, ostream &os, int tabs) : cf(cf), os(os), tabs(tabs) {}

      void print() {
        line() << AccessFlagsPrinter(cf.accessFlags) << " class ";
        os << cf.getThisClassName() << cpindex << "#" << cf.thisClassIndex << reset;
        if (cf.superClassIndex != 0) {
          os << keyword << " extends " << reset;
          os << cf.getClassName(cf.superClassIndex) << cpindex << "#" << cf.superClassIndex
             << reset;
        } else {
          os << cpindex << "#" << cf.superClassIndex << reset;
        }
        os << endl;

        inc();
        line() << "* Version: " << cf.version << endl;

        line() << "* Constant Pool [" << ((ConstPool &) cf).size() << "]" << endl;
        inc();
        printConstPool(cf);
        dec();

        // line() << "* accessFlags: " << AccessFlagsPrinter(cf.accessFlags)
        // << endl;
        // line() << "* thisClassIndex: " << cf.getThisClassName() << "#"
        // << cf.thisClassIndex << endl;


        line() << "* Interfaces [" << cf.interfaces.size() << "]" << endl;
        inc();
        for (u2 interIndex: cf.interfaces) {
          line() << "Interface '" << cf.getClassName(interIndex) << "'#" << interIndex << endl;
        }
        dec();

        line() << "* Fields [" << cf.fields.size() << "]" << endl;
        inc();
        for (const Field &f: cf.fields) {
          line() << "Field " << cf.getUtf8(f.nameIndex) << ": " << AccessFlagsPrinter(f.accessFlags)
                 << " #" << f.nameIndex << ": " << cf.getUtf8(f.descIndex) << "#" << f.descIndex
                 << endl;

          printAttrs(f.attrs);
        }
        dec();

        // line() << "* Methods [" << cf.methods.size() << "]" << endl;
        // inc();

        for (const Method &m: cf.methods) {
          line() << m;

          printAttrs(m.attrs, (void *) &m);
        }
        // dec();

        os << "--- class ----" << endl;
        printAttrs(cf.attrs);

        dec();
      }

    private:
      // static const char* ConstNames[];

      void printConstPool(const ConstPool &cp) {
        line() << "#0 [null entry]: -" << endl;

        for (ConstPool::Iterator it = cp.iterator(); it.hasNext(); it++) {
          ConstPool::Index i = *it;
          ConstPool::Tag tag = cp.getTag(i);

          line() << "#" << i << " [" << ConstNames[tag] << "]: ";

          const ConstPool::Item *entry = &cp.entries[i];

          switch (tag) {
            case ConstPool::CLASS:
              os << cp.getClassName(i) << "#" << entry->clazz.nameIndex;
              break;
            case ConstPool::FIELDREF: {
              string clazzName, name, desc;
              cp.getFieldRef(i, &clazzName, &name, &desc);

              os << clazzName << "#" << entry->memberRef.classIndex << "." << name << ":" << desc
                 << "#" << entry->memberRef.nameAndTypeIndex;
              break;
            }

            case ConstPool::METHODREF: {
              string clazzName, name, desc;
              cp.getMethodRef(i, &clazzName, &name, &desc);

              os << clazzName << "#" << entry->memberRef.classIndex << "." << name << ":" << desc
                 << "#" << entry->memberRef.nameAndTypeIndex;
              break;
            }

            case ConstPool::INTERMETHODREF: {
              string clazzName, name, desc;
              cp.getInterMethodRef(i, &clazzName, &name, &desc);

              os << clazzName << "#" << entry->memberRef.classIndex << "." << name << ":" << desc
                 << "#" << entry->memberRef.nameAndTypeIndex;
              break;
            }
            case ConstPool::STRING:
              os << cp.getUtf8(entry->s.stringIndex) << "#" << entry->s.stringIndex;
              break;
            case ConstPool::INTEGER:
              os << entry->i.value;
              break;
            case ConstPool::FLOAT:
              os << entry->f.value;
              break;
            case ConstPool::LONG:
              os << cp.getLong(i);
              // i++;
              break;
            case ConstPool::DOUBLE:
              os << cp.getDouble(i);
              // i++;
              break;
            case ConstPool::NAMEANDTYPE:
              os << "#" << entry->nameAndType.nameIndex << ".#"
                 << entry->nameAndType.descriptorIndex;
              break;
            case ConstPool::UTF8:
              os << entry->utf8.str;
              break;
            case ConstPool::METHODHANDLE:
              os << entry->methodHandle.referenceKind << " #" << entry->methodHandle.referenceIndex;
              break;
            case ConstPool::METHODTYPE:
              os << "#" << entry->methodType.descriptorIndex;
              break;
            case ConstPool::INVOKEDYNAMIC:
              os << "#" << entry->invokeDynamic.bootstrapMethodAttrIndex << ".#"
                 << entry->invokeDynamic.nameAndTypeIndex;
              break;
            default:
              throw Exception("invalid tag in printer!");
          }

          os << endl;
        }
      }

      void printAttrs(const Attrs &attrs, void * = NULL) {
        for (Attr *attrp: attrs) {
          Attr &attr = *attrp;

          switch (attr.kind) {
            case ATTR_UNKNOWN:
              printUnknown((UnknownAttr &) attr);
              break;
            case ATTR_SOURCEFILE:
              printSourceFile((SourceFileAttr &) attr);
              break;
            case ATTR_SIGNATURE:
              printSignature((SignatureAttr &) attr);
              break;
            case ATTR_CODE:
              // printCode((CodeAttr&) attr, (Method*) args);
              // JnifError::raise("unreachable code");
              break;
            case ATTR_EXCEPTIONS:
              printExceptions((ExceptionsAttr &) attr);
              break;
            case ATTR_LVT:
              printLvt((LvtAttr &) attr);
              break;
            case ATTR_LVTT:
              printLvt((LvtAttr &) attr);
              break;
            case ATTR_LNT:
              printLnt((LntAttr &) attr);
              break;
            case ATTR_SMT:
              printSmt((SmtAttr &) attr);
              break;
          }
        }
      }

      void printSourceFile(SourceFileAttr &attr) {
        line() << "Source file: " << attr.sourceFile() << "#" << attr.sourceFileIndex << endl;
      }

      void printSignature(SignatureAttr &attr) {
        line() << "Signature: " << attr.signature() << "#" << attr.signatureIndex << endl;
      }

      void printUnknown(UnknownAttr &attr) {
        const string &attrName = cf.getUtf8(attr.nameIndex);

        line() << "  Attribute unknown '" << attrName << "' # " << attr.nameIndex << "[" << attr.len
               << "]" << endl;
      }

      void printCode(CodeAttr &c, Method *) {
        line(1) << "maxStack: " << c.maxStack << ", maxLocals: " << c.maxLocals << endl;

        line(1) << "Code length: " << c.codeLen << endl;

        inc();

        if (c.cfg != NULL) {
          os << *c.cfg;
        } else {
          os << c.instList;
        }

        for (const CodeAttr::ExceptionHandler &e: c.exceptions) {
          line(1) << "exception entry: startpc: " << e.startpc->label()->id
                  << ", endpc: " << e.endpc->label()->id
                  << ", handlerpc: " << e.handlerpc->label()->id << ", catchtype: " << e.catchtype
                  << endl;
        }

        printAttrs(c.attrs);

        dec();
      }

      void printExceptions(ExceptionsAttr &attr) {
        for (u4 i = 0; i < attr.es.size(); i++) {
          u2 exceptionIndex = attr.es[i];

          const string &exceptionName = cf.getClassName(exceptionIndex);

          line() << "  Exceptions entry: " << red << exceptionName << reset << "'#"
                 << exceptionIndex << endl;
        }
      }

      void printLnt(LntAttr &attr) {
        for (LntAttr::LnEntry e: attr.lnt) {
          line() << "  LocalNumberTable entry: startpc: " << e.startpc << ", lineno: " << e.lineno
                 << endl;
        }
      }

      void printLvt(LvtAttr &attr) {
        for (LvtAttr::LvEntry e: attr.lvt) {
          line() << "  LocalVariable(or Type)Table  entry: start: " << e.startPc
                 << ", len: " << e.len << ", varNameIndex: " << e.varNameIndex
                 << ", varDescIndex: " << e.varDescIndex << ", index: " << endl;
        }
      }

      void parseTs(vector<Type> &locs) {
        line(2) << "[" << locs.size() << "] ";
        for (u1 i = 0; i < locs.size(); i++) {
          Type &vt = locs[i];

          os << vt << " | ";
        }

        os << endl;
      }

      void printSmt(SmtAttr &smt) {

        line() << "Stack Map Table: " << endl;

        int toff = -1;
        for (SmtAttr::Entry &e: smt.entries) {
          line(1) << "frame type (" << e.frameType << ") ";

          u1 frameType = e.frameType;

          if (0 <= frameType && frameType <= 63) {
            toff += frameType + 1;
            os << "offset = " << toff << " ";

            os << "same frame" << endl;
          } else if (64 <= frameType && frameType <= 127) {
            toff += frameType - 64 + 1;
            os << "offset = " << toff << " ";

            os << "sameLocals_1_stack_item_frame. ";
            parseTs(e.sameLocals_1_stack_item_frame.stack);
          } else if (frameType == 247) {
            toff += e.same_locals_1_stack_item_frame_extended.offset_delta + 1;
            os << "offset = " << toff << " ";

            os << "same_locals_1_stack_item_frame_extended. ";
            os << e.same_locals_1_stack_item_frame_extended.offset_delta << endl;
            parseTs(e.same_locals_1_stack_item_frame_extended.stack);
          } else if (248 <= frameType && frameType <= 250) {
            toff += e.chop_frame.offset_delta + 1;
            os << "offset = " << toff << " ";

            os << "chop_frame, ";
            os << "offset_delta = " << e.chop_frame.offset_delta << endl;
          } else if (frameType == 251) {
            toff += e.same_frame_extended.offset_delta + 1;
            os << "offset = " << toff << " ";

            os << "same_frame_extended. ";
            os << e.same_frame_extended.offset_delta << endl;
          } else if (252 <= frameType && frameType <= 254) {
            toff += e.append_frame.offset_delta + 1;
            os << "offset = " << toff << " ";

            os << "append_frame, ";
            os << "offset_delta = " << e.append_frame.offset_delta << endl;
            parseTs(e.append_frame.locals);
          } else if (frameType == 255) {
            toff += e.full_frame.offset_delta + 1;
            os << "offset = " << toff << " ";

            os << "full_frame. ";
            os << e.full_frame.offset_delta << endl;
            parseTs(e.full_frame.locals);
            parseTs(e.full_frame.stack);
          }
        }
      }

    private:
      const ClassFile &cf;

      ostream &os;

      int tabs;

      inline void inc() { tabs++; }

      inline void dec() { tabs--; }

      inline ostream &line(int moretabs = 0) {
        for (int i = 0; i < tabs + moretabs; i++) {
          os << "  ";
        }

        return os;
      }
    };

    ostream &operator<<(ostream &os, const ClassFile &cf) {
      ClassPrinter cp(cf, os, 0);
      cp.print();

      return os;
    }


  } // namespace model
} // namespace jnif
