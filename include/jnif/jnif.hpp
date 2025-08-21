#ifndef JNIF_HPP
#define JNIF_HPP

#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

/**
 * The jnif namespace contains all type definitions, constants, enumerations
 * and structs of the jnif framework.
 *
 * This implementation is based on Chapter 4 (The class File Format) and
 * Chapter 6 (Chapter 6. The Java Virtual Machine Instruction Set) of the
 * Java Virtual Machine Specification version 7. Portions of this
 * documentation are taken from this specification.
 *
 * For more information refer to:
 *
 * http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html
 *
 * http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-6.html
 *
 * @see ClassFile
 */
namespace jnif {

  using std::list;
  using std::map;
  using std::ostream;
  using std::pair;
  using std::set;
  using std::string;
  using std::stringstream;
  using std::vector;

  /**
   * Represents a byte inside the Java Class File.
   * The sizeof(u1) must be equal to 1.
   */
  typedef unsigned char u1;

  /**
   * Represents two bytes inside the Java Class File.
   * The sizeof(u2) must be equal to 2.
   */
  typedef unsigned short u2;

  /**
   * Represents four bytes inside the Java Class File.
   * The sizeof(u4) must be equal to 4.
   */
  typedef unsigned int u4;

  /**
   * Represents the base exception that JNIF can throw.
   */
  class Exception : public std::exception {
  public:
    /**
     * Creates a default exception with the current stacktrace.
     */
    Exception();

    /**
     * Creates an exception given the message and the current stacktrace.
     *
     * @tparam TArgs the types of the arguments to be put in the message.
     * @param args contains information about exceptional situation.
     */
    template<typename... TArgs>
    explicit Exception(const TArgs &...args) : Exception() {
      stringstream ss;
      _format(ss, args...);

      message = ss.str();
    }

    /**
     * Information about the exceptional situation.
     */
    string message;

    /**
     * The stack trace where this exception happened.
     */
    string stackTrace;

    /**
     * Shows the exception in the specified ostream.
     */
    friend ostream &operator<<(ostream &os, const Exception &ex);

    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }

  private:
    static void _format(ostream &) {}

    template<typename TArg, typename... TArgs>
    static void _format(ostream &os, const TArg &arg, const TArgs &...args) {
      os << arg;
      _format(os, args...);
    }
  };

  class WriterException : public Exception {
  public:
    explicit WriterException(const string &message) : Exception(message) {}
  };

  class InvalidMethodLengthException : public WriterException {
  public:
    explicit InvalidMethodLengthException(const string &message) : WriterException(message) {}
  };

  /**
   * Contains static method to facilitate error handling mechanism.
   *
   * @tparam TException The type of exception to be thrown in case of error.
   */
  template<typename TException>
  class Error {
  public:
    template<typename... TArgs>
    static void asserts(bool cond, const TArgs &...args) {
      if (!cond) {
        throw TException(args...);
      }
    }

    template<typename T, typename... TArgs>
    static void assertEquals(const T &expected, const T &actual, const TArgs &...args) {
      asserts(
          expected == actual, "assertEqual failed: expected=", expected, ", actual=", actual,
          ", message: ", args...
      );
    }

    template<typename... TArgs>
    static void checks(bool cond, const TArgs &...args) {
      if (!cond) {
        throw TException(args...);
      }
    }
  };

  class JnifError : public Error<Exception> {};

  template<class T, class... TArgs>
  static void assertEquals(const T &actual, const T &expected, const TArgs &...args) {
    JnifError::asserts(
        actual == expected, "assertEqual failed: expected=", expected, ", actual=", actual,
        ", message: ", args...
    );
  }

  class Arena {
  public:
    static constexpr int BLOCK_SIZE = 1024 * 1024;

    Arena(const Arena &) = delete;

    Arena(Arena &) = delete;

    Arena(const Arena &&) = delete;

    explicit Arena(size_t blockSize = BLOCK_SIZE);

    ~Arena();

    void *alloc(int size);

    template<typename T, typename... TArgs>
    T *create(const TArgs &...args) {
      void *buf = alloc(sizeof(T));
      return new (buf) T(args...);
    }

    template<typename T>
    T *newArray(int size) {
      void *buf = alloc(sizeof(T) * size);
      return new (buf) T[size];
    }

  private:
    class Block;

    size_t blockSize;

    Block *_head;
  };

  class ControlFlowGraph;

  namespace model {

    /**
     * Represents the Java class file's constant pool.
     * Provides the base services to manage the constant pool.
     * The constant pool is a table which holds
     * different kinds of items depending on their use. It can
     * hold references for classes, fields, methods, interface methods, strings,
     * integers, floats, longs, doubles, name and type, utf8 arrays,
     * method handles, method types and invoke dynamic bootstrap methods.
     *
     * This class works by adding these kinds of entries in an array-like
     * structure. When an entry is removed, all entries after the one removed will
     * be invalidated, for this reason, no removed operations are allowed.
     */
    class ConstPool {
    public:
      ConstPool(const ConstPool &) = delete;

      ConstPool(ConstPool &&) = delete;

      ConstPool &operator=(const ConstPool &) = delete;

      ConstPool &operator=(ConstPool &&) = delete;

      /**
       * The Index type represents how each item within the constant pool
       * can be addressed.
       * The specification indicates that this is an u2 value.
       */
      typedef u2 Index;

      /**
       * Represents a class, interface or array type.
       * @see CLASS
       */
      struct Class {

        /**
         * The name of the class of this type.
         * @see UTF8
         * @see Utf8
         */
        const Index nameIndex;
      };

      /// Represents a symbolic reference to a member
      /// (field, method or interface method) of a class.
      /// @see FieldRef
      /// @see MethodRef
      /// @see InterMethodRef
      struct MemberRef {

        /// Index to the class of this member reference.
        /// @see Class
        const Index classIndex;

        /// Index to the name and type of this member reference.
        /// @see NameAndType
        const Index nameAndTypeIndex;
      };

      /// Represents a string constant value.
      /// @see STRING
      struct String {

        /// The index of the utf8 entry containing this string value.
        /// @see Utf8
        const Index stringIndex;
      };

      /// Represents an integer constant value.
      /// @see INTEGER
      struct Integer {

        /// The integer value of this entry.
        const int value;
      };

      /// Represent a float constant value.
      /// @see FLOAT
      struct Float {

        /// The float value of this entry.
        const float value;
      };

      /// Represents a long constant value.
      /// @see LONG
      struct Long {

        /// The long value of this entry.
        const long value;
      };

      /// Represents a double constant value.
      /// @see DOUBLE
      struct Double {

        /// The double value of this entry.
        const double value;
      };

      /// Represents a tuple of name and descriptor.
      struct NameAndType {

        /// The index of the utf8 entry containing the name of this entry.
        const Index nameIndex;

        /// The utf8 entry index containing the descriptor of this entry.
        const Index descriptorIndex;
      };

      /// Contains a modified UTF-8 string.
      /// @see UTF8
      struct Utf8 {

        Utf8() {}

        Utf8(const string &str) : str(str) {}

        /// The string data.
        const string str;
      };

      /// Represents a method handle entry.
      /// @see METHODHANDLE
      struct MethodHandle {

        /// The reference kind of this entry.
        const u1 referenceKind;

        /// The reference index of this entry.
        const u2 referenceIndex;
      };

      /// Represents the type of a method.
      /// @see METHODTYPE
      struct MethodType {

        /// The utf8 index containing the descriptor of this entry.
        const Index descriptorIndex;
      };

      /// Represents an invoke dynamic call site.
      /// @see INVOKEDYNAMIC
      struct InvokeDynamic {

        /// The bootstrap method attribute index.
        const Index bootstrapMethodAttrIndex;

        /// The name and type index of this entry.
        /// @see ConstNameAndType
        const Index nameAndTypeIndex;
      };

      /// Constant pool enum used to distinguish between different kinds of
      /// elements inside the constant pool.
      enum Tag {

        /// Represents the null entry which cannot be addressed.
        /// This is used for the NULLENTRY (position zero) and
        /// for long and double entries.
        NULLENTRY,

        /// Represents a class or an interface.
        CLASS = 7,

        /// Represents a field.
        FIELDREF = 9,

        /// Represents a method.
        METHODREF = 10,

        /// Represents an inteface method.
        INTERMETHODREF = 11,

        /// Used to represent constant objects of the type String.
        STRING = 8,

        /// Represents 4-byte numeric int constants.
        INTEGER = 3,

        /// Represents 4-byte numeric float constants.
        FLOAT = 4,

        /// Represents 8-byte numeric long constants.
        LONG = 5,

        /// Represents 8-byte numeric double constants.
        DOUBLE = 6,

        /// Used to represent a field or method, without indicating which class
        /// or interface type it belongs to.
        NAMEANDTYPE = 12,

        /// Used to represent constant string values.
        UTF8 = 1,

        /// Used to represent a method handle.
        METHODHANDLE = 15,

        /// Used to represent a method type.
        METHODTYPE = 16,

        /// Used by an invokedynamic instruction to specify a bootstrap method,
        /// the dynamic invocation name, the argument and return types of the
        /// call, and optionally, a sequence of additional constants called
        /// static arguments to the bootstrap method.
        INVOKEDYNAMIC = 18
      };

      /**
       * The const item.
       */
      struct Item {

        Item() : tag(NULLENTRY) {}

        explicit Item(Class clazz) : tag(CLASS), clazz(clazz) {}

        Item(Tag tag, MemberRef memberRef) : tag(tag), memberRef(memberRef) {}

        Item(String s) : tag(STRING), s(s) {}

        Item(Integer i) : tag(INTEGER), i(i) {}

        Item(Float f) : tag(FLOAT), f(f) {}

        Item(Long l) : tag(LONG), l(l) {}

        Item(Double d) : tag(DOUBLE), d(d) {}

        Item(NameAndType nat) : tag(NAMEANDTYPE), nameAndType(nat) {}

        Item(MethodHandle mh) : tag(METHODHANDLE), methodHandle(mh) {}

        Item(MethodType mt) : tag(METHODTYPE), methodType(mt) {}

        Item(InvokeDynamic id) : tag(INVOKEDYNAMIC), invokeDynamic(id) {}

        Item(const string &value) : tag(UTF8), utf8(value) {}

        Item(const Item &) = delete;

        Item &operator=(const Item &) = delete;

        Item(Item &&) = default;

        const Tag tag;

        union {
          Class clazz;
          MemberRef memberRef;
          String s;
          Integer i;
          Float f;
          Long l;
          Double d;
          NameAndType nameAndType;
          MethodHandle methodHandle;
          MethodType methodType;
          InvokeDynamic invokeDynamic;
        };

        const Utf8 utf8;
      };

      /**
       * Defines how to iterate the constant pool.
       */
      class Iterator {
        friend class ConstPool;

      public:
        bool hasNext() const { return index < cp.size(); }

        Index operator*() const { return index; }

        Iterator &operator++(int) {
          index += cp._isDoubleEntry(index) ? 2 : 1;

          return *this;
        }

      private:
        Iterator(const ConstPool &cp, Index index) : cp(cp), index(index) {}

        const ConstPool &cp;
        Index index;
      };

      /// Represents the invalid (null) item, which must not be asked for.
      static const Index NULLINDEX = 0;

      /**
       * Initializes an empty constant pool. The valid indices start from 1
       * inclusive, because the null entry (index 0) is added by default.
       */
      explicit ConstPool(size_t initialCapacity = 4096) {
        entries.reserve(initialCapacity);
        entries.emplace_back();
      }

      /// Returns the number of elements in this constant pool.
      /// @returns number of elements in this constant pool.
      u4 size() const;

      /**
       *
       */
      Iterator iterator() const;

      /// Adds a class reference to the constant pool.
      /// @param classNameIndex the utf8 index that represents the name of this
      /// class item.
      /// @returns the index of the newly created reference to a class item.
      Index addClass(Index classNameIndex);

      /**
       * Adds a class reference to the constant pool by class name. This method
       * adds also the utf8 entry corresponding the class name itself.
       *
       * @param className name of the class to reference.
       * @returns the index of the newly created reference to a class item.
       */
      Index addClass(const char *className);

      /**
       * Adds a field reference to this constant pool.
       *
       * @param classIndex the symbolic class that this field belongs to.
       * @param nameAndTypeIndex the name and type symbolic reference
       * describing the name and type of the field to add.
       * @returns the index of the newly created entry.
       */
      Index addFieldRef(Index classIndex, Index nameAndTypeIndex);

      /**
       * Adds a class method reference.
       *
       * @returns the Index of the newly created entry.
       */
      Index addMethodRef(Index classIndex, Index nameAndTypeIndex);

      /**
       * Adds a non-interface methods by method name and descriptor.
       *
       * @returns the Index of the newly created entry.
       */
      Index addMethodRef(Index classIndex, const char *name, const char *desc);

      /// Adds an interface method reference.
      /// @returns the Index of the newly created entry.
      Index addInterMethodRef(Index classIndex, Index nameAndTypeIndex);

      /**
       * @returns the Index of the newly created entry.
       */
      Index addString(Index utf8Index);

      /**
       * Adds a string to the constant pool by providing a string.
       */
      Index addString(const string &str);

      /**
       * Adds a String item to the Constant Pool using a class entry.
       *
       * @returns the Index of the newly created String entry.
       */
      Index addStringFromClass(Index classIndex);

      /**
       * Adds an integer constant value.
       *
       * @param value the integer value.
       * @returns the Index of the newly created entry.
       */
      Index addInteger(int value);

      /**
       * Adds a float constant value.
       *
       * @param value the float value.
       * @returns the Index of the newly created entry.
       */
      Index addFloat(float value);

      /**
       * Adds a long constant value.
       *
       * @param value the long value.
       * @returns the Index of the newly created entry.
       */
      Index addLong(long value);

      /**
       * Adds a double constant value.
       *
       * @param value the double value.
       * @returns the Index of the newly created entry.
       */
      Index addDouble(double value);

      /**
       * Adds a name and type descriptor pair.
       *
       * @param nameIndex the index of the UTF8 entry with the name.
       * @param descIndex the index of the UTF8 entry with the type descriptor.
       * @returns the Index of the newly created entry.
       */
      Index addNameAndType(Index nameIndex, Index descIndex);

      /**
       * Adds a modified UTF8 string given the char array and its len.
       *
       * @param utf8 the char array containing the modified UTF8 string.
       * @param len the len in bytes of utf8.
       * @returns the Index of the newly created entry.
       */
      Index addUtf8(const char *utf8, int len);

      /**
       * Adds a modified UTF8 string given an null-terminated char array.
       *
       * @param str the null-terminated char array containing the modified
       * UTF8 string.
       * @returns the Index of the newly created entry.
       */
      Index addUtf8(const char *str);

      /**
       * @returns the Index of the newly created entry.
       */
      Index addMethodHandle(u1 refKind, u2 refIndex);

      /// @returns the Index of the newly created entry.
      Index addMethodType(u2 descIndex);

      /// @returns the Index of the newly created entry.
      Index addInvokeDynamic(u2 bootstrapMethodAttrIndex, u2 nameAndTypeIndex);

      /**
       *
       */
      Tag getTag(Index index) const {
        const Item *entry = _getEntry(index);
        return entry->tag;
      }

      /**
       * Checks whether the requested index holds a class reference.
       */
      bool isClass(Index index) const { return _getEntry(index)->tag == CLASS; }

      bool isUtf8(Index index) const { return _getEntry(index)->tag == UTF8; }

      Index getClassNameIndex(Index classIndex) const {
        const Item *e = _getEntry(classIndex, CLASS, "CONSTANT_Class");
        Index ni = e->clazz.nameIndex;

        return ni;
      }

      void getFieldRef(Index index, string *className, string *name, string *desc) const {
        const Item *e = _getEntry(index, FIELDREF, "FieldRef");
        const MemberRef &mr = e->memberRef;
        _getMemberRef(className, name, desc, mr);
      }

      void getMethodRef(Index index, string *clazzName, string *name, string *desc) const {
        const Item *e = _getEntry(index, METHODREF, "MethodRef");
        const MemberRef &mr = e->memberRef;
        _getMemberRef(clazzName, name, desc, mr);
      }

      void getInterMethodRef(Index index, string *clazzName, string *name, string *desc) const {
        const Item *e = _getEntry(index, INTERMETHODREF, "imr");
        const MemberRef &mr = e->memberRef;
        _getMemberRef(clazzName, name, desc, mr);
      }

      const char *getString(Index index) const {
        const Item *e = _getEntry(index, STRING, "String");
        return getUtf8(e->s.stringIndex);
      }

      int getInteger(Index index) const {
        return _getEntry(index, INTEGER, "CONSTANT_Integer")->i.value;
      }

      float getFloat(Index index) const {
        return _getEntry(index, FLOAT, "CONSTANT_Float")->f.value;
      }

      long getLong(Index index) const { return _getEntry(index, LONG, "CONSTANT_Long")->l.value; }

      double getDouble(Index index) const {
        return _getEntry(index, DOUBLE, "CONSTANT_Double")->d.value;
      }

      const char *getUtf8(Index utf8Index) const {
        const Item *entry = _getEntry(utf8Index, UTF8, "Utf8");
        return entry->utf8.str.c_str();
      }

      const char *getClassName(Index classIndex) const {
        Index classNameIndex = getClassNameIndex(classIndex);
        return getUtf8(classNameIndex);
      }

      void getNameAndType(Index index, string *name, string *desc) const {
        const Item *e = _getEntry(index, NAMEANDTYPE, "NameAndType");
        u2 nameIndex = e->nameAndType.nameIndex;
        u2 descIndex = e->nameAndType.descriptorIndex;

        *name = getUtf8(nameIndex);
        *desc = getUtf8(descIndex);
      }

      const InvokeDynamic &getInvokeDynamic(Index index) const {
        const Item *e = _getEntry(index, INVOKEDYNAMIC, "Indy");
        return e->invokeDynamic;
      }

      Index getIndexOfUtf8(const char *utf8);

      Index getIndexOfClass(const char *className);

      Index putUtf8(const char *utf8) {
        Index i = getIndexOfUtf8(utf8);
        if (i == NULLENTRY) {
          return addUtf8(utf8);
        } else {
          return i;
        }
      }

      Index putClass(const char *className) {
        Index i = getIndexOfClass(className);
        if (i == NULLENTRY) {
          i = addClass(className);
          classes[className] = i;
          return i;
        } else {
          return i;
        }
      }

      vector<Item> entries;

    private:
      template<class... TArgs>
      Index _addSingle(TArgs... args);

      template<class... TArgs>
      Index _addDoubleEntry(TArgs... args);

      const Item *_getEntry(Index i) const;

      const Item *_getEntry(Index index, u1 tag, const char *message) const;

      bool _isDoubleEntry(Index index) const {
        const Item *e = _getEntry(index);
        return e->tag == LONG || e->tag == DOUBLE;
      }

      void _getMemberRef(
          string *clazzName, string *name, string *desc, const MemberRef &memberRef
      ) const {
        Index classIndex = memberRef.classIndex;
        Index nameAndTypeIndex = memberRef.nameAndTypeIndex;

        *clazzName = getClassName(classIndex);
        getNameAndType(nameAndTypeIndex, name, desc);
      }

      map<string, Index> utf8s;
      map<string, Index> classes;
    };

    ostream &operator<<(ostream &os, const ConstPool::Tag &tag);

    /**
     * The Version class is a tuple containing a major and minor
     * version numbers fields.
     *
     * The values of the major and minor fields are the minor
     * and major version numbers of this class file.
     * Together, a major and a minor version number determine the version of the
     * class file format.
     * If a class file has major version number M and minor version number m,
     * we denote the version of its class file format as M.m.
     * Thus, class file format versions may be ordered lexicographically,
     * for example, 1.5 < 2.0 < 2.1.
     *
     * A Java Virtual Machine implementation can support a class file format of
     * version v if and only if v lies in some contiguous range Mi.0 <= v <= Mj.m.
     * The release level of the Java SE platform to which a Java Virtual Machine
     * implementation conforms is responsible for determining the range.
     */
    class Version {
    public:
      /**
       * Using default 51, which is supported by JDK 1.7.
       *
       * @param majorVersion
       * @param minorVersion
       */
      explicit Version(u2 majorVersion = 51, u2 minorVersion = 0);

      /**
       * @return The major version number.
       */
      u2 majorVersion() const;

      /**
       * @return The minor version number.
       */
      u2 minorVersion() const;

      /**
       * Taken from the official JVM specification.
       *
       * Oracle's Java Virtual Machine implementation in JDK release 1.0.2
       * supports class file format versions 45.0 through 45.3 inclusive.
       * JDK releases 1.1.* support class file format versions in the
       * range 45.0 through 45.65535 inclusive.
       * For k >= 2, JDK release 1.k supports class file format versions in
       * the range 45.0 through 44+k.0 inclusive.
       */
      string supportedByJdk() const;

      /**
       * Equals comparator.
       * @param lhs
       * @param rhs
       * @return
       */
      friend bool operator==(const Version &lhs, const Version &rhs);

      /**
       * Less comparator.
       * @param lhs
       * @param rhs
       * @return
       */
      friend bool operator<(const Version &lhs, const Version &rhs);

      /**
       * Less or equal comparator.
       * @param lhs
       * @param rha
       * @return
       */
      friend bool operator<=(const Version &lhs, const Version &rha);

      /**
       * Display this version.
       * @param os
       * @param version
       * @return
       */
      friend ostream &operator<<(ostream &os, const Version &version);

    private:
      u2 _major;
      u2 _minor;
    };

    /**
     * OPCODES constants definitions.
     *
     * This enumeration type was taken from
     *
     * http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-6.html
     * http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-7.html
     */
    // clang-format off
//   opc,  identifier,      name,            kind,            deprecated
#define BYTECODE_TABLE(XX)                                                                          \
  XX(0x0,  nop,             nop,             ZERO,            false)                               \
  XX(0x1,  aconst_null,     aconst_null,     ZERO,            false)                               \
  XX(0x2,  iconst_m1,       iconst_m1,       ZERO,            false)                               \
  XX(0x3,  iconst_0,        iconst_0,        ZERO,            false)                               \
  XX(0x4,  iconst_1,        iconst_1,        ZERO,            false)                               \
  XX(0x5,  iconst_2,        iconst_2,        ZERO,            false)                               \
  XX(0x6,  iconst_3,        iconst_3,        ZERO,            false)                               \
  XX(0x7,  iconst_4,        iconst_4,        ZERO,            false)                               \
  XX(0x8,  iconst_5,        iconst_5,        ZERO,            false)                               \
  XX(0x9,  lconst_0,        lconst_0,        ZERO,            false)                               \
  XX(0xa,  lconst_1,        lconst_1,        ZERO,            false)                               \
  XX(0xb,  fconst_0,        fconst_0,        ZERO,            false)                               \
  XX(0xc,  fconst_1,        fconst_1,        ZERO,            false)                               \
  XX(0xd,  fconst_2,        fconst_2,        ZERO,            false)                               \
  XX(0xe,  dconst_0,        dconst_0,        ZERO,            false)                               \
  XX(0xf,  dconst_1,        dconst_1,        ZERO,            false)                               \
  XX(0x10, bipush,          bipush,          BIPUSH,          false)                               \
  XX(0x11, sipush,          sipush,          SIPUSH,          false)                               \
  XX(0x12, ldc,             ldc,             LDC,             false)                               \
  XX(0x13, ldc_w,           ldc_w,           LDC,             false)                               \
  XX(0x14, ldc2_w,          ldc2_w,          LDC,             false)                               \
  XX(0x15, iload,           iload,           VAR,             false)                               \
  XX(0x16, lload,           lload,           VAR,             false)                               \
  XX(0x17, fload,           fload,           VAR,             false)                               \
  XX(0x18, dload,           dload,           VAR,             false)                               \
  XX(0x19, aload,           aload,           VAR,             false)                               \
  XX(0x1a, iload_0,         iload_0,         ZERO,            false)                               \
  XX(0x1b, iload_1,         iload_1,         ZERO,            false)                               \
  XX(0x1c, iload_2,         iload_2,         ZERO,            false)                               \
  XX(0x1d, iload_3,         iload_3,         ZERO,            false)                               \
  XX(0x1e, lload_0,         lload_0,         ZERO,            false)                               \
  XX(0x1f, lload_1,         lload_1,         ZERO,            false)                               \
  XX(0x20, lload_2,         lload_2,         ZERO,            false)                               \
  XX(0x21, lload_3,         lload_3,         ZERO,            false)                               \
  XX(0x22, fload_0,         fload_0,         ZERO,            false)                               \
  XX(0x23, fload_1,         fload_1,         ZERO,            false)                               \
  XX(0x24, fload_2,         fload_2,         ZERO,            false)                               \
  XX(0x25, fload_3,         fload_3,         ZERO,            false)                               \
  XX(0x26, dload_0,         dload_0,         ZERO,            false)                               \
  XX(0x27, dload_1,         dload_1,         ZERO,            false)                               \
  XX(0x28, dload_2,         dload_2,         ZERO,            false)                               \
  XX(0x29, dload_3,         dload_3,         ZERO,            false)                               \
  XX(0x2a, aload_0,         aload_0,         ZERO,            false)                               \
  XX(0x2b, aload_1,         aload_1,         ZERO,            false)                               \
  XX(0x2c, aload_2,         aload_2,         ZERO,            false)                               \
  XX(0x2d, aload_3,         aload_3,         ZERO,            false)                               \
  XX(0x2e, iaload,          iaload,          ZERO,            false)                               \
  XX(0x2f, laload,          laload,          ZERO,            false)                               \
  XX(0x30, faload,          faload,          ZERO,            false)                               \
  XX(0x31, daload,          daload,          ZERO,            false)                               \
  XX(0x32, aaload,          aaload,          ZERO,            false)                               \
  XX(0x33, baload,          baload,          ZERO,            false)                               \
  XX(0x34, caload,          caload,          ZERO,            false)                               \
  XX(0x35, saload,          saload,          ZERO,            false)                               \
  XX(0x36, istore,          istore,          VAR,             false)                               \
  XX(0x37, lstore,          lstore,          VAR,             false)                               \
  XX(0x38, fstore,          fstore,          VAR,             false)                               \
  XX(0x39, dstore,          dstore,          VAR,             false)                               \
  XX(0x3a, astore,          astore,          VAR,             false)                               \
  XX(0x3b, istore_0,        istore_0,        ZERO,            false)                               \
  XX(0x3c, istore_1,        istore_1,        ZERO,            false)                               \
  XX(0x3d, istore_2,        istore_2,        ZERO,            false)                               \
  XX(0x3e, istore_3,        istore_3,        ZERO,            false)                               \
  XX(0x3f, lstore_0,        lstore_0,        ZERO,            false)                               \
  XX(0x40, lstore_1,        lstore_1,        ZERO,            false)                               \
  XX(0x41, lstore_2,        lstore_2,        ZERO,            false)                               \
  XX(0x42, lstore_3,        lstore_3,        ZERO,            false)                               \
  XX(0x43, fstore_0,        fstore_0,        ZERO,            false)                               \
  XX(0x44, fstore_1,        fstore_1,        ZERO,            false)                               \
  XX(0x45, fstore_2,        fstore_2,        ZERO,            false)                               \
  XX(0x46, fstore_3,        fstore_3,        ZERO,            false)                               \
  XX(0x47, dstore_0,        dstore_0,        ZERO,            false)                               \
  XX(0x48, dstore_1,        dstore_1,        ZERO,            false)                               \
  XX(0x49, dstore_2,        dstore_2,        ZERO,            false)                               \
  XX(0x4a, dstore_3,        dstore_3,        ZERO,            false)                               \
  XX(0x4b, astore_0,        astore_0,        ZERO,            false)                               \
  XX(0x4c, astore_1,        astore_1,        ZERO,            false)                               \
  XX(0x4d, astore_2,        astore_2,        ZERO,            false)                               \
  XX(0x4e, astore_3,        astore_3,        ZERO,            false)                               \
  XX(0x4f, iastore,         iastore,         ZERO,            false)                               \
  XX(0x50, lastore,         lastore,         ZERO,            false)                               \
  XX(0x51, fastore,         fastore,         ZERO,            false)                               \
  XX(0x52, dastore,         dastore,         ZERO,            false)                               \
  XX(0x53, aastore,         aastore,         ZERO,            false)                               \
  XX(0x54, bastore,         bastore,         ZERO,            false)                               \
  XX(0x55, castore,         castore,         ZERO,            false)                               \
  XX(0x56, sastore,         sastore,         ZERO,            false)                               \
  XX(0x57, pop,             pop,             ZERO,            false)                               \
  XX(0x58, pop2,            pop2,            ZERO,            false)                               \
  XX(0x59, dup,             dup,             ZERO,            false)                               \
  XX(0x5a, dup_x1,          dup_x1,          ZERO,            false)                               \
  XX(0x5b, dup_x2,          dup_x2,          ZERO,            false)                               \
  XX(0x5c, dup2,            dup2,            ZERO,            false)                               \
  XX(0x5d, dup2_x1,         dup2_x1,         ZERO,            false)                               \
  XX(0x5e, dup2_x2,         dup2_x2,         ZERO,            false)                               \
  XX(0x5f, swap,            swap,            ZERO,            false)                               \
  XX(0x60, iadd,            iadd,            ZERO,            false)                               \
  XX(0x61, ladd,            ladd,            ZERO,            false)                               \
  XX(0x62, fadd,            fadd,            ZERO,            false)                               \
  XX(0x63, dadd,            dadd,            ZERO,            false)                               \
  XX(0x64, isub,            isub,            ZERO,            false)                               \
  XX(0x65, lsub,            lsub,            ZERO,            false)                               \
  XX(0x66, fsub,            fsub,            ZERO,            false)                               \
  XX(0x67, dsub,            dsub,            ZERO,            false)                               \
  XX(0x68, imul,            imul,            ZERO,            false)                               \
  XX(0x69, lmul,            lmul,            ZERO,            false)                               \
  XX(0x6a, fmul,            fmul,            ZERO,            false)                               \
  XX(0x6b, dmul,            dmul,            ZERO,            false)                               \
  XX(0x6c, idiv,            idiv,            ZERO,            false)                               \
  XX(0x6d, ldiv,            ldiv,            ZERO,            false)                               \
  XX(0x6e, fdiv,            fdiv,            ZERO,            false)                               \
  XX(0x6f, ddiv,            ddiv,            ZERO,            false)                               \
  XX(0x70, irem,            irem,            ZERO,            false)                               \
  XX(0x71, lrem,            lrem,            ZERO,            false)                               \
  XX(0x72, frem,            frem,            ZERO,            false)                               \
  XX(0x73, drem,            drem,            ZERO,            false)                               \
  XX(0x74, ineg,            ineg,            ZERO,            false)                               \
  XX(0x75, lneg,            lneg,            ZERO,            false)                               \
  XX(0x76, fneg,            fneg,            ZERO,            false)                               \
  XX(0x77, dneg,            dneg,            ZERO,            false)                               \
  XX(0x78, ishl,            ishl,            ZERO,            false)                               \
  XX(0x79, lshl,            lshl,            ZERO,            false)                               \
  XX(0x7a, ishr,            ishr,            ZERO,            false)                               \
  XX(0x7b, lshr,            lshr,            ZERO,            false)                               \
  XX(0x7c, iushr,           iushr,           ZERO,            false)                               \
  XX(0x7d, lushr,           lushr,           ZERO,            false)                               \
  XX(0x7e, iand,            iand,            ZERO,            false)                               \
  XX(0x7f, land,            land,            ZERO,            false)                               \
  XX(0x80, ior,             ior,             ZERO,            false)                               \
  XX(0x81, lor,             lor,             ZERO,            false)                               \
  XX(0x82, ixor,            ixor,            ZERO,            false)                               \
  XX(0x83, lxor,            lxor,            ZERO,            false)                               \
  XX(0x84, iinc,            iinc,            IINC,            false)                               \
  XX(0x85, i2l,             i2l,             ZERO,            false)                               \
  XX(0x86, i2f,             i2f,             ZERO,            false)                               \
  XX(0x87, i2d,             i2d,             ZERO,            false)                               \
  XX(0x88, l2i,             l2i,             ZERO,            false)                               \
  XX(0x89, l2f,             l2f,             ZERO,            false)                               \
  XX(0x8a, l2d,             l2d,             ZERO,            false)                               \
  XX(0x8b, f2i,             f2i,             ZERO,            false)                               \
  XX(0x8c, f2l,             f2l,             ZERO,            false)                               \
  XX(0x8d, f2d,             f2d,             ZERO,            false)                               \
  XX(0x8e, d2i,             d2i,             ZERO,            false)                               \
  XX(0x8f, d2l,             d2l,             ZERO,            false)                               \
  XX(0x90, d2f,             d2f,             ZERO,            false)                               \
  XX(0x91, i2b,             i2b,             ZERO,            false)                               \
  XX(0x92, i2c,             i2c,             ZERO,            false)                               \
  XX(0x93, i2s,             i2s,             ZERO,            false)                               \
  XX(0x94, lcmp,            lcmp,            ZERO,            false)                               \
  XX(0x95, fcmpl,           fcmpl,           ZERO,            false)                               \
  XX(0x96, fcmpg,           fcmpg,           ZERO,            false)                               \
  XX(0x97, dcmpl,           dcmpl,           ZERO,            false)                               \
  XX(0x98, dcmpg,           dcmpg,           ZERO,            false)                               \
  XX(0x99, ifeq,            ifeq,            JUMP,            false)                               \
  XX(0x9a, ifne,            ifne,            JUMP,            false)                               \
  XX(0x9b, iflt,            iflt,            JUMP,            false)                               \
  XX(0x9c, ifge,            ifge,            JUMP,            false)                               \
  XX(0x9d, ifgt,            ifgt,            JUMP,            false)                               \
  XX(0x9e, ifle,            ifle,            JUMP,            false)                               \
  XX(0x9f, if_icmpeq,       if_icmpeq,       JUMP,            false)                               \
  XX(0xa0, if_icmpne,       if_icmpne,       JUMP,            false)                               \
  XX(0xa1, if_icmplt,       if_icmplt,       JUMP,            false)                               \
  XX(0xa2, if_icmpge,       if_icmpge,       JUMP,            false)                               \
  XX(0xa3, if_icmpgt,       if_icmpgt,       JUMP,            false)                               \
  XX(0xa4, if_icmple,       if_icmple,       JUMP,            false)                               \
  XX(0xa5, if_acmpeq,       if_acmpeq,       JUMP,            false)                               \
  XX(0xa6, if_acmpne,       if_acmpne,       JUMP,            false)                               \
  XX(0xa7, GOTO,            goto,            JUMP,            false)                               \
  XX(0xa8, jsr,             jsr,             JUMP,            true)                                \
  XX(0xa9, ret,             ret,             VAR,             true)                                \
  XX(0xaa, tableswitch,     tableswitch,     TABLESWITCH,     false)                               \
  XX(0xab, lookupswitch,    lookupswitch,    LOOKUPSWITCH,    false)                               \
  XX(0xac, ireturn,         ireturn,         ZERO,            false)                               \
  XX(0xad, lreturn,         lreturn,         ZERO,            false)                               \
  XX(0xae, freturn,         freturn,         ZERO,            false)                               \
  XX(0xaf, dreturn,         dreturn,         ZERO,            false)                               \
  XX(0xb0, areturn,         areturn,         ZERO,            false)                               \
  XX(0xb1, RETURN,          return,          ZERO,            false)                               \
  XX(0xb2, getstatic,       getstatic,       FIELD,           false)                               \
  XX(0xb3, putstatic,       putstatic,       FIELD,           false)                               \
  XX(0xb4, getfield,        getfield,        FIELD,           false)                               \
  XX(0xb5, putfield,        putfield,        FIELD,           false)                               \
  XX(0xb6, invokevirtual,   invokevirtual,   INVOKE,          false)                               \
  XX(0xb7, invokespecial,   invokespecial,   INVOKE,          false)                               \
  XX(0xb8, invokestatic,    invokestatic,    INVOKE,          false)                               \
  XX(0xb9, invokeinterface, invokeinterface, INVOKEINTERFACE, false)                               \
  XX(0xba, invokedynamic,   invokedynamic,   INVOKEDYNAMIC,   false)                               \
  XX(0xbb, NEW,             new,             TYPE,            false)                               \
  XX(0xbc, newarray,        newarray,        NEWARRAY,        false)                               \
  XX(0xbd, anewarray,       anewarray,       TYPE,            false)                               \
  XX(0xbe, arraylength,     arraylength,     ZERO,            false)                               \
  XX(0xbf, athrow,          athrow,          ZERO,            false)                               \
  XX(0xc0, checkcast,       checkcast,       TYPE,            false)                               \
  XX(0xc1, instanceof,      instanceof,      TYPE,            false)                               \
  XX(0xc2, monitorenter,    monitorenter,    ZERO,            false)                               \
  XX(0xc3, monitorexit,     monitorexit,     ZERO,            false)                               \
  XX(0xc4, wide,            wide,            ZERO,            false)                               \
  XX(0xc5, multianewarray,  multianewarray,  MULTIARRAY,      false)                               \
  XX(0xc6, ifnull,          ifnull,          JUMP,            false)                               \
  XX(0xc7, ifnonnull,       ifnonnull,       JUMP,            false)                               \
  XX(0xc8, goto_w,          goto_w,          NOTIMPLEMENTED,  false)                               \
  XX(0xc9, jsr_w,           jsr_w,           NOTIMPLEMENTED,  true)                                \
  XX(0xca, breakpoint,      breakpoint,      RESERVED,        false)                               \
  XX(0xcb, reservedcb,      reservedcb,      RESERVED,        false)                               \
  XX(0xcc, reservedcc,      reservedcc,      RESERVED,        false)                               \
  XX(0xcd, reservedcd,      reservedcd,      RESERVED,        false)                               \
  XX(0xce, reservedce,      reservedce,      RESERVED,        false)                               \
  XX(0xcf, reservedcf,      reservedcf,      RESERVED,        false)                               \
  XX(0xd0, reservedd0,      reservedd0,      RESERVED,        false)                               \
  XX(0xd1, reservedd1,      reservedd1,      RESERVED,        false)                               \
  XX(0xd2, reservedd2,      reservedd2,      RESERVED,        false)                               \
  XX(0xd3, reservedd3,      reservedd3,      RESERVED,        false)                               \
  XX(0xd4, reservedd4,      reservedd4,      RESERVED,        false)                               \
  XX(0xd5, reservedd5,      reservedd5,      RESERVED,        false)                               \
  XX(0xd6, reservedd6,      reservedd6,      RESERVED,        false)                               \
  XX(0xd7, reservedd7,      reservedd7,      RESERVED,        false)                               \
  XX(0xd8, reservedd8,      reservedd8,      RESERVED,        false)                               \
  XX(0xd9, reservedd9,      reservedd9,      RESERVED,        false)                               \
  XX(0xda, reservedda,      reservedda,      RESERVED,        false)                               \
  XX(0xdb, reserveddb,      reserveddb,      RESERVED,        false)                               \
  XX(0xdc, reserveddc,      reserveddc,      RESERVED,        false)                               \
  XX(0xdd, reserveddd,      reserveddd,      RESERVED,        false)                               \
  XX(0xde, reservedde,      reservedde,      RESERVED,        false)                               \
  XX(0xdf, reserveddf,      reserveddf,      RESERVED,        false)                               \
  XX(0xe0, reservede0,      reservede0,      RESERVED,        false)                               \
  XX(0xe1, reservede1,      reservede1,      RESERVED,        false)                               \
  XX(0xe2, reservede2,      reservede2,      RESERVED,        false)                               \
  XX(0xe3, reservede3,      reservede3,      RESERVED,        false)                               \
  XX(0xe4, reservede4,      reservede4,      RESERVED,        false)                               \
  XX(0xe5, reservede5,      reservede5,      RESERVED,        false)                               \
  XX(0xe6, reservede6,      reservede6,      RESERVED,        false)                               \
  XX(0xe7, reservede7,      reservede7,      RESERVED,        false)                               \
  XX(0xe8, reservede8,      reservede8,      RESERVED,        false)                               \
  XX(0xe9, reservede9,      reservede9,      RESERVED,        false)                               \
  XX(0xea, reservedea,      reservedea,      RESERVED,        false)                               \
  XX(0xeb, reservedeb,      reservedeb,      RESERVED,        false)                               \
  XX(0xec, reservedec,      reservedec,      RESERVED,        false)                               \
  XX(0xed, reserveded,      reserveded,      RESERVED,        false)                               \
  XX(0xee, reservedee,      reservedee,      RESERVED,        false)                               \
  XX(0xef, reservedef,      reservedef,      RESERVED,        false)                               \
  XX(0xf0, reservedf0,      reservedf0,      RESERVED,        false)                               \
  XX(0xf1, reservedf1,      reservedf1,      RESERVED,        false)                               \
  XX(0xf2, reservedf2,      reservedf2,      RESERVED,        false)                               \
  XX(0xf3, reservedf3,      reservedf3,      RESERVED,        false)                               \
  XX(0xf4, reservedf4,      reservedf4,      RESERVED,        false)                               \
  XX(0xf5, reservedf5,      reservedf5,      RESERVED,        false)                               \
  XX(0xf6, reservedf6,      reservedf6,      RESERVED,        false)                               \
  XX(0xf7, reservedf7,      reservedf7,      RESERVED,        false)                               \
  XX(0xf8, reservedf8,      reservedf8,      RESERVED,        false)                               \
  XX(0xf9, reservedf9,      reservedf9,      RESERVED,        false)                               \
  XX(0xfa, reservedfa,      reservedfa,      RESERVED,        false)                               \
  XX(0xfb, reservedfb,      reservedfb,      RESERVED,        false)                               \
  XX(0xfc, reservedfc,      reservedfc,      RESERVED,        false)                               \
  XX(0xfd, reservedfd,      reservedfd,      RESERVED,        false)                               \
  XX(0xfe, impdep1,         impdep1,         RESERVED,        false)                               \
  XX(0xff, impdep2,         impdep2,         RESERVED,        false)

    // clang-format on

    enum class Opcode {
#define XX(opc, identifier, ...) identifier = opc,
      BYTECODE_TABLE(XX)
#undef XX
    };

    static std::string GetOpcodeName(Opcode opcode) {
      static std::string nameTable[256] = {
#define XX(a, b, name, ...) #name,
          BYTECODE_TABLE(XX)
#undef XX
      };
      return nameTable[static_cast<int>(opcode)];
    }

    ostream &operator<<(ostream &os, Opcode opcode);

    class IClassPath {
    public:
      virtual ~IClassPath() {}

      virtual string getCommonSuperClass(const string &className1, const string &className2) = 0;
    };

    class ClassFile;

    enum OpKind {
      KIND_ZERO,
      KIND_BIPUSH,
      KIND_SIPUSH,
      KIND_LDC,
      KIND_VAR,
      KIND_IINC,
      KIND_JUMP,
      KIND_TABLESWITCH,
      KIND_LOOKUPSWITCH,
      KIND_FIELD,
      KIND_INVOKE,
      KIND_INVOKEINTERFACE,
      KIND_INVOKEDYNAMIC,
      KIND_TYPE,
      KIND_NEWARRAY,
      KIND_MULTIARRAY,
      KIND_NOTIMPLEMENTED,
      KIND_RESERVED,
      KIND_LABEL,
      KIND_FRAME
    };

    static OpKind GetOpcodeKind(Opcode opcode) {
      static OpKind kindTable[256] = {
#define XX(a, b, c, kind, ...) KIND_##kind,
          BYTECODE_TABLE(XX)
#undef XX
      };
      return kindTable[static_cast<int>(opcode)];
    }

    /**
     * Represent a bytecode instruction.
     */
    class Inst {
      friend class LabelInst;

      friend class ZeroInst;

      friend class PushInst;

      friend class LdcInst;

      friend class VarInst;

      friend class IincInst;

      friend class WideInst;

      friend class JumpInst;

      friend class FieldInst;

      friend class InvokeInst;

      friend class InvokeInterfaceInst;

      friend class InvokeDynamicInst;

      friend class TypeInst;

      friend class NewArrayInst;

      friend class MultiArrayInst;

      friend class SwitchInst;

      friend class InstList;

    public:
      bool isJump() const { return kind == KIND_JUMP; }

      bool isTableSwitch() const { return kind == KIND_TABLESWITCH; }

      bool isLookupSwitch() const { return kind == KIND_LOOKUPSWITCH; }

      bool isBranch() const { return isJump() || isTableSwitch() || isLookupSwitch(); }

      bool isExit() const {
        return (opcode >= Opcode::ireturn && opcode <= Opcode::RETURN) || opcode == Opcode::athrow;
      }

      bool isLabel() const { return kind == KIND_LABEL; }

      bool isPush() const { return kind == KIND_BIPUSH || kind == KIND_SIPUSH; }

      bool isLdc() const { return kind == KIND_LDC; }

      bool isVar() const { return kind == KIND_VAR; }

      bool isIinc() const { return kind == KIND_IINC; }

      bool isInvoke() const { return kind == KIND_INVOKE; }

      /**
       * Returns true is this instruction is an invokeinterface instruction.
       * False otherwise.
       */
      bool isInvokeInterface() const { return kind == KIND_INVOKEINTERFACE; }

      /**
       * Returns true is this instruction is an invokedynamic instruction.
       * False otherwise.
       */
      bool isInvokeDynamic() const { return kind == KIND_INVOKEDYNAMIC; }

      bool isType() const { return kind == KIND_TYPE; }

      bool isNewArray() const { return kind == KIND_NEWARRAY; }

      bool isWide() const { return opcode == Opcode::wide && kind == KIND_ZERO; }

      bool isField() const { return kind == KIND_FIELD; }

      bool isMultiArray() const { return kind == KIND_MULTIARRAY; }

      //            bool isJsrOrRet() const {
      //                return opcode == Opcode::jsr || opcode == Opcode::jsr_w || opcode ==
      //                Opcode::ret;
      //            }

      /**
       * The opcode of this instruction.
       */
      const Opcode opcode;

      /**
       * The kind of this instruction.
       */
      const OpKind kind;

      int _offset;

      const ConstPool *const constPool;
      Inst *prev;
      Inst *next;

      class LabelInst *label() { return cast<LabelInst>(isLabel(), "label"); }

      class PushInst *push() { return cast<PushInst>(isPush(), "push"); }

      class LdcInst *ldc() { return cast<LdcInst>(isLdc(), "ldc"); }

      class VarInst *var() { return cast<VarInst>(isVar(), "var"); }

      class IincInst *iinc() { return cast<IincInst>(isIinc(), "iinc"); }

      class InvokeInst *invoke() { return cast<InvokeInst>(isInvoke(), "invoke"); }

      class JumpInst *jump() { return cast<JumpInst>(isJump(), "jump"); }

      class TableSwitchInst *ts() { return cast<TableSwitchInst>(isTableSwitch(), "ts"); }

      class LookupSwitchInst *ls() { return cast<LookupSwitchInst>(isLookupSwitch(), "ls"); }

      class InvokeInterfaceInst *invokeinterface() {
        return cast<InvokeInterfaceInst>(isInvokeInterface(), "invinter");
      }

      class TypeInst *type() { return cast<TypeInst>(isType(), "type"); }

      class NewArrayInst *newarray() { return cast<NewArrayInst>(isNewArray(), "newarray"); }

      class WideInst *wide() { return cast<WideInst>(isWide(), "wide"); }

      class FieldInst *field() { return cast<FieldInst>(isField(), "field"); }

      class MultiArrayInst *multiarray() {
        return cast<MultiArrayInst>(isMultiArray(), "multiarray");
      }

      class LabelInst *label() const { return cast<LabelInst>(isLabel(), "label"); }

      class PushInst *push() const { return cast<PushInst>(isPush(), "push"); }

      class LdcInst *ldc() const { return cast<LdcInst>(isLdc(), "ldc"); }

      class VarInst *var() const { return cast<VarInst>(isVar(), "var"); }

      class IincInst *iinc() const { return cast<IincInst>(isIinc(), "iinc"); }

      class InvokeInst *invoke() const { return cast<InvokeInst>(isInvoke(), "invoke"); }

      class JumpInst *jump() const { return cast<JumpInst>(isJump(), "jump"); }

      class TableSwitchInst *ts() const { return cast<TableSwitchInst>(isTableSwitch(), "ts"); }

      class LookupSwitchInst *ls() const { return cast<LookupSwitchInst>(isLookupSwitch(), "ls"); }

      class InvokeInterfaceInst *invokeinterface() const {
        return cast<InvokeInterfaceInst>(isInvokeInterface(), "invinter");
      }

      class InvokeDynamicInst *indy() const {
        return cast<InvokeDynamicInst>(isInvokeDynamic(), "indy");
      }

      class TypeInst *type() const { return cast<TypeInst>(isType(), "type"); }

      class NewArrayInst *newarray() const { return cast<NewArrayInst>(isNewArray(), "newarray"); }

      class WideInst *wide() const { return cast<WideInst>(isWide(), "wide"); }

      class FieldInst *field() const { return cast<FieldInst>(isField(), "field"); }

      class MultiArrayInst *multiarray() const {
        return cast<MultiArrayInst>(isMultiArray(), "multiarray");
      }

      virtual std::string toString() const { return GetOpcodeName(opcode); }

      set<Inst *> consumes;
      set<Inst *> produces;
      int id = 0;

    private:
      Inst() :
          opcode(Opcode::nop), kind(KIND_ZERO), _offset(0), constPool(nullptr), prev(nullptr),
          next(nullptr) {}

      Inst(
          Opcode opcode, OpKind kind, const ConstPool *constPool, Inst *prev = nullptr,
          Inst *next = nullptr
      ) : opcode(opcode), kind(kind), _offset(0), constPool(constPool), prev(prev), next(next) {}

      virtual ~Inst() = default;

      template<typename TKind>
      TKind *cast(bool cond, const char *kindName) {
        checkCast(cond, kindName);
        return (TKind *) this;
      }

      template<typename TKind>
      TKind *cast(bool cond, const char *kindName) const {
        checkCast(cond, kindName);
        return (TKind *) this;
      }

      void checkCast(bool cond, const char *kindName) const;
    };

    /**
     *
     */
    class LabelInst : public Inst {
      friend class InstList;

    public:
      LabelInst(ConstPool *constPool, int id) :
          Inst(Opcode::nop, KIND_LABEL, constPool), offset(0), deltaOffset(0), id(id),
          isBranchTarget(false), isTryStart(false), isCatchHandler(false) {}

      bool isTarget() const { return isBranchTarget || isTryStart || isCatchHandler; }

      u2 offset;
      u2 deltaOffset;
      int id;
      bool isBranchTarget;
      bool isTryStart;
      bool isCatchHandler;
    };

    /**
     *
     */
    class ZeroInst : public Inst {
      friend class InstList;

    public:
      ZeroInst(Opcode opcode, ConstPool *constPool) : Inst(opcode, KIND_ZERO, constPool) {}

      std::string toString() const override { return GetOpcodeName(opcode); }
    };

    /**
     *
     */
    class PushInst : public Inst {
      friend class InstList;

    public:
      PushInst(Opcode opcode, OpKind kind, int value, ConstPool *constPool) :
          Inst(opcode, kind, constPool), value(value) {}

      const int value;

      std::string toString() const override {
        return GetOpcodeName(opcode) + " " + std::to_string(value) + "// real value";
      }
    };

    /**
     *
     */
    class LdcInst : public Inst {
      friend class InstList;

    public:
      LdcInst(Opcode opcode, ConstPool::Index valueIndex, ConstPool *constPool) :
          Inst(opcode, KIND_LDC, constPool), valueIndex(valueIndex) {}

      const ConstPool::Index valueIndex;

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(valueIndex) + " // const index";
      }
    };

    /**
     *
     */
    class VarInst : public Inst {
      friend class InstList;

    public:
      VarInst(Opcode opcode, u1 lvindex, ConstPool *constPool) :
          Inst(opcode, KIND_VAR, constPool), lvindex(lvindex) {}

      const u1 lvindex;

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(lvindex) + " // local index";
      }
    };

    /**
     *
     */
    class IincInst : public Inst {
      friend class InstList;

    public:
      IincInst(u1 index, u1 value, ConstPool *constPool) :
          Inst(Opcode::iinc, KIND_IINC, constPool), index(index), value(value) {}

      const u1 index;
      const u1 value;

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(index) + " " + std::to_string(value) +
               " // local index and increment value";
      }
    };

    /**
     *
     */
    class WideInst : public Inst {
      friend class InstList;

    public:
      WideInst(Opcode subOpcode, u2 lvindex, ConstPool *constPool) :
          Inst(Opcode::wide, KIND_ZERO, constPool), subOpcode(subOpcode) {
        if (subOpcode == Opcode::ret) {
          throw Exception("Ret found in wide instruction!!!");
        }

        var.lvindex = lvindex;
      }

      WideInst(u2 index, u2 value, ConstPool *constPool) :
          Inst(Opcode::wide, KIND_ZERO, constPool), subOpcode(Opcode::iinc) {
        iinc.index = index;
        iinc.value = value;
      }

      const Opcode subOpcode;

      std::string toString() const override { return GetOpcodeName(opcode); }

      union {
        struct {
          u2 lvindex;
        } var;

        struct {
          u2 index;
          u2 value;
        } iinc;
      };
    };

    /**
     *
     */
    class JumpInst : public Inst {
      friend class InstList;

    public:
      JumpInst(Opcode opcode, LabelInst *targetLabel, ConstPool *constPool) :
          Inst(opcode, KIND_JUMP, constPool), label2(targetLabel) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " " + std::to_string(label2->_offset) + " // target offset";
      }

      const Inst *label2;
    };

    /**
     *
     */
    class FieldInst : public Inst {
      friend class InstList;

    public:
      FieldInst(Opcode opcode, ConstPool::Index fieldRefIndex, ConstPool *constPool) :
          Inst(opcode, KIND_FIELD, constPool), fieldRefIndex(fieldRefIndex) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(fieldRefIndex) + " // field index";
      }

      const ConstPool::Index fieldRefIndex;
    };

    /**
     *
     */
    class InvokeInst : public Inst {
      friend class InstList;

    public:
      InvokeInst(Opcode opcode, ConstPool::Index methodRefIndex, ConstPool *constPool) :
          Inst(opcode, KIND_INVOKE, constPool), methodRefIndex(methodRefIndex) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(methodRefIndex) + " // method index";
      }

      const ConstPool::Index methodRefIndex;
    };

    /**
     *
     */
    class InvokeInterfaceInst : public Inst {
      friend class InstList;

    public:
      InvokeInterfaceInst(ConstPool::Index interMethodRefIndex, u1 count, ConstPool *constPool) :
          Inst(Opcode::invokeinterface, KIND_INVOKEINTERFACE, constPool),
          interMethodRefIndex(interMethodRefIndex), count(count) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(interMethodRefIndex) +
               " // method index";
      }

      const u2 interMethodRefIndex;
      const u1 count;
    };

    /**
     * Represents an invokedynamic bytecode.
     */
    class InvokeDynamicInst : public Inst {
      friend class InstList;

    public:
      InvokeDynamicInst(ConstPool::Index callSite, ConstPool *constPool) :
          Inst(Opcode::invokedynamic, KIND_INVOKEDYNAMIC, constPool), _callSite(callSite) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(_callSite) + " // bootstrap index";
      }

      /**
       * Returns the call site for this invokedynamic instruction.
       */
      ConstPool::Index callSite() const { return _callSite; }

    private:
      ConstPool::Index _callSite;
    };

    /**
     *
     */
    class TypeInst : public Inst {
      friend class InstList;

    public:
      TypeInst(Opcode opcode, ConstPool::Index classIndex, ConstPool *constPool) :
          Inst(opcode, KIND_TYPE, constPool), classIndex(classIndex) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(classIndex) + " // class index";
      }

      /**
       * Index in the constant pool of a class entry.
       */
      ConstPool::Index classIndex;
    };

    /**
     *
     */
    class NewArrayInst : public Inst {
      friend class InstList;

    public:
      /**
       *
       */
      enum NewArrayType {
        NEWARRAYTYPE_BOOLEAN = 4,
        NEWARRAYTYPE_CHAR = 5,
        NEWARRAYTYPE_FLOAT = 6,
        NEWARRAYTYPE_DOUBLE = 7,
        NEWARRAYTYPE_BYTE = 8,
        NEWARRAYTYPE_SHORT = 9,
        NEWARRAYTYPE_INT = 10,
        NEWARRAYTYPE_LONG = 11
      };

      NewArrayInst(Opcode opcode, u1 atype, ConstPool *constPool) :
          Inst(opcode, KIND_NEWARRAY, constPool), atype(atype) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " " + std::to_string(atype) + " // element type";
      }

      u1 atype;
    };

    /**
     *
     */
    class MultiArrayInst : public Inst {
      friend class InstList;

    public:
      MultiArrayInst(Opcode opcode, ConstPool::Index classIndex, u1 dims, ConstPool *constPool) :
          Inst(opcode, KIND_MULTIARRAY, constPool), classIndex(classIndex), dims(dims) {}

      std::string toString() const override {
        return GetOpcodeName(opcode) + " #" + std::to_string(classIndex) + " " +
               std::to_string(dims) + " // class index, dims";
      }

      ConstPool::Index classIndex;
      u1 dims;
    };

    /**
     * Base class for TableSwitchInst and LookupSwitchInst.
     */
    class SwitchInst : public Inst {
      friend class TableSwitchInst;

      friend class LookupSwitchInst;

    public:
      vector<Inst *> targets;

      void addTarget(LabelInst *label) {
        targets.push_back(label);
        label->isBranchTarget = true;
      }

    private:
      SwitchInst(Opcode opcode, OpKind kind, ConstPool *constPool) :
          Inst(opcode, kind, constPool) {}
    };

    /**
     *
     */
    class TableSwitchInst : public SwitchInst {
      friend class InstList;

    public:
      TableSwitchInst(LabelInst *def, int low, int high, ConstPool *constPool) :
          SwitchInst(Opcode::tableswitch, KIND_TABLESWITCH, constPool), def(def), low(low),
          high(high) {}

      std::string toString() const override {
        std::ostringstream ss;
        ss << GetOpcodeName(opcode) + " " + std::to_string(low) + " " + std::to_string(high) +
                  " // low, high, default: " + std::to_string(def->_offset);
        if (targets.size() > 0) {
          ss << ", targets: [ ";
          for (const auto &target: targets) {
            ss << target->_offset << " ";
          }
          ss << "]";
        }

        return ss.str();
      }

      Inst *def;
      int low;
      int high;
    };

    /**
     *
     */
    class LookupSwitchInst : public SwitchInst {
      friend class InstList;

    public:
      LookupSwitchInst(LabelInst *def, u4 npairs, ConstPool *constPool) :
          SwitchInst(Opcode::lookupswitch, KIND_LOOKUPSWITCH, constPool), defbyte(def),
          npairs(npairs) {}

      std::string toString() const override {
        std::ostringstream ss;
        ss << GetOpcodeName(opcode) + " " + std::to_string(npairs) +
                  " // npairs, default: " + std::to_string(defbyte->_offset);
        if (targets.size() > 0) {
          ss << ", targets: [ ";
          for (const auto &target: targets) {
            ss << target->_offset << " ";
          }
          ss << "]";
        }

        return ss.str();
      }

      Inst *defbyte;
      u4 npairs;
      vector<u4> keys;
    };

    ostream &operator<<(ostream &os, const Inst &inst);

    /**
     * Represents the bytecode of a method.
     */
    class InstList {
      friend class CodeAttr;

    public:
      class Iterator {
        friend class InstList;

      public:
        Inst *operator*();

        Inst *operator->() const;

        bool friend operator==(const Iterator &lhs, const Iterator &rhs) {
          return lhs.position == rhs.position;
        }

        bool friend operator!=(const Iterator &lhs, const Iterator &rhs) {
          return lhs.position != rhs.position;
        }

        Iterator &operator++();

        Iterator &operator--();

      private:
        Iterator(Inst *position, Inst *last) : position(position), last(last) {}

        Inst *position;
        Inst *last;
      };

      LabelInst *createLabel();

      void addLabel(LabelInst *inst, Inst *pos = nullptr);

      LabelInst *addLabel(Inst *pos = nullptr);

      ZeroInst *addZero(Opcode opcode, Inst *pos = nullptr);

      PushInst *addBiPush(u1 value, Inst *pos = nullptr);

      PushInst *addSiPush(u2 value, Inst *pos = nullptr);

      LdcInst *addLdc(Opcode opcode, ConstPool::Index valueIndex, Inst *pos = nullptr);

      VarInst *addVar(Opcode opcode, u1 lvindex, Inst *pos = nullptr);

      IincInst *addIinc(u1 index, u1 value, Inst *pos = nullptr);

      WideInst *addWideVar(Opcode varOpcode, u2 lvindex, Inst *pos = nullptr);

      WideInst *addWideIinc(u2 index, u2 value, Inst *pos = nullptr);

      JumpInst *addJump(Opcode opcode, LabelInst *targetLabel, Inst *pos = nullptr);

      FieldInst *addField(Opcode opcode, ConstPool::Index fieldRefIndex, Inst *pos = nullptr);

      InvokeInst *addInvoke(Opcode opcode, ConstPool::Index methodRefIndex, Inst *pos = nullptr);

      InvokeInterfaceInst *
      addInvokeInterface(ConstPool::Index interMethodRefIndex, u1 count, Inst *pos = nullptr);

      InvokeDynamicInst *addInvokeDynamic(ConstPool::Index callSite, Inst *pos = nullptr);

      TypeInst *addType(Opcode opcode, ConstPool::Index classIndex, Inst *pos = nullptr);

      NewArrayInst *addNewArray(u1 atype, Inst *pos = nullptr);

      MultiArrayInst *addMultiArray(ConstPool::Index classIndex, u1 dims, Inst *pos = nullptr);

      TableSwitchInst *addTableSwitch(LabelInst *def, int low, int high, Inst *pos = nullptr);

      LookupSwitchInst *addLookupSwitch(LabelInst *def, u4 npairs, Inst *pos = nullptr);

      bool hasBranches() const { return branchesCount > 0; }

      bool hasJsrOrRet() const { return jsrOrRet; }

      int size() const { return _size; }

      Iterator begin() const { return Iterator(first, last); }

      Iterator end() const { return Iterator(nullptr, last); }

      Inst *getInst(int offset);

      std::string toString(bool offset = false) const {
        std::ostringstream oss;
        for (Inst *i: *this) {
          if (i->isLabel()) {
            continue;
          }
          if (offset) {
            oss << i->_offset << ": ";
          }
          oss << i->toString() << "\n";
        }
        return oss.str();
      }

      ClassFile *const constPool;

    private:
      InstList(ClassFile *arena) :
          constPool(arena), first(nullptr), last(nullptr), _size(0), nextLabelId(1),
          branchesCount(0), jsrOrRet(false) {}

      ~InstList();

      void addInst(Inst *inst, Inst *pos);

      Inst *first;
      Inst *last;

      int _size;

      int nextLabelId;

      int branchesCount;

      bool jsrOrRet;

      template<typename TInst, typename... TArgs>
      TInst *_create(const TArgs &...args);
    };

    ostream &operator<<(ostream &os, const InstList &instList);

    enum TypeTag {
      TYPE_TOP = 0,
      TYPE_INTEGER = 1,
      TYPE_FLOAT = 2,
      TYPE_LONG = 4,
      TYPE_DOUBLE = 3,
      TYPE_NULL = 5,
      TYPE_UNINITTHIS = 6,
      TYPE_OBJECT = 7,
      TYPE_UNINIT = 8,
      TYPE_VOID,
      TYPE_BOOLEAN,
      TYPE_BYTE,
      TYPE_CHAR,
      TYPE_SHORT,
    };

    /**
     * Verification type class
     */
    class Type {
      friend class TypeFactory;

    public:
      bool operator==(const Type &other) const {
        return tag == other.tag && (tag != TYPE_OBJECT || className == other.className) &&
               dims == other.dims;
      }

      friend bool operator!=(const Type &lhs, const Type &rhs) { return !(lhs == rhs); }

      bool isTop() const { return tag == TYPE_TOP && !isArray(); }

      /**
       * Returns true when this type is exactly int type.
       * False otherwise.
       */
      bool isInt() const { return tag == TYPE_INTEGER && !isArray(); }

      /**
       * Returns true when this type is an
       * integral type, i.e., int, boolean, byte, char, short.
       * False otherwise.
       */
      bool isIntegral() const {
        switch (tag) {
          case TYPE_INTEGER:
          case TYPE_BOOLEAN:
          case TYPE_BYTE:
          case TYPE_CHAR:
          case TYPE_SHORT:
            return !isArray();
          default:
            return false;
        }
      }

      bool isFloat() const { return tag == TYPE_FLOAT && !isArray(); }

      bool isLong() const { return tag == TYPE_LONG && !isArray(); }

      bool isDouble() const { return tag == TYPE_DOUBLE && !isArray(); }

      bool isNull() const { return tag == TYPE_NULL; }

      bool isUninitThis() const { return tag == TYPE_UNINITTHIS; }

      bool isUninit() const { return tag == TYPE_UNINIT; }

      bool isObject() const { return tag == TYPE_OBJECT || isArray(); }

      bool isArray() const { return dims > 0; }

      bool isVoid() const { return tag == TYPE_VOID; }

      bool isOneWord() const {
        return isIntegral() || isFloat() || isNull() || isObject() || isUninitThis();
      }

      bool isTwoWord() const { return isLong() || isDouble(); }

      bool isOneOrTwoWord() const { return isOneWord() || isTwoWord(); }

      bool isClass() const { return isObject() && !isArray(); }

      string getClassName() const;

      u2 getCpIndex() const;

      void setCpIndex(u2 index) {
        // check(isObject(), "Type is not object type to get cp index: ", *this);
        classIndex = index;
      }

      u4 getDims() const { return dims; }

      /**
       * Returns the element type of this array type. Requires that this type
       * is an array type. The return type is the same base type but with a less
       * dimension.
       *
       * For example, assuming that this type represents [[[I, then the result
       * value is [[I.
       *
       * @returns the element type of this array.
       */
      Type elementType() const;

      /**
       * Removes the any dimension on this type. This type has to be an array
       * type.
       *
       * For example, assuming that this type represents [[[I, then the result
       * value is I.
       *
       * @returns the base type of this type. The result ensures that is not an
       * array type.
       */
      Type stripArrayType() const;

      union {
        mutable struct {
          short offset;
          Inst *label;
          TypeInst *newinst;
        } uninit;
      };

      mutable bool init;

      mutable long typeId;

      static long nextTypeId;

      TypeTag tag;
      u4 dims;
      u2 classIndex;
      string className;

    private:
      Type(TypeTag tag) : init(true), typeId(0), tag(tag), dims(0), classIndex(0) {}

      Type(TypeTag tag, short offset, Inst *label) :
          init(true), typeId(0), tag(tag), dims(0), classIndex(0) {
        uninit.offset = offset;
        uninit.label = label;
      }

      Type(TypeTag tag, const string &className, u2 classIndex = 0) :
          init(true), typeId(0), tag(tag), dims(0), classIndex(classIndex), className(className) {}

      Type(const Type &other, u4 dims) {
        this->dims = dims;

        // Type(other)
        uninit.offset = other.uninit.offset;
        uninit.label = other.uninit.label;
        uninit.newinst = other.uninit.newinst;

        init = other.init;
        typeId = other.typeId;
        tag = other.tag;
        classIndex = other.classIndex;
        className = other.className;
      }
    };

    ostream &operator<<(ostream &os, const Type &type);

    class TypeFactory {
      friend class Type;

    public:
      static const Type &topType() { return _topType; }

      static const Type &intType() { return _intType; }

      static const Type &floatType() { return _floatType; }

      static const Type &longType() { return _longType; }

      static const Type &doubleType() { return _doubleType; }

      static const Type &booleanType() { return _booleanType; }

      static const Type &byteType() { return _byteType; }

      static const Type &charType() { return _charType; }

      static const Type &shortType() { return _shortType; }

      static const Type &nullType() { return _nullType; }

      static const Type &voidType() { return _voidType; }

      static Type uninitThisType();

      static Type uninitType(short offset, class Inst *label);

      static Type objectType(const string &className, u2 cpindex = 0);

      static Type arrayType(const Type &baseType, u4 dims);

      /**
       * Parses the const class name.
       *
       * @param className the class name to parse.
       * @returns the type that represents the class name.
       */
      static Type fromConstClass(const string &className);

      /**
       * Parses a field descriptor.
       *
       * @param fieldDesc the field descriptor to parse.
       * @returns the type that represents the field descriptor.
       */
      static Type fromFieldDesc(const char *&fieldDesc);

      /**
       * Parses a method descriptor.
       *
       * @param methodDesc the method descriptor to parse.
       * @param argsType collection of method arguments of methodDesc.
       * @returns the type that represents the return type of methodDesc.
       */
      static Type fromMethodDesc(const char *methodDesc, vector<Type> *argsType);

    private:
      static Type _topType;
      static Type _intType;
      static Type _floatType;
      static Type _longType;
      static Type _doubleType;
      static Type _booleanType;
      static Type _byteType;
      static Type _charType;
      static Type _shortType;
      static Type _nullType;
      static Type _voidType;

      static Type _parseBaseType(const char *&fieldDesc, const char *originalFieldDesc);

      static Type _getType(const char *&fieldDesc, const char *originalFieldDesc, int dims);

      static Type _getReturnType(const char *&methodDesc);
    };

    enum AttrKind {
      ATTR_UNKNOWN,
      ATTR_SOURCEFILE,
      ATTR_SIGNATURE,
      ATTR_CODE,
      ATTR_EXCEPTIONS,
      ATTR_LVT,
      ATTR_LVTT,
      ATTR_LNT,
      ATTR_SMT
    };

    /// Defines the base class for all attributes in the class file.
    class Attr {

      Attr(const Attr &) = delete;

      Attr(Attr &&) = delete;

      Attr &operator=(const Attr &) = delete;

    public:
      AttrKind kind;

      u2 nameIndex;
      u4 len;
      ClassFile *const constPool;

      virtual ~Attr() {}

    protected:
      Attr(AttrKind kind, u2 nameIndex, u4 len, ClassFile *constPool) :
          kind(kind), nameIndex(nameIndex), len(len), constPool(constPool) {}
    };

    /**
     * Represents a collection of attributes within a class, method or field
     * or even with another attributes, e.g., CodeAttr.
     */
    class Attrs {
    public:
      Attrs(const Attrs &) = delete;

      Attrs(Attrs &&) = delete;

      Attrs &operator=(const Attrs &) = delete;

      Attrs() {}

      virtual ~Attrs();

      Attr *add(Attr *attr) {
        attrs.push_back(attr);

        return attr;
      }

      u2 size() const { return attrs.size(); }

      const Attr &operator[](u2 index) const { return *attrs[index]; }

      vector<Attr *>::iterator begin() { return attrs.begin(); }

      vector<Attr *>::iterator end() { return attrs.end(); }

      vector<Attr *>::const_iterator begin() const { return attrs.begin(); }

      vector<Attr *>::const_iterator end() const { return attrs.end(); }

      vector<Attr *> attrs;
    };

    /**
     * Represents an unknown opaque attribute to jnif.
     */
    class UnknownAttr : public Attr {
    public:
      const u1 *const data;

      UnknownAttr(u2 nameIndex, u4 len, const u1 *data, ClassFile *constPool) :
          Attr(ATTR_UNKNOWN, nameIndex, len, constPool), data(data) {}
    };

    /**
     * Represents the LineNumberTable attribute within the Code attribute.
     */
    class LvtAttr : public Attr {
    public:
      struct LvEntry {
        u2 startPc;
        Inst *startPcLabel;

        Inst *endPcLabel;

        u2 len;
        u2 varNameIndex;
        u2 varDescIndex;
        u2 index;
      };

      vector<LvEntry> lvt;

      LvtAttr(AttrKind kind, u2 nameIndex, ClassFile *constPool) :
          Attr(kind, nameIndex, 0, constPool) {}
    };

    /**
     * Represents the LineNumberTable attribute within the Code attribute.
     */
    class LntAttr : public Attr {
    public:
      LntAttr(u2 nameIndex, ClassFile *constPool) : Attr(ATTR_LNT, nameIndex, 0, constPool) {}

      struct LnEntry {
        u2 startpc;
        Inst *startPcLabel;

        u2 lineno;
      };

      vector<LnEntry> lnt;
    };

    /**
     * Represents the StackMapTable attribute within the Code attribute.
     */
    class SmtAttr : public Attr {
    public:
      SmtAttr(u2 nameIndex, ClassFile *constPool) : Attr(ATTR_SMT, nameIndex, 0, constPool) {}

      class Entry {
      public:
        int frameType;
        Inst *label;

        struct {
        } sameFrame;

        struct {
          vector<Type> stack; // [1]
        } sameLocals_1_stack_item_frame;

        struct {
          short offset_delta;
          vector<Type> stack; // [1]
        } same_locals_1_stack_item_frame_extended;

        struct {
          short offset_delta;
        } chop_frame;

        struct {
          short offset_delta;
        } same_frame_extended;

        struct {
          short offset_delta;
          vector<Type> locals; // frameType - 251
        } append_frame;

        struct {
          short offset_delta;
          vector<Type> locals;
          vector<Type> stack;
        } full_frame;
      };

      vector<Entry> entries;
    };

    /**
     * Represents the Exceptions attribute.
     */
    class ExceptionsAttr : public Attr {
    public:
      ExceptionsAttr(u2 nameIndex, ClassFile *constPool, const vector<u2> &es) :
          Attr(ATTR_EXCEPTIONS, nameIndex, es.size() * 2 + 2, constPool), es(es) {}

      vector<ConstPool::Index> es;
    };

    /**
     * Represent the Code attribute of a method.
     */
    class CodeAttr : public Attr {
    public:
      CodeAttr(u2 nameIndex, ClassFile *constPool) :
          Attr(ATTR_CODE, nameIndex, 0, constPool), maxStack(0), maxLocals(0), codeLen(-1),
          instList(constPool), cfg(nullptr) {}

      ~CodeAttr();

      /**
       * Gives the maximum depth of the operand stack of this
       * method at any point during execution of the method.
       */
      u2 maxStack;
      u2 maxLocals;
      u4 codeLen;

      InstList instList;

      bool hasTryCatch() const { return exceptions.size() > 0; }

      struct ExceptionHandler {
        const LabelInst *const startpc;
        const LabelInst *const endpc;
        const LabelInst *const handlerpc;
        const ConstPool::Index catchtype;
      };

      vector<ExceptionHandler> exceptions;

      class ControlFlowGraph *cfg;

      Attrs attrs;
    };

    class SignatureAttr : public Attr {
    public:
      const ConstPool::Index signatureIndex;

      SignatureAttr(
          ConstPool::Index nameIndex, ConstPool::Index signatureIndex, ClassFile *constPool
      ) : Attr(ATTR_SIGNATURE, nameIndex, 2, constPool), signatureIndex(signatureIndex) {}

      const char *signature() const;
    };

    class SourceFileAttr : public Attr {
    public:
      const ConstPool::Index sourceFileIndex;

      SourceFileAttr(
          ConstPool::Index nameIndex, ConstPool::Index sourceFileIndex, ClassFile *constPool
      ) : Attr(ATTR_SOURCEFILE, nameIndex, 2, constPool), sourceFileIndex(sourceFileIndex) {}

      const char *sourceFile() const;
    };

    class Signature {
    public:
      Signature(const Attrs *attrs) : attrs(attrs) {}

      bool hasSignature() const {
        for (Attr *attr: *attrs) {
          if (attr->kind == ATTR_SIGNATURE) {
            return true;
          }
        }

        return false;
      }

      const char *signature() const {
        for (Attr *attr: *attrs) {
          if (attr->kind == ATTR_SIGNATURE) {
            return ((SignatureAttr *) attr)->signature();
          }
        }

        return nullptr;
      }

    private:
      const Attrs *attrs;
    };

    /// Represent a member of a class. This the base class for Field and
    /// Method classes.
    /// @see Field
    /// @see Method
    class Member {
    public:
      Member(const Member &) = delete;

      Member(Member &&) = delete;

      Member &operator=(const Member &) = delete;

      Member &operator=(Member &&) = delete;

      friend class Field;

      friend class Method;

      const u2 accessFlags;
      const ConstPool::Index nameIndex;
      const ConstPool::Index descIndex;
      const ConstPool &constPool;
      Attrs attrs;
      Signature sig;

      const char *getName() const;

      const char *getDesc() const;

    private:
      Member(
          u2 accessFlags, ConstPool::Index nameIndex, ConstPool::Index descIndex,
          const ConstPool &constPool
      );
    };

    class Field : public Member {
    public:
      Field(const Field &) = delete;

      Field(Field &&) = delete;

      Field &operator=(const Field &) = delete;

      Field &operator=(Field &&) = delete;

      /// Access flags used by fields.
      enum Flags {

        /// Declared public; may be accessed from outside its package.
        PUBLIC = 0x0001,

        /// Declared private; usable only within the defining class.
        PRIVATE = 0x0002,

        /// Declared protected; may be accessed within subclasses.
        PROTECTED = 0x0004,

        /// Declared static.
        STATIC = 0x0008,

        /// Declared final;never directly assigned to after object construction.
        FINAL = 0x0010,

        /// Declared volatile; cannot be cached.
        VOLATILE = 0x0040,

        /// Declared transient; not written/read by a persistent object manager.
        TRANSIENT = 0x0080,

        /// Declared synthetic; not present in the source code.
        SYNTHETIC = 0x1000,

        /// Declared as an element of an enum.
        ENUM = 0x4000
      };

      Field(
          u2 accessFlags, ConstPool::Index nameIndex, ConstPool::Index descIndex,
          const ConstPool &constPool
      ) : Member(accessFlags, nameIndex, descIndex, constPool) {}
    };

    /// Represents a Java method.
    class Method : public Member {
    public:
      Method(const Method &) = delete;

      Method(Method &&) = delete;

      Method &operator=(const Method &) = delete;

      Method &operator=(Method &&) = delete;

      /// Access flags used by methods.
      enum Flags {

        /// Declared public; may be accessed from outside its package.
        PUBLIC = 0x0001,

        /// Declared private; accessible only within the defining class.
        PRIVATE = 0x0002,

        /// Declared protected; may be accessed within subclasses.
        PROTECTED = 0x0004,

        /// Declared static.
        STATIC = 0x0008,

        /// Declared final; must not be overridden (see 5.4.5).
        FINAL = 0x0010,

        /// Declared synchronized; invocation is wrapped by a monitor use.
        SYNCHRONIZED = 0x0020,

        /// A bridge method, generated by the compiler.
        BRIDGE = 0x0040,

        /// Declared with variable number of arguments.
        VARARGS = 0x0080,

        /// Declared native; implemented in a language other than Java.
        NATIVE = 0x0100,

        /// Declared abstract; no implementation is provided.
        ABSTRACT = 0x0400,

        /// Declared strictfp; floating-point mode is FP-strict.
        STRICT = 0x0800,

        /// Declared synthetic; not present in the source code.
        SYNTHETIC = 0x1000,

      };

      Method(
          u2 accessFlags, ConstPool::Index nameIndex, ConstPool::Index descIndex,
          const ConstPool &constPool
      ) : Member(accessFlags, nameIndex, descIndex, constPool) {}

      bool hasCode() const {
        for (Attr *attr: attrs) {
          if (attr->kind == ATTR_CODE) {
            return true;
          }
        }

        return false;
      }

      CodeAttr *codeAttr() const {
        for (Attr *attr: attrs) {
          if (attr->kind == ATTR_CODE) {
            return (CodeAttr *) attr;
          }
        }

        return nullptr;
      }

      InstList &instList();

      bool isPublic() const { return accessFlags & PUBLIC; }

      bool isPrivate() const { return accessFlags & PRIVATE; }

      bool isFinal() const { return accessFlags & FINAL; }

      bool isStatic() const { return accessFlags & STATIC; }

      bool isAbstract() const { return accessFlags & ABSTRACT; }

      bool isNative() const { return accessFlags & NATIVE; }

      bool isSynchronized() const { return accessFlags & SYNCHRONIZED; }

      bool isSynthetic() const { return accessFlags & SYNTHETIC; }

      bool isInit() const;

      bool isMain() const;

      /**
       * Shows this method in the specified ostream.
       */
      friend ostream &operator<<(ostream &os, const Method &m);
    };

    /**
     * Models a Java Class File following the specification of the JVM version 7.
     */
    class ClassFile : public ConstPool {
    public:
      ClassFile(const ClassFile &) = delete;

      ClassFile(ClassFile &&) = delete;

      ClassFile &operator=(const ClassFile &) = delete;

      ClassFile &operator=(ClassFile &&) = delete;

      /**
       * Access flags for the class itself.
       */
      enum Flags {

        /**
         * Declared public; may be accessed from outside its package.
         */
        PUBLIC = 0x0001,

        /// Declared final; no subclasses allowed.
        FINAL = 0x0010,

        /// Treat superclass methods specially when invoked by the
        /// invokespecial instruction.
        SUPER = 0x0020,

        /// Is an interface, not a class.
        INTERFACE = 0x0200,

        /// Declared abstract; must not be instantiated.
        ABSTRACT = 0x0400,

        /// Declared synthetic; not present in the source code.
        SYNTHETIC = 0x1000,

        /// Declared as an annotation type.
        ANNOTATION = 0x2000,

        /// Declared as an enum type.
        ENUM = 0x4000
      };

      /**
       * The magic number signature that must appear at the beginning of each
       * class file, identifying the class file format;
       * it has the value 0xCAFEBABE.
       */
      static constexpr const u4 MAGIC = 0xcafebabe;

      /**
       * Java's root class
       */
      static constexpr const char *OBJECT = "java/lang/Object";

      /**
       * The constructed ClassFile has no class name neither super class name.
       */
      ClassFile();

      /**
       * Constructs a default class file given the class name, the super class
       * name and the access flags.
       * @param className
       * @param superClassName
       * @param accessFlags
       * @param version
       */
      explicit ClassFile(
          const char *className, const char *superClassName = OBJECT, u2 accessFlags = PUBLIC,
          Version version = Version()
      );

      /**
       * Gets the class name of this class file.
       *
       * @return The class name of this class file.
       */
      const char *getThisClassName() const;

      /**
       * Gets the class name of the super class of this class file.
       *
       * @return The super class name.
       */
      const char *getSuperClassName() const;

      /**
       * Determines whether this class file is an interface.
       *
       * @return true when this class is an interface.
       * Otherwise false.
       */
      bool isInterface() { return accessFlags & INTERFACE; }

      /**
       * Adds a new field to this class file.
       *
       * @param nameIndex the utf8 index in the constant pool that contains the
       * name of the field to add.

       * @param descIndex the utf8 index in the constant pool that contains the
       * descriptor of the field to add.
       * @param accessFlags the access flags of the field to add.
       * @returns the newly created field.
       */
      Field &addField(
          ConstPool::Index nameIndex, ConstPool::Index descIndex, u2 accessFlags = Field::PUBLIC
      );

      /**
       * Adds a new field to this class file by passing directly the name
       * and descriptor.
       *
       * @param fieldName the name of the field to add.
       * @param fieldDesc the descriptor of the field to add.
       * @param accessFlags the access flags of the field to add.
       * @returns the newly created field.
       */
      Field &
      addField(const char *fieldName, const char *fieldDesc, u2 accessFlags = Field::PUBLIC) {
        ConstPool::Index nameIndex = addUtf8(fieldName);
        ConstPool::Index descIndex = addUtf8(fieldDesc);

        return addField(nameIndex, descIndex, accessFlags);
      }

      /**
       * Adds a new method to this class file.
       *
       * @param nameIndex the utf8 index in the constant pool that contains the
       * name of the method to add.
       * @param descIndex the utf8 index in the constant pool that contains the
       * descriptor of the method to add.
       * @param accessFlags the access flags of the field to add.
       * @returns the newly created method.
       */
      Method &addMethod(
          ConstPool::Index nameIndex, ConstPool::Index descIndex, u2 accessFlags = Method::PUBLIC
      );

      /**
       * Adds a new method to this class file by passing directly the name
       * and descriptor.
       *
       * @param methodName the name of the method to add.
       * @param methodDesc the descriptor of the method to add.
       * @param accessFlags the access flags of the method to add.
       * @returns the newly created method.
       */
      Method &
      addMethod(const char *methodName, const char *methodDesc, u2 accessFlags = Method::PUBLIC) {
        ConstPool::Index nameIndex = addUtf8(methodName);
        ConstPool::Index descIndex = addUtf8(methodDesc);

        return addMethod(nameIndex, descIndex, accessFlags);
      }

      list<Method>::iterator getMethod(const char *methodName);

      /**
       * Computes the size in bytes of this class file of the in-memory
       * representation. This may change the attributes of this class file
       * if they are not correct.
       */
      u4 computeSize();

      /**
       * Computes the StackMapTable attributes for this class file.
       */
      void computeFrames(IClassPath *classPath);

      /**
       * Writes this class file in the specified buffer according to the
       * specification.
       */
      void write(u1 *classFileData, int classFileLen);

      /**
       * Writes this class file to the specified file.
       */
      void write(const std::string &file);

      /**
       * Export this class file to dot format.
       *
       * @see www.graphviz.org
       */
      void dot(ostream &os) const;

      // Must be the first member, as it is needed for the destructors of the other members.
      Arena _arena;

      ConstPool::Index thisClassIndex = ConstPool::NULLINDEX;
      ConstPool::Index superClassIndex = ConstPool::NULLINDEX;
      u2 accessFlags = PUBLIC;
      Version version;
      list<ConstPool::Index> interfaces;
      list<Field> fields;
      list<Method> methods;
      Attrs attrs;
      Signature sig;
    };

    ostream &operator<<(ostream &os, const ClassFile &classFile);


  } // namespace model

  using namespace model;

  class Frame {

    // Frame(const Frame&) = delete;

  public:
    Frame() : valid(false), topsErased(false), maxStack(0) { lva.reserve(256); }

    Type pop(Inst *inst);

    Type popOneWord(Inst *inst);

    Type popTwoWord(Inst *inst);

    Type popIntegral(Inst *inst);

    Type popFloat(Inst *inst);

    Type popLong(Inst *inst);

    Type popDouble(Inst *inst);

    Type popRef(Inst *inst) {
      Type t = popOneWord(inst);
      // assert(t.is(), "invalid ref type on top of the stack");
      return t;
    }

    Type popArray(Inst *inst) { return popOneWord(inst); }

    void popType(const Type &type, Inst *inst);

    void pushType(const Type &type, Inst *inst);

    void push(const Type &t, Inst *inst);

    void pushInt(Inst *inst) { push(TypeFactory::intType(), inst); }

    void pushLong(Inst *inst) {
      push(TypeFactory::topType(), inst);
      push(TypeFactory::longType(), inst);
    }

    void pushFloat(Inst *inst) { push(TypeFactory::floatType(), inst); }

    void pushDouble(Inst *inst) {
      push(TypeFactory::topType(), inst);
      push(TypeFactory::doubleType(), inst);
    }

    void pushRef(const string &className, Inst *inst) {
      push(TypeFactory::objectType(className), inst);
    }

    void pushArray(const Type &type, u4 dims, Inst *inst) {
      push(TypeFactory::arrayType(type, dims), inst);
    }

    void pushNull(Inst *inst) { push(TypeFactory::nullType(), inst); }

    Type getVar(u4 lvindex, Inst *inst);

    void setVar(u4 *lvindex, const Type &t, Inst *inst);

    void setVar2(u4 lvindex, const Type &t, Inst *inst);

    void setIntVar(u4 lvindex, Inst *inst) { setVar(&lvindex, TypeFactory::intType(), inst); }

    void setLongVar(u4 lvindex, Inst *inst) { setVar(&lvindex, TypeFactory::longType(), inst); }

    void setFloatVar(u4 lvindex, Inst *inst) { setVar(&lvindex, TypeFactory::floatType(), inst); }

    void setDoubleVar(u4 lvindex, Inst *inst) { setVar(&lvindex, TypeFactory::doubleType(), inst); }

    void setRefVar(u4 lvindex, const string &className, Inst *inst) {
      setVar(&lvindex, TypeFactory::objectType(className), inst);
    }

    void setRefVar(u4 lvindex, const Type &type, Inst *inst);

    void clearStack() { stack.clear(); }

    void cleanTops();

    void join(Frame &how, class jnif::model::IClassPath *classPath);

    void init(const Type &type);

    typedef pair<Type, set<Inst *>> T;

    vector<T> lva;
    list<T> stack;
    bool valid;
    bool topsErased;

    unsigned long maxStack;

    friend bool operator==(const Frame &lhs, const Frame &rhs) {
      return lhs.lva == rhs.lva && lhs.stack == rhs.stack && lhs.valid == rhs.valid;
    }

  private:
    void _setVar(u4 lvindex, const Type &t, Inst *inst);
  };

  ostream &operator<<(ostream &os, const Frame &frame);

  /**
   * Represents a basic block of instructions.
   *
   * @see Inst
   */
  class BasicBlock {
  public:
    BasicBlock(const BasicBlock &) = delete;

    BasicBlock(BasicBlock &&) = default;

    friend class ControlFlowGraph;

    void addTarget(BasicBlock *target);

    model::InstList::Iterator start;
    model::InstList::Iterator exit;
    string name;
    Frame in;
    Frame out;

    vector<BasicBlock *>::iterator begin() { return targets.begin(); }

    vector<BasicBlock *>::iterator end() { return targets.end(); }

    BasicBlock *next = nullptr;

    class ControlFlowGraph *const cfg;

    const Inst *last = nullptr;

    vector<BasicBlock *> targets;
    vector<BasicBlock *> ins;

    const BasicBlock *dom = nullptr;

  private:
    BasicBlock(
        InstList::Iterator &start, InstList::Iterator &exit, const string &name,
        class ControlFlowGraph *cfg
    ) : start(start), exit(exit), name(name), cfg(cfg) {}
  };

  /**
   * Represents a control flow graph of instructions.
   */
  class ControlFlowGraph {
    friend struct Dominator;

  public:
    vector<BasicBlock *> basicBlocks;

  public:
    static constexpr const char *EntryName = ".Entry";
    static constexpr const char *ExitName = ".Exit";

    BasicBlock *const entry;

    BasicBlock *const exit;

    const InstList &instList;

    explicit ControlFlowGraph(InstList &instList);

    ~ControlFlowGraph();

    /**
     * Adds a basic block to this control flow graph.
     *
     * @param start the start of the basic block.
     * @param end the end of the basic block.
     * @param name the name of the basic block to add.
     * @returns the newly created basic block added to this control flow graph.
     */
    BasicBlock *addBasicBlock(InstList::Iterator start, InstList::Iterator end, const string &name);

    /**
     * Finds the basic block associated with the given labelId.
     *
     * @param labelId the label id to search in the basic blocks.
     * @returns the basic block that starts at label with labelId. If the label
     * id is not found throws an exception.
     */
    BasicBlock *findBasicBlockOfLabel(int labelId) const;

    vector<BasicBlock *>::iterator begin() { return basicBlocks.begin(); }

    vector<BasicBlock *>::iterator end() { return basicBlocks.end(); }

    vector<BasicBlock *>::const_iterator begin() const { return basicBlocks.begin(); }

    vector<BasicBlock *>::const_iterator end() const { return basicBlocks.end(); }

    typedef map<BasicBlock *, set<BasicBlock *>> D;

    D dominance(BasicBlock *start);

  private:
    BasicBlock *addConstBb(InstList &instList, const char *name) {
      return addBasicBlock(instList.end(), instList.end(), name);
    }
  };

  ostream &operator<<(ostream &os, BasicBlock &bb);

  ostream &operator<<(ostream &os, const ControlFlowGraph &cfg);

  namespace parser {

    class ClassFileParser : public model::ClassFile {
    public:
      explicit ClassFileParser(const u1 *data, u4 len);

      static void parse(const u1 *data, u4 len, ClassFile *classFile);
    };

  } // namespace parser

  namespace stream {

    /**
     * Represents a ClassFile created from a disk file.
     */
    class IClassFileStream : public model::ClassFile {
    public:
      /**
       * Constructs a ClassFile parsing the specified fileName from disk.
       *
       * @param fileName The name of the file to parse.
       */
      explicit IClassFileStream(const char *fileName);
    };

    /**
     * Represents a ClassFile written to a disk file.
     */
    class OClassFileStream {
    public:
      explicit OClassFileStream(const char *fileName, ClassFile *classFile);
    };
  } // namespace stream

  namespace jar {

    class JarException {
    public:
      explicit JarException(const char *message) : message(message) {}

      const char *message;
    };

    class JarFile {
    public:
      typedef void (*ZipCallback)(
          void *args, int jarid, void *buffer, int size, const char *fileNameInZip
      );

      explicit JarFile(const char *zipPath);

      ~JarFile();

      int forEach(void *args, int jarid, ZipCallback callback);

    private:
      int _extractCurrentFile(void *args, int jarid, ZipCallback callback);

    private:
      void *_uf;
    };

  } // namespace jar

  /**
   *
   */
  class ClassHierarchy {
  public:
    //	IClassFinder* finder;
    //
    //	ClassHierarchy(IClassFinder* finder) :
    //			finder(finder) {
    //	}

    /**
     *
     */
    class ClassEntry {
    public:
      string className;
      string superClassName;
      // std::vector<String> interfaces;
    };

    /**
     *
     */
    // ClassFile* defineClass(const ClassFile& classFile);
    /**
     *
     */
    void addClass(const ClassFile &classFile);

    const string &getSuperClass(const string &className) const;

    bool isAssignableFrom(const string &sub, const string &sup) const;

    bool isDefined(const string &className) const;

    //	std::list<ClassEntry>::iterator begin() {
    //		return classes.begin();
    //	}
    //
    //	std::list<ClassEntry>::iterator end() {
    //		return classes.end();
    //	}
    //
    //	std::list<ClassEntry>::const_iterator begin() const {
    //		return classes.begin();
    //	}
    //
    //	std::list<ClassEntry>::const_iterator end() const {
    //		return classes.end();
    //	}

    friend ostream &operator<<(ostream &os, const ClassHierarchy &ch);

  private:
    // list<ClassEntry> classes;

    map<string, ClassEntry> classes;

    map<string, ClassEntry>::const_iterator getEntry(const string &className) const;
  };

  typedef map<BasicBlock *, set<BasicBlock *>> DomMap;

  template<class TDir>
  struct Dom : DomMap {

    Dom(const ControlFlowGraph &cfg) {
      for (BasicBlock *bb: cfg) {
        (*this)[bb].insert(cfg.basicBlocks.begin(), cfg.basicBlocks.end());
      }

      bool changed = true;
      while (changed) {
        changed = false;

        for (BasicBlock *bb: cfg) {
          set<BasicBlock *> ns;
          for (BasicBlock *p: TDir::dir(bb)) {
            if (ns.empty()) {
              ns = (*this)[p];
            } else {
              for (auto it = ns.begin(); it != ns.end();) {
                if ((*this)[p].count(*it) == 0) {
                  it = ns.erase(it);
                } else {
                  ++it;
                }
              }
            }
          }
          ns.insert(bb);

          if (ns != (*this)[bb]) {
            changed = true;
            (*this)[bb] = ns;
          }
        }
      }
    }
  };

  template<class TDir>
  struct SDom : Dom<TDir> {
    SDom(const ControlFlowGraph &cfg) : Dom<TDir>(cfg) {
      int count = this->erase(TDir::start(cfg));
      JnifError::asserts(count == 1, "count: ", count);

      for (auto &d: *this) {
        int count = d.second.erase(d.first);
        JnifError::asserts(count == 1, "count: ", count);
      }
    }
  };

  template<class TDir>
  struct IDom : DomMap {
    IDom(SDom<TDir> &ds) {
      for (pair<BasicBlock *const, set<BasicBlock *>> &d: ds) {
        JnifError::asserts(!d.second.empty(), "Empty: ", d.first->name);

        set<BasicBlock *> sdomBy = d.second;
        for (BasicBlock *bb: d.second) {
          if (ds.find(bb) != ds.end()) {
            for (BasicBlock *dd: ds[bb]) {
              sdomBy.erase(dd);
            }
          }
        }

        JnifError::asserts(sdomBy.size() == 1, "dom size: ", sdomBy.size());
        (*this)[*sdomBy.begin()].insert(d.first);
      }
    }
  };

  struct Forward {
    static vector<BasicBlock *> &dir(BasicBlock *bb) { return bb->ins; }

    static BasicBlock *start(const ControlFlowGraph &cfg) { return cfg.entry; }
  };

  struct Backward {
    static vector<BasicBlock *> &dir(BasicBlock *bb) { return bb->targets; }

    static BasicBlock *start(const ControlFlowGraph &cfg) { return cfg.exit; }
  };

  ostream &operator<<(ostream &os, const DomMap &ds);

} // namespace jnif

#endif
