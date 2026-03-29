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

#ifndef REIFY_ARGUMENT_HPP
#define REIFY_ARGUMENT_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "json.hpp"
#include "lib/dbgutils.hpp"

namespace symir {
  class VarDef;
}

// Forward declaration of helper function to avoid circular dependency
std::string GetVarDefTypeName(const symir::VarDef *varDef);

// Tag types for constructor disambiguation
struct ArrayDimTag {};

inline constexpr ArrayDimTag ArrayDim{};

template<typename IntType>
class ArgPlus;

/// Type hierarchy for ArgPlus following: ty -> IntType | Array[n] ty | Struct {field: ty, ...}
template<typename IntType>
struct ArgType {
  virtual ~ArgType() = default;
  virtual bool IsScalar() const = 0;
  virtual bool IsArray() const = 0;
  virtual bool IsStruct() const = 0;
  virtual size_t getSize() const = 0;
  virtual std::string GetTypeCastStr(const symir::VarDef *varDef) const = 0;
  virtual int GetNumLeaves() const = 0;
  virtual nlohmann::json ToJson() const = 0;
};

/// Scalar type: represents a single IntType value
template<typename IntType>
struct ScalarType : public ArgType<IntType> {
  IntType value;

  explicit ScalarType(IntType v = 0) : value(v) {}

  bool IsScalar() const override { return true; }

  bool IsArray() const override { return false; }

  bool IsStruct() const override { return false; }

  size_t getSize() const override { return 1; }

  std::string GetTypeCastStr(const symir::VarDef *varDef) const override { return ""; }

  int GetNumLeaves() const override { return 1; }

  nlohmann::json ToJson() const override {
    auto obj = nlohmann::json::object();
    obj["kind"] = "scalar";
    obj["value"] = value;
    return obj;
  }
};

/// Array type: represents a single-dimension array of ArgPlus elements
/// Multi-dimensional arrays are represented by nesting: int[2][3] = ArrayType(dim=2,
/// elements=[ArrayType(dim=3, ...), ArrayType(dim=3, ...)])
template<typename IntType>
struct ArrayType : public ArgType<IntType> {
  int dim; // Length of this dimension
  std::vector<ArgPlus<IntType>> elements;

  explicit ArrayType(int d);

  bool IsScalar() const override { return false; }

  bool IsArray() const override { return true; }

  bool IsStruct() const override { return false; }

  size_t getSize() const override { 
    size_t res = 0;
    for (const auto& ele : elements) res += ele.getSize();
    return res;
  }

  std::string GetTypeCastStr(const symir::VarDef *varDef) const override;

  int GetNumLeaves() const override;

  nlohmann::json ToJson() const override;
};

/// Struct type: represents a struct with named fields
template<typename IntType>
struct StructType : public ArgType<IntType> {
  std::string structName;
  std::vector<std::string> fieldNames;
  std::vector<ArgPlus<IntType>> fields;

  explicit StructType(std::string sName, std::vector<std::string> fNames);

  bool IsScalar() const override { return false; }

  bool IsArray() const override { return false; }

  bool IsStruct() const override { return true; }

  size_t getSize() const override {
    size_t res = 0;
    for (const auto& field : fields) res += field.getSize();
    return res;
  }

  std::string GetTypeCastStr(const symir::VarDef *varDef) const override;

  int GetNumLeaves() const override;

  nlohmann::json ToJson() const override;
};

/// ArgPlus: A type-safe container for function arguments following proper type hierarchy
template<typename IntType>
class ArgPlus {
  // Allow ArgType classes to access private members
  friend struct ArgType<IntType>;
  friend struct ScalarType<IntType>;
  friend struct ArrayType<IntType>;
  friend struct StructType<IntType>;

public:
  /// Default constructor: creates a scalar with value 0
  explicit ArgPlus() : type(std::make_unique<ScalarType<IntType>>(0)) {}

  /// Constructor for scalar values
  explicit ArgPlus(IntType v) : type(std::make_unique<ScalarType<IntType>>(v)) {}

  /// Constructor for array types (with tag to disambiguate from scalar constructor)
  explicit ArgPlus(ArrayDimTag, int d) : type(std::make_unique<ArrayType<IntType>>(d)) {}

  /// Constructor for struct types
  explicit ArgPlus(std::string sName, std::vector<std::string> fNames) :
      type(std::make_unique<StructType<IntType>>(std::move(sName), std::move(fNames))) {}

  /// Copy constructor
  ArgPlus(const ArgPlus &other);

  /// Move constructor
  ArgPlus(ArgPlus &&other) noexcept = default;

  /// Copy assignment
  ArgPlus &operator=(const ArgPlus &other);

  /// Move assignment
  ArgPlus &operator=(ArgPlus &&other) noexcept = default;

  // Type queries
  [[nodiscard]] bool IsScalar() const { return type->IsScalar(); }

  [[nodiscard]] bool IsArray() const { return type->IsArray(); }

  [[nodiscard]] bool IsStruct() const { return type->IsStruct(); }

  [[nodiscard]] bool getSize() const { return type->getSize(); }

  // Backward compatibility alias
  [[nodiscard]] bool IsVector() const { return IsArray(); }

  // Scalar operations
  [[nodiscard]] IntType GetValue() const {
    Assert(IsScalar(), "Cannot get scalar value for non-scalar type");
    return static_cast<ScalarType<IntType> *>(type.get())->value;
  }

  void SetValue(IntType v) {
    Assert(IsScalar(), "Cannot set scalar value for non-scalar type");
    static_cast<ScalarType<IntType> *>(type.get())->value = v;
  }

  // Flat indexing operations (for accessing nested structures linearly)
  [[nodiscard]] IntType GetValue(int at) const;

  void SetValue(int at, IntType v);

  // Array-specific operations
  [[nodiscard]] int GetVecNumDims() const {
    Assert(IsArray(), "Cannot get dimensions for non-array type");
    auto *arr = static_cast<ArrayType<IntType> *>(type.get());
    // Count this dimension plus dimensions of nested arrays
    if (!arr->elements.empty() && arr->elements[0].IsArray()) {
      return 1 + arr->elements[0].GetVecNumDims();
    }
    return 1;
  }

  [[nodiscard]] int GetVecDimLen(int d) const {
    Assert(IsArray(), "Cannot get dimension length for non-array type");
    auto *arr = static_cast<ArrayType<IntType> *>(type.get());
    if (d == 0) {
      return arr->dim;
    }
    // Recurse into elements for deeper dimensions
    Assert(!arr->elements.empty() && arr->elements[0].IsArray(), "Dimension index out of bounds");
    return arr->elements[0].GetVecDimLen(d - 1);
  }

  [[nodiscard]] std::vector<int> GetVecShape() const {
    Assert(IsArray(), "Cannot get shape for non-array type");
    std::vector<int> shape;
    const ArgPlus *curr = this;
    while (curr->IsArray()) {
      auto *arr = static_cast<ArrayType<IntType> *>(curr->type.get());
      shape.push_back(arr->dim);
      if (!arr->elements.empty()) {
        curr = &arr->elements[0];
      } else {
        break;
      }
    }
    return shape;
  }

  [[nodiscard]] IntType GetValue(const std::vector<int> &indices) const;

  [[nodiscard]] int GetVecNumEls(int dim = -1) const;

  // Struct-specific operations
  [[nodiscard]] const std::string &GetStructName() const {
    Assert(IsStruct(), "Cannot get struct name for non-struct type");
    return static_cast<StructType<IntType> *>(type.get())->structName;
  }

  [[nodiscard]] const std::vector<std::string> &GetFieldNames() const {
    Assert(IsStruct(), "Cannot get field names for non-struct type");
    return static_cast<StructType<IntType> *>(type.get())->fieldNames;
  }

  void SetStructName(std::string name) {
    Assert(IsStruct(), "Cannot set struct name for non-struct type");
    static_cast<StructType<IntType> *>(type.get())->structName = std::move(name);
  }

  // Element access (for arrays and structs)
  [[nodiscard]] size_t NumChildren() const;

  ArgPlus &operator[](int i);

  const ArgPlus &operator[](int i) const;

  // Code generation
  [[nodiscard]] std::string ToCxStr() const;
  [[nodiscard]] std::string ToCxStrWithReplaced(std::map<size_t, std::pair<std::string, int32_t> *> replacers, size_t offset = 0) const;

  [[nodiscard]] std::string GetTypeCastStr(const symir::VarDef *varDef) const {
    return type->GetTypeCastStr(varDef);
  }

  // Serialization
  [[nodiscard]] nlohmann::json ToJson() const { return type->ToJson(); }

  static ArgPlus FromJson(const nlohmann::json &obj);

private:
  std::unique_ptr<ArgType<IntType>> type;

  // Helper for flat recursive access
  IntType GetFlatRecursive(int target, int &current) const;
  bool SetFlatRecursive(int target, IntType v, int &current);
};

// ============================================================================
// ArrayType Implementation
// ============================================================================

template<typename IntType>
ArrayType<IntType>::ArrayType(int d) : dim(d) {
  elements.resize(d);
}

template<typename IntType>
std::string ArrayType<IntType>::GetTypeCastStr(const symir::VarDef *varDef) const {
  std::ostringstream oss;
  oss << "(";

  // Forward declare to avoid circular dependency issues
  extern std::string GetVarDefTypeName(const symir::VarDef *varDef);
  oss << GetVarDefTypeName(varDef);

  // Recursively collect all array dimensions
  std::function<void(const ArgPlus<IntType> *)> collectDims = [&](const ArgPlus<IntType> *arg) {
    if (arg->IsArray()) {
      auto *arr = static_cast<ArrayType<IntType> *>(arg->type.get());
      oss << "[" << arr->dim << "]";
      // Recurse into first element to get nested array dimensions
      if (!arr->elements.empty()) {
        collectDims(&arr->elements[0]);
      }
    }
  };

  // Start collection from this array - emit this level's dimension
  oss << "[" << dim << "]";
  // Recurse into first element for nested dimensions
  if (!elements.empty()) {
    collectDims(&elements[0]);
  }

  oss << ")";
  return oss.str();
}

template<typename IntType>
int ArrayType<IntType>::GetNumLeaves() const {
  int total = 0;
  for (const auto &el: elements) {
    total += el.type->GetNumLeaves();
  }
  return total;
}

template<typename IntType>
nlohmann::json ArrayType<IntType>::ToJson() const {
  auto obj = nlohmann::json::object();
  obj["kind"] = "array";
  obj["dim"] = dim;
  auto arr = nlohmann::json::array();
  for (const auto &el: elements) {
    arr.push_back(el.ToJson());
  }
  obj["elements"] = arr;
  return obj;
}

// ============================================================================
// StructType Implementation
// ============================================================================

template<typename IntType>
StructType<IntType>::StructType(std::string sName, std::vector<std::string> fNames) :
    structName(std::move(sName)), fieldNames(std::move(fNames)) {
  fields.resize(fieldNames.size());
}

template<typename IntType>
std::string StructType<IntType>::GetTypeCastStr(const symir::VarDef *varDef) const {
  return ""; // Structs don't need type cast in compound literals
}

template<typename IntType>
int StructType<IntType>::GetNumLeaves() const {
  int total = 0;
  for (const auto &f: fields) {
    total += f.type->GetNumLeaves();
  }
  return total;
}

template<typename IntType>
nlohmann::json StructType<IntType>::ToJson() const {
  auto obj = nlohmann::json::object();
  obj["kind"] = "struct";
  obj["structName"] = structName;
  obj["fieldNames"] = fieldNames;
  auto arr = nlohmann::json::array();
  for (const auto &f: fields) {
    arr.push_back(f.ToJson());
  }
  obj["fields"] = arr;
  return obj;
}

// ============================================================================
// ArgPlus Implementation
// ============================================================================

template<typename IntType>
ArgPlus<IntType>::ArgPlus(const ArgPlus &other) {
  if (other.IsScalar()) {
    type =
        std::make_unique<ScalarType<IntType>>(*static_cast<ScalarType<IntType> *>(other.type.get())
        );
  } else if (other.IsArray()) {
    auto *arr = static_cast<ArrayType<IntType> *>(other.type.get());
    auto newArr = std::make_unique<ArrayType<IntType>>(arr->dim);
    newArr->elements = arr->elements; // Copy elements
    type = std::move(newArr);
  } else if (other.IsStruct()) {
    auto *str = static_cast<StructType<IntType> *>(other.type.get());
    auto newStr = std::make_unique<StructType<IntType>>(str->structName, str->fieldNames);
    newStr->fields = str->fields; // Copy fields
    type = std::move(newStr);
  }
}

template<typename IntType>
ArgPlus<IntType> &ArgPlus<IntType>::operator=(const ArgPlus &other) {
  if (this != &other) {
    ArgPlus temp(other);
    type = std::move(temp.type);
  }
  return *this;
}

template<typename IntType>
IntType ArgPlus<IntType>::GetFlatRecursive(int target, int &current) const {
  if (IsScalar()) {
    if (current == target)
      return static_cast<ScalarType<IntType> *>(type.get())->value;
    current++;
    return 0;
  }

  auto &children = IsArray() ? static_cast<ArrayType<IntType> *>(type.get())->elements
                             : static_cast<StructType<IntType> *>(type.get())->fields;

  for (const auto &c: children) {
    int num = c.type->GetNumLeaves();
    if (current + num > target)
      return c.GetFlatRecursive(target, current);
    current += num;
  }
  return 0;
}

template<typename IntType>
bool ArgPlus<IntType>::SetFlatRecursive(int target, IntType v, int &current) {
  if (IsScalar()) {
    if (current == target) {
      static_cast<ScalarType<IntType> *>(type.get())->value = v;
      return true;
    }
    current++;
    return false;
  }

  auto &children = IsArray() ? static_cast<ArrayType<IntType> *>(type.get())->elements
                             : static_cast<StructType<IntType> *>(type.get())->fields;

  for (auto &c: children) {
    int num = c.type->GetNumLeaves();
    if (current + num > target)
      return c.SetFlatRecursive(target, v, current);
    current += num;
  }
  return false;
}

template<typename IntType>
IntType ArgPlus<IntType>::GetValue(int at) const {
  int current = 0;
  return GetFlatRecursive(at, current);
}

template<typename IntType>
void ArgPlus<IntType>::SetValue(int at, IntType v) {
  int current = 0;
  if (!SetFlatRecursive(at, v, current))
    Panic("Failed to set value at index %d", at);
}

template<typename IntType>
IntType ArgPlus<IntType>::GetValue(const std::vector<int> &indices) const {
  Assert(IsArray(), "Cannot get multi-dim value for non-array type");

  // Recursively index through nested arrays
  const ArgPlus *curr = this;
  for (size_t i = 0; i < indices.size(); i++) {
    Assert(curr->IsArray(), "Not enough dimensions for provided indices");
    auto *arr = static_cast<ArrayType<IntType> *>(curr->type.get());
    Assert(indices[i] >= 0 && indices[i] < arr->dim, "Index out of bounds");
    curr = &arr->elements[indices[i]];
  }

  // Navigate to scalar if we have a nested structure
  while (!curr->IsScalar() && curr->NumChildren() > 0) {
    curr = &(*curr)[0];
  }
  return curr->GetValue();
}

template<typename IntType>
int ArgPlus<IntType>::GetVecNumEls(int dim) const {
  // Scalars have 1 element
  if (IsScalar()) {
    if (dim == -1)
      return 1;
    Panic("Cannot get dimension %d for scalar type", dim);
  }

  // Structs: return total number of leaf elements (no dimensions)
  if (IsStruct()) {
    if (dim == -1) {
      return type->GetNumLeaves();
    }
    Panic("Cannot get dimension %d for struct type", dim);
  }

  // Arrays: handle dimensional access
  auto *arr = static_cast<ArrayType<IntType> *>(type.get());
  const int numDims = GetVecNumDims();

  if (dim == -1) {
    int total = 0;
    for (const auto &el: arr->elements)
      total += el.type->GetNumLeaves();
    return total;
  }

  Assert(dim >= 0 && dim < numDims, "Dimension out of bound");

  // Calculate number of elements from dimension 'dim' onwards
  // This is the product of all dimensions starting from 'dim'
  int num = GetVecDimLen(dim);
  for (int i = dim + 1; i < numDims; i++) {
    num *= GetVecDimLen(i);
  }

  // Multiply by the number of leaf elements in the innermost type
  if (!arr->elements.empty()) {
    const ArgPlus *curr = &arr->elements[0];
    for (int i = 1; i < numDims; i++) {
      if (curr->IsArray()) {
        auto *innerArr = static_cast<ArrayType<IntType> *>(curr->type.get());
        if (!innerArr->elements.empty()) {
          curr = &innerArr->elements[0];
        }
      }
    }
    num *= curr->type->GetNumLeaves();
  }

  return num;
}

template<typename IntType>
size_t ArgPlus<IntType>::NumChildren() const {
  if (IsArray()) {
    return static_cast<ArrayType<IntType> *>(type.get())->elements.size();
  } else if (IsStruct()) {
    return static_cast<StructType<IntType> *>(type.get())->fields.size();
  }
  return 0;
}

template<typename IntType>
ArgPlus<IntType> &ArgPlus<IntType>::operator[](int i) {
  if (IsArray()) {
    return static_cast<ArrayType<IntType> *>(type.get())->elements[i];
  } else if (IsStruct()) {
    return static_cast<StructType<IntType> *>(type.get())->fields[i];
  }
  Panic("Cannot index into scalar type");
}

template<typename IntType>
const ArgPlus<IntType> &ArgPlus<IntType>::operator[](int i) const {
  if (IsArray()) {
    return static_cast<ArrayType<IntType> *>(type.get())->elements[i];
  } else if (IsStruct()) {
    return static_cast<StructType<IntType> *>(type.get())->fields[i];
  }
  Panic("Cannot index into scalar type");
}

template<typename IntType>
std::string ArgPlus<IntType>::ToCxStr() const {
  if (IsScalar())
    return std::to_string(GetValue());

  std::ostringstream oss;
  if (IsStruct()) {
    auto *str = static_cast<StructType<IntType> *>(type.get());
    oss << "(struct " << str->structName << "){";
    for (size_t i = 0; i < str->fieldNames.size(); ++i) {
      oss << "." << str->fieldNames[i] << " = " << str->fields[i].ToCxStr();
      if (i != str->fieldNames.size() - 1)
        oss << ", ";
    }
    oss << "}";
  } else {
    auto *arr = static_cast<ArrayType<IntType> *>(type.get());
    oss << "{";
    for (size_t i = 0; i < arr->elements.size(); i++) {
      oss << arr->elements[i].ToCxStr();
      if (i != arr->elements.size() - 1)
        oss << ", ";
    }
    oss << "}";
  }
  return oss.str();
}

template<typename IntType>
std::string ArgPlus<IntType>::ToCxStrWithReplaced(std::map<size_t, std::pair<std::string, int32_t> *> replacers, size_t offset) const {
  if (IsScalar()) {
    if (replacers.contains(offset)) {
      const auto& replacer = *replacers[offset];
      return replacer.first + " + " + std::to_string(replacer.second);
    } else {
      return std::to_string(GetValue());
    }
  }

  std::ostringstream oss;
  if (IsStruct()) {
    auto *str = static_cast<StructType<IntType> *>(type.get());
    oss << "(struct " << str->structName << "){";
    size_t newOffset = offset;
    for (size_t i = 0; i < str->fieldNames.size(); ++i) {
      oss << "." << str->fieldNames[i] << " = " << str->fields[i].ToCxStrWithReplaced(replacers, newOffset);
      newOffset += str->fields[i].getSize();
      if (i != str->fieldNames.size() - 1)
        oss << ", ";
    }
    oss << "}";
  } else {
    auto *arr = static_cast<ArrayType<IntType> *>(type.get());
    oss << "{";
    size_t newOffset = offset;
    for (size_t i = 0; i < arr->elements.size(); i++) {
      oss << arr->elements[i].ToCxStrWithReplaced(replacers, newOffset);
      newOffset += arr->elements[i].getSize();
      if (i != arr->elements.size() - 1)
        oss << ", ";
    }
    oss << "}";
  }
  return oss.str();
}

template<typename IntType>
ArgPlus<IntType> ArgPlus<IntType>::FromJson(const nlohmann::json &obj) {
  if (!obj.contains("kind")) {
    // Legacy format support
    if (obj.contains("isScalar") && obj["isScalar"].get<bool>()) {
      return ArgPlus(obj["val"].get<IntType>());
    }
    if (obj.contains("children")) {
      ArgPlus res;
      if (obj.contains("shape")) {
        // Old format with flat shape vector - convert to nested structure
        // For now we'll just use the first dimension as a simple conversion
        std::vector<int> shape = obj["shape"].get<std::vector<int>>();
        Assert(!shape.empty(), "Empty shape in legacy format");
        res = ArgPlus(ArrayDim, shape[0]);
        auto *arr = static_cast<ArrayType<IntType> *>(res.type.get());
        for (size_t i = 0; i < obj["children"].size(); ++i) {
          arr->elements[i] = ArgPlus<IntType>::FromJson(obj["children"][i]);
        }
      } else if (obj.contains("structName")) {
        res = ArgPlus(
            obj["structName"].get<std::string>(), obj["fieldNames"].get<std::vector<std::string>>()
        );
        auto *str = static_cast<StructType<IntType> *>(res.type.get());
        for (size_t i = 0; i < obj["children"].size(); ++i) {
          str->fields[i] = ArgPlus<IntType>::FromJson(obj["children"][i]);
        }
      }
      return res;
    }
    if (obj.contains("elems")) {
      std::vector<IntType> old_elems = obj["elems"].get<std::vector<IntType>>();
      if (obj.contains("shape") && !obj["shape"].get<std::vector<int>>().empty()) {
        std::vector<int> shape = obj["shape"].get<std::vector<int>>();
        auto arg = ArgPlus(ArrayDim, shape[0]);
        for (size_t i = 0; i < old_elems.size(); ++i)
          arg.SetValue(i, old_elems[i]);
        return arg;
      }
      if (obj.contains("structName")) {
        auto arg = ArgPlus(
            obj["structName"].get<std::string>(), obj["fieldNames"].get<std::vector<std::string>>()
        );
        for (size_t i = 0; i < old_elems.size(); ++i)
          arg.SetValue(i, old_elems[i]);
        return arg;
      }
      return ArgPlus(old_elems[0]);
    }
    return ArgPlus();
  }

  // New format
  std::string kind = obj["kind"].get<std::string>();
  if (kind == "scalar") {
    return ArgPlus(obj["value"].get<IntType>());
  } else if (kind == "array") {
    // Support both old format (with "shape" array) and new format (with "dim" int)
    ArgPlus res;
    if (obj.contains("dim")) {
      // New format: single dimension at this level
      res = ArgPlus(ArrayDim, obj["dim"].get<int>());
    } else if (obj.contains("shape")) {
      // Old format: reconstruct nested structure from flat shape array
      // For now, treat shape[0] as this level's dimension for backward compatibility
      std::vector<int> shape = obj["shape"].get<std::vector<int>>();
      Assert(!shape.empty(), "Empty shape array in JSON");
      res = ArgPlus(ArrayDim, shape[0]);
    } else {
      Panic("Array type missing both 'dim' and 'shape' fields");
    }

    auto *arr = static_cast<ArrayType<IntType> *>(res.type.get());
    for (size_t i = 0; i < obj["elements"].size(); ++i) {
      arr->elements[i] = ArgPlus<IntType>::FromJson(obj["elements"][i]);
    }
    return res;
  } else if (kind == "struct") {
    auto res = ArgPlus(
        obj["structName"].get<std::string>(), obj["fieldNames"].get<std::vector<std::string>>()
    );
    auto *str = static_cast<StructType<IntType> *>(res.type.get());
    for (size_t i = 0; i < obj["fields"].size(); ++i) {
      str->fields[i] = ArgPlus<IntType>::FromJson(obj["fields"][i]);
    }
    return res;
  }
  return ArgPlus();
}

#endif
