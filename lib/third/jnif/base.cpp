/*
 * Error.cpp
 *
 *  Created on: Jun 13, 2014
 *      Author: luigi
 */
#include "jnif/jnif.hpp"

#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>

namespace jnif {

  static void _backtrace(ostream &os) {
    void *array[20];
    size_t size;

    size = backtrace(array, 20);

    char **symbols = backtrace_symbols(array, size);
    for (size_t i = 0; i < size; i++) {
      const char *symbol = symbols[i];
      os << "    " << symbol << std::endl;
    }

    free(symbols);
  }

  Exception::Exception() {
    stringstream os;
    _backtrace(os);

    stackTrace = os.str();
  }

  class Arena::Block {
  public:
    Block(Block *next, size_t blockSize) : _next(next), _buffer(malloc(blockSize)), _position(0) {
      JnifError::checks(_buffer != nullptr, "Block alloc is NULL");
    }

    ~Block() { free(_buffer); }

    void *alloc(int size) {
      JnifError::asserts(size <= BLOCK_SIZE, "Size too large for a block: ", size);

      if (_position + size <= BLOCK_SIZE) {
        void *offset = (char *) _buffer + _position;
        _position += size;
        return offset;
      }

      return nullptr;
    }

    Block *_next;
    void *_buffer;
    int _position;
  };

  Arena::Arena(size_t blockSize) : blockSize(blockSize), _head(new Block(nullptr, blockSize)) {}

  Arena::~Arena() {
    for (Block *block = _head; block != nullptr;) {
      Block *next = block->_next;
      delete block;
      block = next;
    }
  }

  void *Arena::alloc(int size) {
    void *res = _head->alloc(size);
    if (res == nullptr) {
      _head = new Block(_head, blockSize);
      res = _head->alloc(size);
    }

    JnifError::asserts(res != nullptr, "alloc == NULL");

    return res;
  }

  void ClassHierarchy::addClass(const ClassFile &classFile) {
    ClassEntry e;
    e.className = classFile.getThisClassName();

    if (classFile.superClassIndex == ConstPool::NULLENTRY) {
      JnifError::checks(
          e.className == "java/lang/Object",
          "invalid class name for null super class: ", e.className, "asdfasf"
      );
      e.superClassName = "0";
    } else {
      e.superClassName = classFile.getClassName(classFile.superClassIndex);
    }

    // for (ConstIndex interIndex : classFile.interfaces) {
    // const string& interName = classFile.getClassName(interIndex);
    // e.interfaces.push_back(interName);
    // }

    classes[e.className] = e;
    // classes.push_front(e);
  }

  const string &ClassHierarchy::getSuperClass(const string &className) const {
    auto it = getEntry(className);
    JnifError::asserts(it != classes.end(), "Class not defined");

    return it->second.superClassName;
  }

  bool ClassHierarchy::isAssignableFrom(const string &sub, const string &sup) const {

    string cls = sub;
    while (cls != "0") {
      if (cls == sup) {
        return true;
      }

      cls = getSuperClass(cls);
    }

    return false;
  }

  bool ClassHierarchy::isDefined(const string &className) const {
    //	const ClassHierarchy::ClassEntry* e = getEntry(className);
    //	return e != NULL;
    auto it = getEntry(className);
    return it != classes.end();
  }

  map<string, ClassHierarchy::ClassEntry>::const_iterator
  ClassHierarchy::getEntry(const string &className) const {

    auto it = classes.find(className);

    return it;

    //	if (it != classes.end()) {
    //		return &it->second;
    //	} else {
    //		return NULL;
    //	}
    //	for (const ClassHierarchy::ClassEntry& e : *this) {
    //		if (e.className == className) {
    //			return &e;
    //		}
    //	}
    //
    //	return NULL;
  }

  ostream &operator<<(ostream &os, const ClassHierarchy &) {
    //	for (const ClassHierarchy::ClassEntry& e : ch) {
    //		os << "Class: " << e.className << ", ";
    //		os << "Super: " << e.superClassName << ", ";
    //		os << "Interfaces: { ";
    //		for (const string& interName : e.interfaces) {
    //			os << interName << " ";
    //		}
    //
    //		os << " }" << endl;
    //	}

    return os;
  }

  ostream &operator<<(ostream &os, const DomMap &ds) {
    for (const pair<BasicBlock *, set<BasicBlock *>> &d: ds) {
      os << d.first->name << ": ";
      for (const BasicBlock *bb: d.second) {
        os << bb->name << " ";
      }
      os << std::endl;
    }

    return os;
  }

} // namespace jnif
