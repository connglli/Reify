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

#ifndef REIFY_TRANSFORMATIONS_HPP
#define REIFY_TRANSFORMATIONS_HPP

#include "lib/Transformations/primitive.hpp"
#include "lib/Transformations/instcombine.hpp"
#include "lib/Transformations/vectorize.hpp"
#include "lib/lang.hpp"

using namespace transformations;
/// ==================== Rewrite Engine Definition ====================
class RewriteEngine {
public:
  /// Get empty RewriteEngine without any rules added
  static RewriteEngine Empty() { return RewriteEngine(); }
  /// Get default RewriteEngine with the default set of rules added
  static RewriteEngine Default() {
    auto engine = RewriteEngine();
                // Rule                                                  // Weight
    engine.addRule(std::make_unique<primitive::Guard>(),                        50);
    engine.addRule(std::make_unique<primitive::Reg2Mem>(),                       2);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaAdd>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaSub>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaMul>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaDiv>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaNot>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaAnd>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaXor>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaOr>(),         1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaShl>(),        1);
    engine.addRule(std::make_unique<primitive::ConstPropagationViaShr>(),        1);
    engine.addRule(std::make_unique<primitive::AdditionFromConst>(),           100);
    engine.addRule(std::make_unique<primitive::AggressiveAdditionFromConst>(),  50);
    engine.addRule(std::make_unique<primitive::InsertConstZeroAdditions>(),     10);
    engine.addRule(std::make_unique<primitive::ForSumFromConst>(),              20);
    engine.addRule(std::make_unique<primitive::DeadCodeFromAssign>(),            1);
    engine.addRule(std::make_unique<vectorize::DeadAssignFromCopy>(),            2);
    engine.addRule(std::make_unique<vectorize::Reduction>(),                    20);
    engine.addRule(std::make_unique<vectorize::Induction>(),                    20);
    engine.addRule(std::make_unique<vectorize::WithAliasCheck>(),                5);
    engine.addRule(std::make_unique<instcombine::FoldAddLikeCommutative>(),    100);
    engine.addRule(std::make_unique<instcombine::ShlToAddTwice>(),             100);
    engine.addRule(std::make_unique<instcombine::OrToAddAndXor>(),             100);
    engine.addRule(std::make_unique<instcombine::AddToAddOrAnd>(),             100);
    engine.addRule(std::make_unique<instcombine::AndToSubOrXor>(),             100);
    engine.addRule(std::make_unique<instcombine::XorToSubOrAnd>(),             100);
    engine.addRule(std::make_unique<instcombine::AndToSubAndAnd>(),            100);
    return engine;
  }
  void addRule(std::unique_ptr<Rule> rule, int weight);
  /// normal run method used for random selection of rules based on the passed weight, attempts to runs `times` rules in total
  void run(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    size_t times
  ) const;

  /// Runs a Rules as a Pass e.g. run it over all stmts in all blocks
  void runAsPass(
    symir::FunctBuilder *funBd,
    std::vector<symir::BlockBuilder *> &blockBds,
    VariableState &varState,
    Rule &rule
  ) const;


private:
  RewriteEngine() = default;

  std::optional<Rule *> getRandomMatchingRule(const symir::Stmt *stmt) const;

protected:
  std::vector<std::unique_ptr<Rule>> rules{};
  std::vector<int> weights{};
};

#endif //REIFY_TRANSFORMATIONS_HPP
