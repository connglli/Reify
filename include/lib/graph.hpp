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

class Graph {

public:
  explicit Graph(int n) : numNodes(n), adjTab(n) {}

  int NumNodes() const { return numNodes; }

  const std::set<int> &GetAdj(int i) const { return adjTab[i]; }

  // Generate the random graph
  void Generate();

  // Check if a node has a path to a given target
  bool HasPath(int start, int target) const;

  std::vector<int> SampleWalk(int start, int end, int max_steps = 100);

  std::vector<int> SampleConsistentWalk(int start, int end, int max_steps = 100);

  void PrintGraph();

private:
  // Add an edge between node u and node v
  void addEdge(int u, int v);

  // Ensure every node has a path to the last node while keeping outdegree <= 2
  void enforceReachabilityToLast();

  // Ensure every node is reachable from 0 while keeping outdegree <= 2
  void enforceReachabilityFromStart();

  std::vector<std::vector<int>> sampleKDisjointWalksAndBackpatchGraph(int start, int end, int k);

  std::vector<std::vector<int>> getKDistinctWalks(int start, int end, int k);

  std::vector<std::vector<int>> getKDistinctConsistentWalks(int start, int end, int k);

private:
  const int numNodes;
  std::vector<std::set<int>> adjTab; // Use set to avoid duplicate edges
};

#endif // GRAPHFUZZ_GRAPH_HPP
