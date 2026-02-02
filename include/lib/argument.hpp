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
#include <sstream>
#include <string>
#include <vector>
#include "json.hpp"
#include "lib/dbgutils.hpp"

template<typename IntType>
class ArgPlus {
public:
  explicit ArgPlus() : isScalar(true), val(0) {}

  explicit ArgPlus(IntType v) : isScalar(true), val(v) {}

  explicit ArgPlus(std::vector<int> sh) : isScalar(false), shape(std::move(sh)) {
    int n = 1;
    for (int s: shape)
      n *= s;
    children.resize(n);
  }

  explicit ArgPlus(std::string sName, std::vector<std::string> fNames) :
      isScalar(false), structName(std::move(sName)), fieldNames(std::move(fNames)) {
    children.resize(fieldNames.size());
  }

  [[nodiscard]] bool IsScalar() const { return isScalar && shape.empty() && structName.empty(); }

  [[nodiscard]] bool IsVector() const { return !shape.empty(); }

  [[nodiscard]] bool IsStruct() const { return !structName.empty(); }

  [[nodiscard]] IntType GetValue() const {
    Assert(isScalar, "Cannot get a scalar value for a vector or struct");
    return val;
  }

  [[nodiscard]] IntType GetValue(int at) const {
    int current = 0;
    return GetFlatRecursive(at, current);
  }

  IntType GetFlatRecursive(int target, int &current) const {
    if (isScalar) {
      if (current == target)
        return val;
      current++;
      return 0;
    }
    for (const auto &c: children) {
      int num = c.GetVecNumEls();
      if (current + num > target)
        return c.GetFlatRecursive(target, current);
      current += num;
    }
    return 0;
  }

  [[nodiscard]] IntType GetValue(const std::vector<int> &indices) const {
    Assert(IsVector(), "Cannot get a multi-dim value for a non-vector variable");
    int at = 0;
    for (int i = 0; i < (int) shape.size(); i++) {
      at = at * shape[i] + indices[i];
    }
    const ArgPlus *curr = &children[at];
    while (!curr->isScalar && !curr->children.empty())
      curr = &curr->children[0];
    return curr->val;
  }

  [[nodiscard]] int GetVecNumEls(int dim = -1) const {
    if (dim == -1) {
      if (isScalar)
        return 1;
      int n = 0;
      for (const auto &c: children)
        n += c.GetVecNumEls();
      return n;
    }
    Assert(IsVector(), "Cannot get number of elements for a non-vector");
    const int numDims = (int) shape.size();
    Assert(dim >= 0 && dim < numDims, "Dimension out of bound");
    int num = 1;
    for (int i = dim + 1; i < numDims; i++)
      num *= shape[i];
    if (!children.empty())
      num *= children[0].GetVecNumEls();
    return num;
  }

  [[nodiscard]] int GetVecNumDims() const { return (int) shape.size(); }

  [[nodiscard]] int GetVecDimLen(int d) const { return shape[d]; }

  [[nodiscard]] std::vector<int> GetVecShape() const { return shape; }

  [[nodiscard]] const std::string &GetStructName() const { return structName; }

  [[nodiscard]] const std::vector<std::string> &GetFieldNames() const { return fieldNames; }

  void SetValue(IntType v) {
    if (isScalar)
      val = v;
    else
      Panic("Cannot set a scalar value for a vector or struct");
  }

  void SetValue(int at, IntType v) {
    int current = 0;
    if (!SetFlatRecursive(at, v, current))
      Panic("Failed to set value at index %d", at);
  }

  bool SetFlatRecursive(int target, IntType v, int &current) {
    if (isScalar) {
      if (current == target) {
        val = v;
        return true;
      }
      current++;
      return false;
    }
    for (auto &c: children) {
      int num = c.GetVecNumEls();
      if (current + num > target)
        return c.SetFlatRecursive(target, v, current);
      current += num;
    }
    return false;
  }

  void SetStructName(std::string name) { structName = std::move(name); }

  [[nodiscard]] size_t NumChildren() const { return children.size(); }

  ArgPlus &operator[](int i) { return children[i]; }

  const ArgPlus &operator[](int i) const { return children[i]; }

  [[nodiscard]] std::string ToCxStr() const {
    if (IsScalar())
      return std::to_string(val);
    std::ostringstream oss;
    if (IsStruct()) {
      oss << "(struct " << structName << "){";
      for (size_t i = 0; i < fieldNames.size(); ++i) {
        oss << "." << fieldNames[i] << " = " << children[i].ToCxStr();
        if (i != fieldNames.size() - 1)
          oss << ", ";
      }
      oss << "}";
    } else {
      oss << "{";
      for (size_t i = 0; i < children.size(); i++) {
        oss << children[i].ToCxStr();
        if (i != children.size() - 1)
          oss << ", ";
      }
      oss << "}";
    }
    return oss.str();
  }

  [[nodiscard]] nlohmann::json ToJson() const {
    auto obj = nlohmann::json::object();
    obj["isScalar"] = isScalar;
    if (isScalar)
      obj["val"] = val;
    else {
      auto ch = nlohmann::json::array();
      for (const auto &c: children)
        ch.push_back(c.ToJson());
      obj["children"] = ch;
      if (IsVector())
        obj["shape"] = shape;
      if (IsStruct()) {
        obj["structName"] = structName;
        obj["fieldNames"] = fieldNames;
      }
    }
    return obj;
  }

  static ArgPlus FromJson(const nlohmann::json &obj) {
    if (obj.contains("isScalar") && obj["isScalar"].get<bool>()) {
      return ArgPlus(obj["val"].get<IntType>());
    }
    if (obj.contains("children")) {
      ArgPlus res;
      res.isScalar = false;
      if (obj.contains("shape"))
        res.shape = obj["shape"].get<std::vector<int>>();
      if (obj.contains("structName"))
        res.structName = obj["structName"].get<std::string>();
      if (obj.contains("fieldNames"))
        res.fieldNames = obj["fieldNames"].get<std::vector<std::string>>();
      for (const auto &c: obj["children"]) {
        res.children.push_back(ArgPlus<IntType>::FromJson(c));
      }
      return res;
    }
    if (obj.contains("elems")) {
      std::vector<IntType> old_elems = obj["elems"].get<std::vector<IntType>>();
      if (obj.contains("shape") && !obj["shape"].get<std::vector<int>>().empty()) {
        auto arg = ArgPlus(obj["shape"].get<std::vector<int>>());
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

private:
  bool isScalar;
  IntType val;
  std::vector<ArgPlus> children{};
  std::vector<int> shape{};
  std::string structName = "";
  std::vector<std::string> fieldNames{};
};

#endif
