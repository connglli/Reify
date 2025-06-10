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

#include "lib/graph.hpp"

#include <iostream>
#include <map>
#include <queue>

#include "lib/random.hpp"

bool CfgBkboneGen::HasPath(int u, int v) const {
  std::vector<bool> visited(numNodes, false);
  std::queue<int> worklist;

  worklist.push(u);
  visited[u] = true;

  while (!worklist.empty()) {
    int curr = worklist.front();
    worklist.pop();
    if (curr == v)
      return true;
    for (int neighbor: adjTab[curr]) {
      if (!visited[neighbor]) {
        visited[neighbor] = true;
        worklist.push(neighbor);
      }
    }
  }

  return false;
}

std::vector<int> CfgBkboneGen::SampleWalk(int start, int end, int maxSteps) const {
  auto rand = Random::Get().Uniform();

  std::vector<int> walk;
  // Ensure maxSteps is at least 10 times the number of nodes
  maxSteps = std::max(maxSteps, 50 * numNodes);
  int curNode = start;
  walk.push_back(curNode);

  int steps = 0;
  while (curNode != end && steps < maxSteps) {
    const std::set<int> &neighbors = adjTab[curNode];
    if (neighbors.empty()) {
      break; // No more neighbors to move to
    }

    // Randomly choose a neighbor to walk to
    auto it = neighbors.begin();
    std::advance(it, rand() % neighbors.size()); // Advance random steps

    curNode = *it;
    walk.push_back(curNode);

    steps++;
  }

  // If we did not reach the end, the walk is incomplete
  return walk;
}

std::vector<int> CfgBkboneGen::SampleConsistentWalk(int start, int end, int maxSteps) const {
  auto rand = Random::Get().Uniform();

  std::vector<int> walk;
  maxSteps = std::max(maxSteps, 50 * numNodes); // Ensure enough steps
  int curNode = start;
  walk.push_back(curNode);

  std::map<int, std::set<int>> visitedNeighbors; // Tracks visited neighbors for each node
  std::map<int, int> lockedNeighbors; // Locks a node to one of its neighbors once both are visited

  int steps = 0;
  while (curNode != end && steps < maxSteps) {
    const std::set<int> &neighbors = adjTab[curNode];
    if (neighbors.empty()) {
      break; // No more neighbors to move to
    }

    // Check if this node already has a locked neighbor
    if (lockedNeighbors.find(curNode) != lockedNeighbors.end()) {
      // If locked, always go to the locked neighbor
      curNode = lockedNeighbors[curNode];
    } else {
      // Otherwise, randomly choose a neighbor
      auto it = neighbors.begin();
      std::advance(it, rand() % neighbors.size()); // Advance random steps

      int nextNode = *it;

      // Record the visit
      visitedNeighbors[curNode].insert(nextNode);

      // If the node had outdegree 2 and now has visited both, lock to the last one
      if (neighbors.size() == 2 && visitedNeighbors[curNode].size() == 2) {
        lockedNeighbors[curNode] = nextNode; // Lock to the first visited neighbor
      }

      curNode = nextNode;
    }

    walk.push_back(curNode);

    steps++;
  }

  return walk;
}

std::vector<std::vector<int>> CfgBkboneGen::GetKDistinctWalks(int start, int end, int k) const {
  std::set<std::vector<int>> walks;
  int tries = 0;
  while (walks.size() < k && tries < 1000) {
    std::vector<int> walk = SampleWalk(start, end);
    if (walk.size() > 1) {
      walks.insert(walk);
    }
    tries++;
  }
  return std::vector<std::vector<int>>(walks.begin(), walks.end());
}

std::vector<std::vector<int>>
CfgBkboneGen::GetKDistinctConsistentWalks(int start, int end, int k) const {
  std::set<std::vector<int>> walks;
  int tries = 0;
  while (walks.size() < k && tries < 1000) {
    std::vector<int> walk = SampleConsistentWalk(start, end);
    if (walk.size() > 1) {
      walks.insert(walk);
    }
    tries++;
  }
  return std::vector<std::vector<int>>(walks.begin(), walks.end());
}

std::vector<std::vector<int>>
CfgBkboneGen::SampleKDisjointWalksAndBackpatch(int start, int end, int k) {
  auto rand = Random::Get().Uniform();

  // k paths must only have the start and end node in common
  // for others, we make random connections between the nodes in order to make them disjoint
  std::vector<std::vector<int>> walks;
  int tries = 0;
  std::set<int> nonVisitedNodes;
  for (int i = 0; i < numNodes; i++) {
    nonVisitedNodes.insert(i);
  }
  while (walks.size() < k && tries < 1000) {
    std::vector<int> walk;
    // let's sample a walk from the unvisited nodes
    int curNode = start;
    int numNodesInWalk = 0;
    walk.push_back(curNode);
    // look for a neighbour in the unvisited nodes, if not, then we create a new edge after
    // arbitrarily choosing a node from the unvisited nodes
    while (curNode != end || numNodesInWalk >= 50 * numNodes) {
      const std::set<int> &neighbors = adjTab[curNode];
      // check if any of the current node's neighbours are in the unvisited nodes
      std::set<int> unvisitedNeighbors;
      for (auto neighbor: neighbors) {
        if (nonVisitedNodes.find(neighbor) != nonVisitedNodes.end()) {
          unvisitedNeighbors.insert(neighbor);
        }
      }
      if (!unvisitedNeighbors.empty()) {
        auto it = unvisitedNeighbors.begin();
        std::advance(it, rand() % unvisitedNeighbors.size()); // Random step
        curNode = *it;
      } else {
        // choose a random node from the unvisited nodes
        auto it = nonVisitedNodes.begin();
        std::advance(it, rand() % nonVisitedNodes.size()); // Random step
        adjTab[curNode].insert(*it);
        curNode = *it;
      }
      walk.push_back(curNode);
      numNodesInWalk++;
    }
    // remove all the nodes in the walk from the nonVisitedNodes set
    for (auto node: nonVisitedNodes) {
      if (node != start && node != end) {
        nonVisitedNodes.erase(node);
      }
    }
    // add the walk to the set of walks
    if (walk.size() > 1) {
      walks.push_back(walk);
    }
    tries++;
  }
  return walks;
}

void CfgBkboneGen::Generate() {
  auto rand = Random::Get().Uniform(0, numNodes - 1);

  for (int i = 0; i < numNodes - 1; i++) {
    if (rand() % 2 == 0 || i == numNodes - 2) {
      int target = rand();
      while (target == 0 || target == i)
        target = rand();
      addEdge(i, target);
    } else {
      int target1 = rand();
      int target2 = rand();

      while (target1 == 0 || target1 == i)
        target1 = rand();
      while (target2 == 0 || target2 == i || target2 == target1)
        target2 = rand();

      addEdge(i, target1);
      addEdge(i, target2);
    }
  }

  enforceReachabilityFromEntry();
  enforceReachabilityToExit();
}

void CfgBkboneGen::addEdge(int u, int v) {
  // Ensure outdegree <= 2 as we are a CFG
  if (u != v && adjTab[u].size() < 2) {
    adjTab[u].insert(v);
  }
}

void CfgBkboneGen::enforceReachabilityToExit() {
  for (int i = numNodes - 2; i >= 0; i--) {
    if (!HasPath(i, numNodes - 1)) {
      std::vector<int> candidates;
      for (int j = i + 1; j < numNodes; j++) {
        if (HasPath(j, numNodes - 1)) {
          candidates.push_back(j);
        }
      }
      bool found = false;
      for (auto candidate: candidates) {
        if (candidate == numNodes - 1) {
          continue;
        }
        if (adjTab[candidate].size() < 2) {
          addEdge(i, candidate);
          found = true;
        }
      }
      if (!found) {
        addEdge(i, numNodes - 1);
      }
    }
  }
}

void CfgBkboneGen::enforceReachabilityFromEntry() {
  for (int i = 1; i < numNodes; i++) {
    if (!HasPath(0, i)) {
      std::vector<int> candidates;
      for (int j = 0; j < i; j++) {
        if (HasPath(0, j)) {
          candidates.push_back(j);
        }
      }
      for (auto candidate: candidates) {
        if (candidate == 0) {
          continue;
        }
        if (adjTab[candidate].size() < 2) {
          addEdge(candidate, i);
        }
      }
    }
  }
}
