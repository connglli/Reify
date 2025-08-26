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

#include <sstream>
#include <vector>

#include "json.hpp"
#include "lib/dbgutils.hpp"

template<typename IntType>
class ArgPlus {
public:
  explicit ArgPlus() : shape({}) { elems.resize(1); }

  explicit ArgPlus(std::vector<int> shape) : shape(std::move(shape)) {
    for (const int &len: this->shape) {
      Assert(
          len > 0, "Each vector dimension must have at least one element, however %d is given", len
      );
    }
    int numEls = 1;
    for (int i = 0; i < GetVecNumDims(); i++) {
      numEls *= this->shape[i];
    }
    elems.resize(numEls);
  }

  [[nodiscard]] bool IsScalar() const { return shape.size() == 0; }

  [[nodiscard]] bool IsVector() const { return shape.size() != 0; }

  [[nodiscard]] IntType GetValue() const {
    Assert(IsScalar(), "Cannot get a scalar value for a vector");
    Assert(elems.size() == 1, "The scalar variable should have exactly one element");
    return elems[0];
  }

  [[nodiscard]] IntType GetValue(int at) const {
    Assert(IsVector(), "Cannot get a value for a scalar variable");
    Assert(at < GetVecNumEls(), "The index (%d) is out of bound (%d)", at, GetVecNumEls());
    return elems[at];
  }

  [[nodiscard]] IntType GetValue(const std::vector<int> &indices) const {
    Assert(IsVector(), "Cannot get a value for a scalar variable");
    Assert(
        indices.size() == shape.size(),
        "The indices (size=%lu) does not align with the dimensions (%lu)", indices.size(),
        shape.size()
    );
    int at = 0;
    for (int i = 0; i < GetVecNumDims(); i++) {
      Assert(
          indices[i] < shape[i], "The index (%d) is out of bound (%d) at dim %d", indices[i],
          shape[i], i
      );
      Assert(indices[i] >= 0, "The index (%d) must be non-negative at dim %d", indices[i], i);
      at = at * shape[i] + indices[i];
    }
    return elems[at];
  }

  [[nodiscard]] const std::vector<IntType> &GetEls() const { return elems; }

  [[nodiscard]] std::vector<int> GetVecShape() const { return shape; }

  [[nodiscard]] int GetVecDimLen(int d) const {
    Assert(IsVector(), "Cannot get the length of a vector dimension lengths for a scalar");
    Assert(d < GetVecNumDims(), "The dimension (%d) is out of bound (%d)", d, GetVecNumDims());
    return shape[d];
  }

  [[nodiscard]] int GetVecNumDims() const {
    Assert(IsVector(), "Cannot get number of dimensions for a scalar");
    return static_cast<int>(shape.size());
  }

  [[nodiscard]] int GetVecNumEls(int dim = -1) const {
    Assert(IsVector(), "Cannot get number of elements for a scalar");
    if (dim == -1) {
      return static_cast<int>(elems.size());
    }
    const int numDims = static_cast<int>(shape.size());
    Assert(
        dim >= 0 && dim < numDims, "The accessing dimension (%d) is out of bound (%d)", dim, numDims
    );
    int num = 1;
    for (int i = dim + 1; i < numDims; i++) {
      num *= shape[i];
    }
    return num;
  }

  void SetValue(IntType val) {
    Assert(IsScalar(), "Cannot set a scalar value for a vector");
    elems = {val};
  }

  void SetValue(int at, IntType val) {
    Assert(IsVector(), "Cannot set a value for a scalar variable");
    Assert(at < GetVecNumEls(), "The index (%d) is out of bound (%d)", at, GetVecNumEls());
    elems[at] = {val};
  }

  [[nodiscard]] std::string ToCxStr() const {
    std::ostringstream oss;
    if (IsScalar()) {
      oss << GetValue();
    } else {
      const auto numEls = GetVecNumEls();
      oss << "(int(*)";
      for (int d = 1; d < GetVecNumDims(); d++) {
        oss << "[" << GetVecDimLen(d) << "]";
      }
      oss << ")((int[" << numEls << "]){";
      for (int k = 0; k < numEls; k++) {
        oss << GetValue(k);
        if (k < numEls - 1) {
          oss << ", ";
        }
      }
      oss << "})";
    }
    return oss.str();
  }

  [[nodiscard]] nlohmann::json ToJson() const {
    auto obj = nlohmann::json::object();
    obj["shape"] = shape;
    obj["elems"] = elems;
    return obj;
  }

  static ArgPlus<IntType> FromJson(nlohmann::json obj) {
    Assert(obj.is_object(), "The argument JSON is not an object");
    Assert(obj.contains("shape"), "The argument JSON does not contain the field 'shape'");
    Assert(obj.contains("elems"), "The argument JSON does not contain the field 'elems'");
    Assert(obj["shape"].is_array(), "The 'shape' field is not an array");
    Assert(obj["elems"].is_array(), "The 'elems' field is not an array");

    std::vector<int> shape = obj["shape"].get<std::vector<int>>();
    std::vector<IntType> elems = obj["elems"].get<std::vector<IntType>>();
    if (shape.empty()) {
      auto arg = ArgPlus<IntType>();
      arg.SetValue(elems[0]);
      return arg;
    } else {
      auto arg = ArgPlus<IntType>(shape);
      Assert(
          static_cast<int>(elems.size()) == arg.GetVecNumEls(),
          "The number of elements (%lu) does not match the expected size (%d)", elems.size(),
          arg.GetVecNumEls()
      );
      for (int i = 0; i < static_cast<int>(elems.size()); i++) {
        arg.SetValue(i, elems[i]);
      }
      return arg;
    }
  }

private:
  std::vector<int> shape;
  std::vector<IntType> elems{};
};


#endif // REIFY_ARGUMENT_HPP
