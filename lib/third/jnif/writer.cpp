/*
 * writer.cpp
 *
 *  Created on: Apr 4, 2014
 *      Author: luigi
 */
#include <fstream>
#include "jnif/jnif.hpp"

using namespace std;

namespace jnif {

  /**
   * Implements an no-op writer that only counts the size of the written data.
   */
  class SizeWriter {
  public:
    inline SizeWriter() : offset(0) {}

    inline void writeu1(u1) { offset += 1; }

    inline void writeu2(u2) { offset += 2; }

    inline void writeu4(u4) { offset += 4; }

    inline void writecount(const void *, int count) { offset += count; }

    inline int getOffset() const { return offset; }

  private:
    int offset;
  };

  /**
   * Implements a memory buffer writer in big-endian encoding.
   */
  class BufferWriter {
  public:
    BufferWriter(u1 *buffer, int len) : buffer(buffer), len(len), offset(0) {}

    ~BufferWriter() {
      // Error::check(offset == len, "Expected end of buffer", offset, len);
    }

    void writeu1(u1 value) {
      JnifError::checks(offset + 1 <= len, "Invalid write: offset: ", offset + 1, ", len: ", len);

      buffer[offset] = value;

      offset += 1;
    }

    void writeu2(u2 value) {
      JnifError::checks(
          offset + 2 <= len, "Invalid write: offset: ", offset + 2, ", len: ", len,
          ", value: ", value
      );

      buffer[offset + 0] = ((u1 *) &value)[1];
      buffer[offset + 1] = ((u1 *) &value)[0];

      offset += 2;
    }

    void writeu4(u4 value) {
      JnifError::checks(offset + 4 <= len, "Invalid write: offset: ", offset + 4, ", len: ", len);

      buffer[offset + 0] = ((u1 *) &value)[3];
      buffer[offset + 1] = ((u1 *) &value)[2];
      buffer[offset + 2] = ((u1 *) &value)[1];
      buffer[offset + 3] = ((u1 *) &value)[0];

      offset += 4;
    }

    void writecount(const void *source, int count) {
      JnifError::checks(offset + count <= len, "Invalid write count");

      copy((u1 *) source, (u1 *) source + count, buffer + offset);

      offset += count;
    }

    int getOffset() const { return offset; }

  private:
    u1 *const buffer;
    const int len;
    int offset;
  };

  /**
   * Implements a file writer in big-endian encoding.
   */
  class FileWriter {
  public:
    explicit FileWriter(std::string file) : out(file, std::ostream::binary), offset(0) {}

    ~FileWriter() { out.close(); }

    void writeu1(u1 value) {
      const char a[1] = {static_cast<char>(value)};
      out.write(a, 1);
      offset += 1;
    }

    void writeu2(u2 value) {
      writeu1(static_cast<u1>((value >> 8) & 0xff));
      writeu1(static_cast<u1>((value >> 0) & 0xff));
      offset += 2;
    }

    void writeu4(u4 value) {
      writeu1(static_cast<u1>((value >> 24) & 0xff));
      writeu1(static_cast<u1>((value >> 16) & 0xff));
      writeu1(static_cast<u1>((value >> 8) & 0xff));
      writeu1(static_cast<u1>((value >> 0) & 0xff));
      offset += 4;
    }

    void writecount(const void *source, int count) {
      for (int i = 0; i < count; i++) {
        writeu1(static_cast<const u1 *>(source)[i]);
      }
      offset += count;
    }

    int getOffset() const { return offset; }

  private:
    std::ofstream out;
    int offset;
  };

  template<typename TWriter>
  class ClassWriter : private Error<Exception> {
  public:
    ClassWriter(TWriter &bw) : bw(bw) {}

    void writeClassFile(const ClassFile &cf) {
      bw.writeu4(ClassFile::MAGIC);

      bw.writeu2(cf.version.minorVersion());
      bw.writeu2(cf.version.majorVersion());

      writeConstPool(cf);

      bw.writeu2(cf.accessFlags);
      bw.writeu2(cf.thisClassIndex);
      bw.writeu2(cf.superClassIndex);

      u2 interCount = cf.interfaces.size();
      bw.writeu2(interCount);

      for (u2 interIndex: cf.interfaces) {
        bw.writeu2(interIndex);
      }

      bw.writeu2(cf.fields.size());
      for (const Field &f: cf.fields) {
        bw.writeu2(f.accessFlags);
        bw.writeu2(f.nameIndex);
        bw.writeu2(f.descIndex);

        writeAttrs(f.attrs);
      }

      bw.writeu2(cf.methods.size());
      for (const Method &m: cf.methods) {
        bw.writeu2(m.accessFlags);
        bw.writeu2(m.nameIndex);
        bw.writeu2(m.descIndex);

        writeAttrs(m.attrs);
      }

      writeAttrs(cf.attrs);
    }

    void writeConstPool(const ConstPool &cp) {
      u2 count = cp.size();
      bw.writeu2(count);

      for (ConstPool::Iterator it = cp.iterator(); it.hasNext(); it++) {
        ConstPool::Index i = *it;
        const ConstPool::Item *entry = &cp.entries[i];

        bw.writeu1(entry->tag);

        switch (entry->tag) {
          case ConstPool::CLASS:
            bw.writeu2(entry->clazz.nameIndex);
            break;
          case ConstPool::FIELDREF:
            bw.writeu2(entry->memberRef.classIndex);
            bw.writeu2(entry->memberRef.nameAndTypeIndex);
            break;
          case ConstPool::METHODREF:
            bw.writeu2(entry->memberRef.classIndex);
            bw.writeu2(entry->memberRef.nameAndTypeIndex);
            break;
          case ConstPool::INTERMETHODREF:
            bw.writeu2(entry->memberRef.classIndex);
            bw.writeu2(entry->memberRef.nameAndTypeIndex);
            break;
          case ConstPool::STRING:
            bw.writeu2(entry->s.stringIndex);
            break;
          case ConstPool::INTEGER:
            bw.writeu4(entry->i.value);
            break;
          case ConstPool::FLOAT: {
            float fvalue = entry->f.value;
            u4 value = *(u4 *) &fvalue;
            bw.writeu4(value);
            break;
          }
          case ConstPool::LONG: {
            long value = cp.getLong(i);
            bw.writeu4(value >> 32);
            bw.writeu4(value & 0xffffffff);
            //		i++;
            break;
          }
          case ConstPool::DOUBLE: {
            double dvalue = cp.getDouble(i);
            long value = *(long *) &dvalue;
            bw.writeu4(value >> 32);
            bw.writeu4(value & 0xffffffff);
            //			i++;
            break;
          }
          case ConstPool::NAMEANDTYPE:
            bw.writeu2(entry->nameAndType.nameIndex);
            bw.writeu2(entry->nameAndType.descriptorIndex);
            break;
          case ConstPool::UTF8: {
            u2 len = entry->utf8.str.length();
            const char *str = entry->utf8.str.c_str();
            bw.writeu2(len);
            bw.writecount(str, len);
            break;
          }
          case ConstPool::METHODHANDLE:
            bw.writeu1(entry->methodHandle.referenceKind);
            bw.writeu2(entry->methodHandle.referenceIndex);
            break;
          case ConstPool::METHODTYPE:
            bw.writeu2(entry->methodType.descriptorIndex);
            break;
          case ConstPool::INVOKEDYNAMIC:
            bw.writeu2(entry->invokeDynamic.bootstrapMethodAttrIndex);
            bw.writeu2(entry->invokeDynamic.nameAndTypeIndex);
            break;
          default:
            throw Exception("invalid tag!");
        }
      }
    }

    void writeUnknown(const UnknownAttr &attr) { bw.writecount(attr.data, attr.len); }

    void writeExceptions(const ExceptionsAttr &attr) {
      u2 size = attr.es.size();

      bw.writeu2(size);
      for (u4 i = 0; i < size; i++) {
        u2 e = attr.es[i];
        bw.writeu2(e);
      }
    }

    void writeLnt(const LntAttr &attr) {
      u2 count = attr.lnt.size();

      bw.writeu2(count);

      for (u4 i = 0; i < count; i++) {
        const LntAttr::LnEntry &lne = attr.lnt[i];

        // bw.writeu2(lne.startpc);
        bw.writeu2(lne.startPcLabel->label()->offset);
        bw.writeu2(lne.lineno);
      }
    }

    void writeLvt(LvtAttr &attr) {
      u2 count = attr.lvt.size();

      bw.writeu2(count);

      for (u4 i = 0; i < count; i++) {
        LvtAttr::LvEntry &lve = attr.lvt[i];

        u2 startPc = lve.startPcLabel->label()->offset;
        // u2 endPc = lve.endPcLabel->label()->offset;

        // bw.writeu2(lve.startPc);
        bw.writeu2(startPc);

        // u2 len = endPc - startPc;
        bw.writeu2(lve.len);
        // bw.writeu2(len);

        bw.writeu2(lve.varNameIndex);
        bw.writeu2(lve.varDescIndex);
        bw.writeu2(lve.index);
      }
    }

    void writeSourceFile(SourceFileAttr &attr) { bw.writeu2(attr.sourceFileIndex); }

    void writeSignature(SignatureAttr &attr) { bw.writeu2(attr.signatureIndex); }

    void writeSmt(SmtAttr &attr) {
      bw.writeu2(attr.entries.size());

      int toff = -1;

      for (u2 i = 0; i < attr.entries.size(); i++) {
        SmtAttr::Entry &e = attr.entries[i];

        u2 offset = e.label->label()->offset;

        toff += 1;

        u2 deltaOffset = offset - toff;

        toff = offset;

        u1 frameType = e.frameType;

        if (0 <= frameType && frameType <= 63) {
          if (deltaOffset <= 63) {
            frameType = deltaOffset;
          } else {
            frameType = 251;
            // EXCEPTION("not implemented 1");
          }
        } else if (64 <= frameType && frameType <= 127) {
          if (deltaOffset <= 63) {
            frameType = 64 + deltaOffset;
          } else {
            frameType = 247;
            // EXCEPTION("not implemented 2");
          }
        }

        bw.writeu1(frameType);

        if (0 <= frameType && frameType <= 63) {
        } else if (64 <= frameType && frameType <= 127) {
          writeSmtTypes(e.sameLocals_1_stack_item_frame.stack);
        } else if (frameType == 247) {
          u2 offsetDelta = deltaOffset;
          bw.writeu2(offsetDelta);
          writeSmtTypes(e.same_locals_1_stack_item_frame_extended.stack);
        } else if (248 <= frameType && frameType <= 250) {
          u2 offsetDelta = deltaOffset;
          bw.writeu2(offsetDelta);
        } else if (frameType == 251) {
          u2 offsetDelta = deltaOffset;
          bw.writeu2(offsetDelta);
        } else if (252 <= frameType && frameType <= 254) {
          u2 offsetDelta = deltaOffset;
          bw.writeu2(offsetDelta);
          writeSmtTypes(e.append_frame.locals);
        } else if (frameType == 255) {
          u2 offsetDelta = deltaOffset;
          bw.writeu2(offsetDelta);

          u2 numberOfLocals = e.full_frame.locals.size();
          bw.writeu2(numberOfLocals);
          writeSmtTypes(e.full_frame.locals);

          u2 numberOfStackItems = e.full_frame.stack.size();
          bw.writeu2(numberOfStackItems);
          writeSmtTypes(e.full_frame.stack);
        }
      }
    }

    void writeSmtTypes(const vector<Type> &locs) {
      for (u1 i = 0; i < locs.size(); i++) {
        const Type &type = locs[i];

        if (type.isTop()) {
          bw.writeu1(TYPE_TOP);
        } else if (type.isIntegral()) {
          bw.writeu1(TYPE_INTEGER);
        } else if (type.isFloat()) {
          bw.writeu1(TYPE_FLOAT);
        } else if (type.isLong()) {
          bw.writeu1(TYPE_LONG);
        } else if (type.isDouble()) {
          bw.writeu1(TYPE_DOUBLE);
        } else if (type.isNull()) {
          bw.writeu1(TYPE_NULL);
        } else if (type.isUninitThis()) {
          bw.writeu1(TYPE_UNINITTHIS);
        } else if (type.isObject()) {
          bw.writeu1(TYPE_OBJECT);
          u2 cpIndex = type.getCpIndex();
          bw.writeu2(cpIndex);
        } else if (type.isUninit()) {
          bw.writeu1(TYPE_UNINIT);
          u2 offset = type.uninit.offset;
          offset = type.uninit.label->label()->offset;
          bw.writeu2(offset);
        } else {
          throw Exception("Invalid type on write: ", type);
        }

        //			u1 tag = type.tag;
        //			bw.writeu1(tag);
        //			switch (tag) {
        //				case Type::TYPE_TOP:
        //					break;
        //				case Type::TYPE_INTEGER:
        //					break;
        //				case Type::TYPE_FLOAT:
        //					break;
        //				case Type::TYPE_LONG:
        //					break;
        //				case Type::TYPE_DOUBLE:
        //					break;
        //				case Type::TYPE_NULL:
        //					break;
        //				case Type::TYPE_UNINITTHIS:
        //					break;
        //				case Type::TYPE_OBJECT: {
        //					u2 cpIndex = type.getCpIndex();
        //					bw.writeu2(cpIndex);
        //					break;
        //				}
        //				case Type::TYPE_UNINIT: {
        //					u2 offset = type.uninit.offset;
        //					offset = type.uninit.label->label.offset;
        //					bw.writeu2(offset);
        //					break;
        //				}
        //			}
      }
    }

    int pos(int offset) { return bw.getOffset() - offset; }

    void computeInstOffset(InstList &instList) {
      SizeWriter sz = SizeWriter();

      for (Inst *instp: instList) {
        Inst &inst = *instp;

        instp->_offset = sz.getOffset();

        if (inst.kind == KIND_LABEL) {
          inst.label()->offset = sz.getOffset();
          continue;
        }

        sz.writeu1(static_cast<u1>(0));

        switch (inst.kind) {
          case KIND_ZERO:
            if (inst.opcode == Opcode::wide) {
              sz.writeu1(static_cast<u1>(0));
              if (inst.wide()->subOpcode == Opcode::iinc) {
                sz.writeu2(static_cast<u2>(0));
                sz.writeu2(static_cast<u2>(0));
              } else {
                sz.writeu2(static_cast<u2>(0));
              }
            }
            break;
          case KIND_BIPUSH: {
            sz.writeu1(static_cast<u1>(0));
            break;
          }
          case KIND_SIPUSH:
            sz.writeu2(static_cast<u2>(0));
            break;
          case KIND_LDC:
            if (inst.opcode == Opcode::ldc) {
              sz.writeu1(static_cast<u1>(0));
            } else {
              sz.writeu2(static_cast<u2>(0));
            }
            break;
          case KIND_VAR:
            sz.writeu1(static_cast<u1>(0));
            break;
          case KIND_IINC:
            sz.writeu1(static_cast<u1>(0));
            sz.writeu1(static_cast<u1>(0));
            break;
          case KIND_JUMP: {
            sz.writeu2(static_cast<u2>(0));
            break;
          }
          case KIND_TABLESWITCH: {
            int pad = (4 - (sz.getOffset() % 4)) % 4;
            for (int i = 0; i < pad; i++) {
              sz.writeu1(static_cast<u1>(0));
            }

            sz.writeu4(static_cast<u4>(0));
            sz.writeu4(static_cast<u4>(0));
            sz.writeu4(static_cast<u4>(0));

            for (int i = 0; i < inst.ts()->high - inst.ts()->low + 1; i++) {
              sz.writeu4(static_cast<u4>(0));
            }

            break;
          }
          case KIND_LOOKUPSWITCH: {
            int pad = (4 - (sz.getOffset() % 4)) % 4;
            for (int i = 0; i < pad; i++) {
              sz.writeu1(static_cast<u1>(0));
            }

            sz.writeu4(static_cast<u4>(0));
            sz.writeu4(static_cast<u4>(0));

            for (u4 i = 0; i < inst.ls()->npairs; i++) {
              sz.writeu4(static_cast<u4>(0));
              sz.writeu4(static_cast<u4>(0));
            }
            break;
          }
          case KIND_FIELD:
            sz.writeu2(static_cast<u2>(0));
            break;
          case KIND_INVOKE:
            sz.writeu2(static_cast<u2>(0));
            break;
          case KIND_INVOKEINTERFACE:
            sz.writeu2(static_cast<u2>(0));
            sz.writeu1(static_cast<u1>(0));
            sz.writeu1(static_cast<u1>(0));
            break;
          case KIND_INVOKEDYNAMIC:
            sz.writeu2(static_cast<u2>(0));
            sz.writeu2(static_cast<u2>(0));
            break;
          case KIND_TYPE:
            sz.writeu2(static_cast<u2>(0));
            break;
          case KIND_NEWARRAY:
            sz.writeu1(static_cast<u1>(0));
            break;
          case KIND_MULTIARRAY:
            sz.writeu2(static_cast<u2>(0));
            sz.writeu1(static_cast<u1>(0));
            break;
          case KIND_PARSE4TODO:
            throw Exception("not implemetd");
            break;
          case KIND_RESERVED:
            throw Exception("not implemetd");
            break;
          default:
            throw Exception("default kind in instlist: ", inst.kind);
        }
      }
    }

    void writeInstList(InstList &instList) {
      int offset = bw.getOffset();

      for (Inst *instp: instList) {
        Inst &inst = *instp;
        if (inst.kind == KIND_LABEL) {
          continue; // Labels are not written
        }

        bw.writeu1((u1) inst.opcode);

        switch (inst.kind) {
          case KIND_ZERO:
            if (inst.opcode == Opcode::wide) {
              bw.writeu1((u1) inst.wide()->subOpcode);
              if (inst.wide()->subOpcode == Opcode::iinc) {
                bw.writeu2(inst.wide()->iinc.index);
                bw.writeu2(inst.wide()->iinc.value);
              } else {
                bw.writeu2(inst.wide()->var.lvindex);
              }
            }
            break;
          case KIND_BIPUSH: {
            u1 value = inst.push()->value;
            Error::asserts(value == inst.push()->value, "push: u1 value != u2 value");
            bw.writeu1(inst.push()->value);
            break;
          }
          case KIND_SIPUSH:
            bw.writeu2(inst.push()->value);
            break;
          case KIND_LDC:
            if (inst.opcode == Opcode::ldc) {
              Error::asserts(
                  inst.ldc()->valueIndex == (u1) inst.ldc()->valueIndex,
                  "invalid value for ldc: %d", inst.ldc()->valueIndex
              );

              bw.writeu1(inst.ldc()->valueIndex);
            } else {
              bw.writeu2(inst.ldc()->valueIndex);
            }
            break;
          case KIND_VAR:
            bw.writeu1(inst.var()->lvindex);
            break;
          case KIND_IINC:
            bw.writeu1(inst.iinc()->index);
            bw.writeu1(inst.iinc()->value);
            break;
          case KIND_JUMP: {
            int jumppos = instp->_offset;
            bw.writeu2(inst.jump()->label2->label()->offset - jumppos);
            break;
          }
          case KIND_TABLESWITCH: {
            int tspos = instp->_offset;

            int pad = (4 - (pos(offset) % 4)) % 4;
            for (int i = 0; i < pad; i++) {
              bw.writeu1(0);
            }

            bool check = pos(offset) % 4 == 0;
            Error::asserts(check, "Padding offset must be mod 4: %d", pos(offset));

            bw.writeu4(inst.ts()->def->label()->offset - tspos);
            bw.writeu4(inst.ts()->low);
            bw.writeu4(inst.ts()->high);

            for (int i = 0; i < inst.ts()->high - inst.ts()->low + 1; i++) {
              Inst *t = inst.ts()->targets[i];
              bw.writeu4(t->label()->offset - tspos);
            }

            //	fprintf(stderr, "writer ts: offset: %d\n", pos());

            break;
          }
          case KIND_LOOKUPSWITCH: {
            int lspos = instp->_offset;

            int pad = (4 - (pos(offset) % 4)) % 4;
            for (int i = 0; i < pad; i++) {
              bw.writeu1(0);
            }

            bool check = pos(offset) % 4 == 0;
            Error::asserts(check, "Padding offset must be mod 4: %d", pos(offset));

            bw.writeu4(inst.ls()->defbyte->label()->offset - lspos);
            bw.writeu4(inst.ls()->npairs);

            for (u4 i = 0; i < inst.ls()->npairs; i++) {
              u4 k = inst.ls()->keys[i];
              bw.writeu4(k);

              Inst *t = inst.ls()->targets[i];
              bw.writeu4(t->label()->offset - lspos);
            }
            break;
          }
          case KIND_FIELD:
            bw.writeu2(inst.field()->fieldRefIndex);
            break;
          case KIND_INVOKE:
            bw.writeu2(inst.invoke()->methodRefIndex);
            break;
          case KIND_INVOKEINTERFACE:
            bw.writeu2(inst.invokeinterface()->interMethodRefIndex);
            bw.writeu1(inst.invokeinterface()->count);
            bw.writeu1(0);
            break;
          case KIND_INVOKEDYNAMIC:
            bw.writeu2(inst.indy()->callSite());
            bw.writeu2(0);
            break;
          case KIND_TYPE:
            bw.writeu2(inst.type()->classIndex);
            break;
          case KIND_NEWARRAY:
            bw.writeu1(inst.newarray()->atype);
            break;
          case KIND_MULTIARRAY:
            bw.writeu2(inst.multiarray()->classIndex);
            bw.writeu1(inst.multiarray()->dims);
            break;
          case KIND_PARSE4TODO:
            throw Exception("not implemetd");
            break;
          case KIND_RESERVED:
            throw Exception("not implemetd");
            break;
          default:
            throw Exception("default kind in instlist: ", inst.kind);
        }
      }
    }

    void writeCode(CodeAttr &attr) {
      bw.writeu2(attr.maxStack);
      bw.writeu2(attr.maxLocals);
      bw.writeu4(attr.codeLen);

      u4 offset = bw.getOffset();

      computeInstOffset(attr.instList);
      writeInstList(attr.instList);

      attr.codeLen = bw.getOffset() - offset;

      // if (attr.codeLen != -1) {
      try {
        Error::checks(attr.codeLen != 0, "Method code must not be zero");
        Error::checks(
            attr.codeLen < 65536, "Method code must be less than 65536 but it is equals to ",
            attr.codeLen
        );
      } catch (const Exception &ex) {
        throw InvalidMethodLengthException(ex.message);
      }
      //}

      u2 esize = attr.exceptions.size();
      bw.writeu2(esize);
      // for (u4 i = 0; i < esize; i++) {
      // const CodeExceptionEntry& e = attr.exceptions[i];
      for (const CodeAttr::ExceptionHandler &e: attr.exceptions) {
        bw.writeu2(e.startpc->label()->offset);
        bw.writeu2(e.endpc->label()->offset);
        bw.writeu2(e.handlerpc->label()->offset);
        bw.writeu2(e.catchtype);
      }

      writeAttrs(attr.attrs);
    }

    void writeAttrs(const Attrs &attrs) {
      bw.writeu2(attrs.size());

      for (u4 i = 0; i < attrs.size(); i++) {

        Attr &attr = (Attr &) *attrs.attrs[i];

        bw.writeu2(attr.nameIndex);
        bw.writeu4(attr.len);

        u4 offset = bw.getOffset();

        switch (attr.kind) {
          case ATTR_SOURCEFILE:
            writeSourceFile((SourceFileAttr &) attr);
            break;
          case ATTR_SIGNATURE:
            writeSignature((SignatureAttr &) attr);
            break;
          case ATTR_CODE:
            writeCode((CodeAttr &) attr);
            break;
          case ATTR_EXCEPTIONS:
            writeExceptions((ExceptionsAttr &) attr);
            break;
          case ATTR_LVT:
            writeLvt((LvtAttr &) attr);
            break;
          case ATTR_LVTT:
            writeLvt((LvtAttr &) attr);
            break;
          case ATTR_LNT:
            writeLnt((LntAttr &) attr);
            break;
          case ATTR_SMT:
            writeSmt((SmtAttr &) attr);
            break;
          case ATTR_UNKNOWN:
            writeUnknown((UnknownAttr &) attr);
            break;
        }
        //}

        //		u4 len = bw.getOffset() - offset;
        //		ASSERT(attr.len == len, "Expected %d, actual %d in %d", attr.len, len,
        //				attr.kind);

        attr.len = bw.getOffset() - offset;
      }
    }

  private:
    TWriter &bw;
  };

  u4 ClassFile::computeSize() {
    SizeWriter bw;
    ClassWriter<SizeWriter>(bw).writeClassFile(*this);

    return bw.getOffset();
  }

  void ClassFile::write(u1 *fileImage, int fileImageLen) {
    BufferWriter bw(fileImage, fileImageLen);
    ClassWriter<BufferWriter>(bw).writeClassFile(*this);
  }

  void ClassFile::write(const std::string &file) {
    FileWriter fw(file);
    ClassWriter<FileWriter>(fw).writeClassFile(*this);
  }

} // namespace jnif
