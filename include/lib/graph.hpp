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

#ifndef GRAPHFUZZ_GRAPH_HPP
#define GRAPHFUZZ_GRAPH_HPP

#include <algorithm>
#include <set>
#include <vector>

#include "lib/dbgutils.hpp"

/// A GraphPlus is random graph generator
class GraphPlus {
  friend class CfgSketch;

public:
  explicit GraphPlus(int numNodes, int maxOutdeg = 2) : maxOutdeg(maxOutdeg), adjTab(numNodes) {
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

#endif // GRAPHFUZZ_GRAPH_HPP
