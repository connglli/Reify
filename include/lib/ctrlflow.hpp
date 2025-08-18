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

#ifndef REIFY_GRAPHPLUS_HPP
#define REIFY_GRAPHPLUS_HPP

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "lib/dbgutils.hpp"
#include "lib/graph.hpp"

class CfgSketch;

/// A Bimpo is an imposter of a basic block.
/// This means the bimpo might not be a real basic block.
class Bimpo {
public:
  enum Type {
    BLOCK, // The bimpo represents a real basic block
    LOOP,  // The bimpo represents a loop's control flow graph
  };

public:
  virtual ~Bimpo() = default;

  // Get the number of actual basic blocks this bimpo is pretending
  [[nodiscard]] virtual int NumBbls() const = 0;

  // Get the type of the real basic block or basic blocks
  [[nodiscard]] virtual Type GetType() const = 0;
};

/// A BiBlock is a real basic block
class BiBlock final : public Bimpo {
public:
  [[nodiscard]] int NumBbls() const override { return 1; }

  [[nodiscard]] Type GetType() const override { return BLOCK; }
};

/// A BiLoop is a reducible loop with entry, latch, and exit
class BiLoop final : public Bimpo {
public:
  explicit BiLoop(int numBbls, bool allowNestedLoops = true) : graph(numBbls - 1, 2) {
    bool reachable = false;
    // We instantiate the loop's CFG directly. We'll try until we successfully
    // generate a loop that is reachable from the entry to the exit.
    for (int tries = 0; tries < 100; tries++) {
      graph.Generate(/*acyclic=*/!allowNestedLoops);
      if (!graph.IsEntryExitReachable()) {
        // If the graph is not reachable from the entry to the exit, we try again
        graph.Reset();
      } else {
        reachable = true;
        break; // We successfully generated a loop
      }
    }
    Assert(reachable, "Failed to generate a reducible loop after 100 attempts.");
    // We add a new exit node to
    // (1) treat the prior exit as the latch, and
    // (2) treat the new exit node as the loop exit
    graph.AddNewExit();
  }

  [[nodiscard]] int GetEntry() const { return 0; }

  [[nodiscard]] int GetLatch() const { return graph.NumNodes() - 2; }

  [[nodiscard]] int GetExit() const { return graph.NumNodes() - 1; }

  [[nodiscard]] const std::set<int> &GetSuccessor(int block) const { return graph.GetAdj(block); }

  [[nodiscard]] int NumBbls() const override { return graph.NumNodes(); }

  [[nodiscard]] Type GetType() const override { return LOOP; }

  // Sample an execution and stop once we executed the exit block
  // or we have reached the limit of our execution steps
  [[nodiscard]] std::vector<int>
  SampleOneIter(int stepLimit, bool consistent, bool inclExit = false) const;

private:
  GraphPlus graph;
};

/// A BblSketch is a sketch of a basic block (without generating SIRs) in the CFG.
class BblSketch {
  friend class CfgSketch;

public:
  enum Type {
    BLOCK_ORDINARY = 0, // Ordinary block
    BLOCK_LOOP_HEAD,    // Loop head block
    BLOCK_LOOP_LATCH,   // Loop latch block
    BLOCK_LOOP_EXIT,    // Loop exit block
  };

public:
  explicit BblSketch(Type type) : type(type) {}

  [[nodiscard]] Type GetType() const { return type; }

  [[nodiscard]] const std::vector<int> &GetSuccessors() const { return successors; }

private:
  Type type;                     // The type of the basic block (e.g., BLOCK, LOOP)
  std::vector<int> successors{}; // The successors of the basic block
};

/// CfgSketch is a random generator for the backbone (without generating SIRs) of a CFG
class CfgSketch {
public:
  explicit CfgSketch(const int initNumBbls) : graph(initNumBbls, 2) {}

  CfgSketch(const CfgSketch &) = delete;
  CfgSketch &operator=(const CfgSketch &) = delete;

  // Get the number of basic blocks
  [[nodiscard]] int NumBbls() const { return static_cast<int>(basicblocks.size()); }

  // Get the basic block with the given ID
  [[nodiscard]] const BblSketch &GetBbl(int id) const {
    Assert(
        id >= 0 && id < NumBbls(), "Invalid basic block id %d, must be in [0, %d)", id, NumBbls()
    );
    return basicblocks[id];
  }

  // Set a graph set for sampling the underlying graphs
  void SetGraphSet(std::string file) { graph.SetGraphSet(std::move(file)); }

  // Sample an execution and stop once we executed the exit block
  // or we have reached the limit of our execution steps
  [[nodiscard]] std::vector<int> SampleExec(int stepLimit, bool consistent, int maxLoopIter = 5);

  // Generate the sketch of the control flow graph randomly.
  void Generate();

  // Generate a reducible loop and insert into the graph.
  // This method must be called after Generate().
  // This method will turn a random block into a
  void GenerateReduLoop(int numBbls = 5);

  void Print() const;

private:
  [[nodiscard]] int numBimpos() const { return graph.NumNodes(); }

private:
  // The unflattened view of the CFG
  GraphPlus graph; // Each node in the graph corresponds to an bimpo
  std::vector<std::unique_ptr<Bimpo>> bimpos{};
  std::vector<int> indexInBbls{}; // The index of each bimpo in the basicblocks list

  // The flattened view of the CFG, each correspond to a real basic block
  std::vector<BblSketch> basicblocks{};
};

#endif // REIFY_GRAPHPLUS_HPP
