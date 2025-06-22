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

#ifndef GRAPHFUZZ_GRAPHPLUS_HPP
#define GRAPHFUZZ_GRAPHPLUS_HPP

#include <algorithm>
#include <memory>
#include <set>
#include <vector>
#include "lib/dbgutils.hpp"

class CfgSketch;

/// A GraphPlus is random graph generator
class GraphPlus {
  friend class CfgSketch;

public:
  explicit GraphPlus(int numNodes, int maxOutdeg = 2) : adjTab(numNodes), maxOutdeg(maxOutdeg) {
    Assert(numNodes >= 3, "The minimum nodes must be greater than 3, but got %d", numNodes);
    Assert(maxOutdeg >= 2, "The maximum outdegree must be greater than 2, but got %d", maxOutdeg);
  }

  [[nodiscard]] int NumNodes() const { return static_cast<int>(adjTab.size()); }

  [[nodiscard]] const std::set<int> &GetAdj(int i) const { return adjTab[i]; }

  [[nodiscard]] const std::vector<std::set<int>> &GetAdjTab() const { return adjTab; }

  // Check if u->v is reachable, i.e., there exists a path from node u to node v.
  [[nodiscard]] bool IsReachable(int u, int v) const;

  // Check if the entry node can reach the exit node
  [[nodiscard]] bool IsEntryExitReachable() const { return IsReachable(0, NumNodes() - 1); }

  // Check if the graph has cycles.
  [[nodiscard]] bool HasCycles() const;

  // Walk the graph randomly from start and stop if we encounter end or reach max steps.
  // Return the path we've been through (including start and the end).
  [[nodiscard]] std::vector<int> SampleWalk(int start, int end, int maxSteps = 100) const;

  // Walk the graph randomly from start and stop if we encounter end or reach max steps.
  // Return the path we've been through (including start and the end).
  // Different from inconsistent walk, this walk consistently visit a single successor after
  // successors of a node have all been visited.
  [[nodiscard]] std::vector<int> SampleConsistentWalk(int start, int end, int maxSteps = 100) const;

  // Sample the whole graph from the entry to the exit, or reach the max number of steps.
  [[nodiscard]] std::vector<int> SampleGraph(bool consistent, int maxSteps = 100) const;

  // Add a new entry node to the graph, which points to the old entry node
  void AddNewEntry() {
    // Update all the successors of all the old nodes
    for (int node = 0; node < NumNodes(); ++node) {
      // Update the successors of node by adding each successor's value by 1
      auto &oldSuccessors = adjTab[node];
      std::vector<int> newSuccessors(oldSuccessors.size());
      std::transform(oldSuccessors.begin(), oldSuccessors.end(), newSuccessors.begin(), [](int v) {
        return v + 1;
      });
      oldSuccessors.clear();
      oldSuccessors.insert(newSuccessors.begin(), newSuccessors.end());
    }
    // Add an entry node to the graph
    adjTab.insert(adjTab.begin(), std::set<int>());
    // Point the entry node to the old entry node
    adjTab.front().insert(1);
  }

  // Add a new exit node to the graph, which is pointed by the old exit node
  void AddNewExit() {
    // Point the old exit node to the new exit node
    adjTab.back().insert(NumNodes());
    // Add an exit node to the graph
    adjTab.emplace_back();
  }

  // Generate the backbone graph randomly.
  void Generate(bool acyclic = false);

  // Reset the graph
  void Reset();

private:
  // Add an edge between node u and node v, return true if added
  bool addEdge(int u, int v);

  // Ensure the graph has no cycles
  void ensureNoCycles();

  // Try to make every node is reachable from the entry node
  // However, this is not ensured
  void tryMakeReachFromEntry();

  // Try to make every node has a path to the exit node
  // However, this is not ensured
  void tryMakeReachToExit();

  // Enforce the reachability from every node to the exit.
  // This is an unsafe operation as they may break our set
  // constraints over the maximum in/out degrees.
  void unsafeEnforceReachToExit();

  // Enforce the reachability from the entry to every node.
  // This is an unsafe operation as they may break our set
  // constraints over the maximum in/out degrees.
  void unsafeEnforceReachFromEntry();

private:
  const int maxOutdeg;
  std::vector<std::set<int>> adjTab{};
};

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
  [[nodiscard]] virtual int NumBbls() const = 0;
  [[nodiscard]] virtual Type GetType() const = 0;
};

/// A Block is a real basic block
class Block final : public Bimpo {
public:
  [[nodiscard]] int NumBbls() const override { return 1; }

  [[nodiscard]] Type GetType() const override { return BLOCK; }
};

/// A Loop is a reducible loop with entry, latch, and exit
class Loop final : public Bimpo {
public:
  explicit Loop(int numBbls, bool allowNestedLoops = true) : graph(numBbls - 1, 2) {
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

  int GetEntry() const { return 0; }

  int GetLatch() const { return graph.NumNodes() - 2; }

  int GetExit() const { return graph.NumNodes() - 1; }

  const std::set<int> &GetSuccessor(int block) const { return graph.GetAdj(block); }

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
    BLOCK_ORDINARY = 0, // Oridinary block
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
  explicit CfgSketch(int initNumBbls) : graph(initNumBbls, 2) {}

  CfgSketch(const CfgSketch &) = delete;
  CfgSketch &operator=(const CfgSketch &) = delete;

  // Get the number of basic blocks
  [[nodiscard]] int NumBbls() const { return static_cast<int>(basicblocks.size()); }

  [[nodiscard]] const BblSketch &GetBbl(int id) const {
    Assert(
        id >= 0 && id < NumBbls(), "Invalid basic block id %d, must be in [0, %d)", id, NumBbls()
    );
    return basicblocks[id];
  }

  // Sample an execution and stop once we executed the exit block
  // or we have reached the limit of our execution steps
  [[nodiscard]] std::vector<int> SampleExec(int stepLimit, bool consistent, int maxLoopIter = 5);

  // Generate the sketch of the control flow graph randomly.
  void Generate();

  // Generate a reducible loop and insert into the graph.
  // This method must be called after Generate().
  // This method will turn a random block into a
  void GenerateReduLoop(int numBbls = 5);

  void Print();

private:
  int numBimpos() const { return graph.NumNodes(); }

private:
  // The unflattened view of the CFG
  GraphPlus graph; // Each node in the graph corresponds to an bimpo
  std::vector<std::unique_ptr<Bimpo>> bimpos{};
  std::vector<int> indexInBbls{}; // The index of each bimpo in the basicblocks list

  // The flattened view of the CFG, each correspond to a real basic block
  std::vector<BblSketch> basicblocks{};
};

#endif // GRAPHFUZZ_GRAPHPLUS_HPP
