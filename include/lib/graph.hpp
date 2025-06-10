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

#include <set>
#include <vector>

// CfgBkboneGen is a random generator for the backbone (without generating SIRs) of a CFG
class CfgBkboneGen {

public:
  explicit CfgBkboneGen(int n) : numNodes(n), adjTab(n) {}

  [[nodiscard]] int NumNodes() const { return numNodes; }

  [[nodiscard]] const std::set<int> &GetAdj(int i) const { return adjTab[i]; }

  // Check if there exists a path from node u to node v.
  [[nodiscard]] bool HasPath(int u, int v) const;

  // Walk the graph randomly from start and stop if we encounter end or reach max steps.
  // Return the path we've been through (including start and the end).
  [[nodiscard]] std::vector<int> SampleWalk(int start, int end, int maxSteps = 100) const;

  [[nodiscard]] std::vector<int> SampleConsistentWalk(int start, int end, int maxSteps = 100) const;

  // Generate the backbone graph randomly.
  void Generate();

  [[nodiscard]] std::vector<std::vector<int>> GetKDistinctWalks(int start, int end, int k) const;

  [[nodiscard]] std::vector<std::vector<int>>
  GetKDistinctConsistentWalks(int start, int end, int k) const;

  [[nodiscard]] std::vector<std::vector<int>>
  SampleKDisjointWalksAndBackpatch(int start, int end, int k);

private:
  // Add an edge between node u and node v
  void addEdge(int u, int v);

  // Ensure every node is reachable from the entry node while keeping outdegree <= 2 as we are a CFG
  void enforceReachabilityFromEntry();

  // Ensure every node has a path to the exit node while keeping outdegree <= 2 as we are a CFG
  void enforceReachabilityToExit();

private:
  const int numNodes;
  std::vector<std::set<int>> adjTab;
};

#endif // GRAPHFUZZ_GRAPH_HPP
