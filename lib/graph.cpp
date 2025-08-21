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

#include <fstream>
#include <iostream>
#include <map>
#include <queue>

#include "global.hpp"
#include "lib/logger.hpp"
#include "lib/random.hpp"

const SimpleGraphSet::SimpleGraph SimpleGraphSet::NO_AVAILABLE_GRAPHS{};

const SimpleGraphSet::SimpleGraph &SimpleGraphSet::SampleGraph(int atLeastNumNodes) {
  if (data.empty()) {
    std::ifstream ifs(file);
    std::string line;
    while (std::getline(ifs, line)) {
      if (line.empty()) {
        continue;
      }
      try {
        data.push_back(nlohmann::json::parse(line));
      } catch (const nlohmann::json::exception &e) {
        Panic(
            "Failed to parse JSON line\n"
            "Line: %s\n"
            "File: %s\n"
            "Error: %s",
            line.c_str(), file.c_str(), e.what()
        );
      }
      const auto &item = data.back();
      Assert(
          item.is_object(), "The item at line %lu is not an object: %s", data.size(), line.c_str()
      );
      Assert(
          item.contains("num_bbls"), "The item at line %lu is missing the 'num_bbls' field: %s",
          data.size(), line.c_str()
      );
      Assert(
          item.contains("adj_tab"), "The item at line %lu is missing the 'adj_tab' field: %s",
          data.size(), line.c_str()
      );
    }
    ifs.close();
  }
  auto rand = Random::Get().Uniform(0, static_cast<int>(data.size()) - 1);
  for (auto tries = 0; tries < 100; tries++) {
    auto idx = rand();
    if (idx >= static_cast<int>(graphs.size())) {
      for (size_t i = graphs.size(); i <= static_cast<size_t>(idx); i++) {
        graphs.push_back(parseGraph(data[i]));
      }
    }
    auto &g = graphs[idx];
    if (g.size() >= static_cast<size_t>(atLeastNumNodes)) {
      return g;
    }
  }
  // All attempts are failed, return an empty graph to notify an error
  return NO_AVAILABLE_GRAPHS;
}

SimpleGraphSet::SimpleGraph SimpleGraphSet::parseGraph(const nlohmann::json &object) {
  SimpleGraph g{};
  const int numNodes = object["num_bbls"].get<int>();
  for (int i = 0; i < numNodes; i++) {
    g.emplace_back();
  }
  for (auto it = object["adj_tab"].begin(); it != object["adj_tab"].end(); ++it) {
    Assert(
        it->is_array(), "Adjacency list at index %ld is not an array",
        (it - object["adj_tab"].begin())
    );
    Assert(
        it->size() == 3, "Adjacency list at index %ld does not have 3 elements",
        (it - object["adj_tab"].begin())
    );
    const int sId = (*it)[0].get<int>();
    Assert(sId < numNodes, "Source node id %d is out of bounds [0, %d)", sId, numNodes);
    if (const int t1Id = (*it)[1].get<int>(); t1Id != -1) {
      Assert(
          t1Id < numNodes, "Target node at index %d id %d is out of bounds [-1, %d)", t1Id, 0,
          numNodes
      );
      g[sId].push_back(t1Id);
    }
    if (const int t2Id = (*it)[2].get<int>(); t2Id != -1) {
      Assert(
          t2Id < numNodes, "Target node at index %d id %d is out of bounds [-1, %d)", t2Id, 1,
          numNodes
      );
      g[sId].push_back(t2Id);
    }
  }
  return g;
}

bool GraphPlus::IsReachable(const int u, const int v) const {
  std::vector visited(NumNodes(), false);
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

auto GraphPlus::SampleWalk(const int start, const int end, int maxSteps) const -> std::vector<int> {
  auto rand = Random::Get().Uniform();

  std::vector<int> walk;
  // Ensure maxSteps is at least 10 times the number of nodes
  maxSteps = std::max(maxSteps, 50 * NumNodes());
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

auto GraphPlus::SampleConsistentWalk(const int start, const int end, int maxSteps) const
    -> std::vector<int> {
  auto rand = Random::Get().Uniform();

  std::vector<int> walk;
  maxSteps = std::max(maxSteps, 50 * NumNodes()); // Ensure enough steps
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

      // If the node had max outdegree and now has visited all, lock to the last one
      if (neighbors.size() == maxOutdeg && visitedNeighbors[curNode].size() == maxOutdeg) {
        lockedNeighbors[curNode] = nextNode; // Lock to the first visited neighbor
      }

      curNode = nextNode;
    }

    walk.push_back(curNode);

    steps++;
  }

  return walk;
}

std::vector<int> GraphPlus::SampleGraph(const bool consistent, const int maxSteps) const {
  return consistent ? SampleConsistentWalk(0, NumNodes() - 1, maxSteps)
                    : SampleWalk(0, NumNodes() - 1, maxSteps);
}

void GraphPlus::Generate(const bool acyclic) {
  bool hasBaseGraph = false;

  if (graphSet != nullptr &&
      Random::Get().UniformReal()() < GlobalOptions::Get().SampleBaseGraphProba) {
    // Sample a base graph from the graph database if available
    SimpleGraphSet::SimpleGraph base = graphSet->SampleGraph(NumNodes());
    if (base != SimpleGraphSet::NO_AVAILABLE_GRAPHS) {
      Log::Get().Out() << "Sampled a base graph from the graph database: #bbls=" << base.size()
                       << std::endl;
      Assert(
          base.size() >= static_cast<size_t>(NumNodes()),
          "The sampled base graph from the graph database has less than %d nodes, but got %lu",
          NumNodes(), base.size()
      );
      adjTab.clear();
      // FIXME: There're chances that the base graph is overly large. We need better strategies to
      // shrink the graph while maintaining its "good" properties. Also, the shrinked graph (e.g., a
      // subgraph) should be self-contained, not containing nodes out of it.
      for (int node = 0; node < static_cast<int>(base.size()); node++) {
        adjTab.emplace_back();
        for (int target: base[node]) {
          addEdge(node, target);
        }
      }
      // The exit node should not have any successor
      Assert(
          adjTab.back().empty(),
          "The exit node has %lu successors after sampling a base graph from the graph database",
          adjTab.back().size()
      );
      hasBaseGraph = true;
    }
  }

  if (!hasBaseGraph) {
    auto randTarget = Random::Get().Uniform(0, NumNodes() - 1);
    auto randTarNum = Random::Get().Uniform(2, static_cast<int>(maxOutdeg));

    for (int node = 0; node < NumNodes() - 1; node++) {
      if (randTarget() % 2 == 0 || node == NumNodes() - 2) {
        int target = randTarget();
        while (target == 0 || target == node) {
          target = randTarget();
        }
        addEdge(node, target);
      } else {
        std::set<int> targets;
        const int numTargets = randTarNum();

        for (int i = 0; i < numTargets; i++) {
          int target = randTarget();
          while (target == 0 || target == node || targets.contains(target)) {
            target = randTarget();
          }
          addEdge(node, target);
          targets.insert(target);
        }
      }
    }
  }

  if (acyclic) {
    ensureNoCycles();
    Assert(!HasCycles(), "The generated graph still has cycles even though we removed some");
  }

  tryMakeReachFromEntry();
  tryMakeReachToExit();

  if (!IsReachable(0, NumNodes() - 1)) {
    std::cout << "Warning: The generated graph is not reachable from the entry to the exit"
              << std::endl;
  }
}

bool GraphPlus::HasCycles() const {
  // This is a simple cycle detection algorithm using DFS
  std::vector<bool> visited(NumNodes(), false);
  std::vector<bool> recStack(NumNodes(), false);

  const std::function<bool(int)> hasCycles = [&](int n) {
    if (!visited[n]) {
      visited[n] = true;
      recStack[n] = true;
      for (int nb: adjTab[n]) {
        if (!visited[nb] && hasCycles(nb)) {
          return true; // Cycle detected
        } else if (recStack[nb]) {
          return true; // Cycle detected
        }
      }
    }
    recStack[n] = false; // Backtrack
    return false;
  };

  for (int node = 0; node < NumNodes(); node++) {
    if (!visited[node] && hasCycles(node)) {
      return true;
    }
  }

  return false;
}

void GraphPlus::Reset() {
  for (auto &adj: adjTab) {
    adj.clear();
  }
}

void GraphPlus::AddNewEntry() {
  // Update all the successors of all the old nodes
  for (int node = 0; node < NumNodes(); ++node) {
    // Update the successors of node by adding each successor's value by 1
    auto &oldSuccessors = adjTab[node];
    std::vector<int> newSuccessors(oldSuccessors.size());
    std::ranges::transform(oldSuccessors, newSuccessors.begin(), [](int v) { return v + 1; });
    oldSuccessors.clear();
    oldSuccessors.insert(newSuccessors.begin(), newSuccessors.end());
  }
  // Add an entry node to the graph
  adjTab.insert(adjTab.begin(), std::set<int>());
  // Point the entry node to the old entry node
  adjTab.front().insert(1);
}

void GraphPlus::AddNewExit() {
  // Point the old exit node to the new exit node
  adjTab.back().insert(NumNodes());
  // Add an exit node to the graph
  adjTab.emplace_back();
}

bool GraphPlus::addEdge(const int u, const int v) {
  // Ensure outdegree <= maxOutdeg
  if (u != v && adjTab[u].size() < maxOutdeg) {
    adjTab[u].insert(v);
    return true;
  }
  return false;
}

void GraphPlus::ensureNoCycles() {
  // This is a simple cycle detection algorithm using DFS
  std::vector<bool> visited(NumNodes(), false);
  std::vector<bool> recStack(NumNodes(), false);

  const std::function<void(int)> removeCycles = [&](int n) {
    if (!visited[n]) {
      visited[n] = true;
      recStack[n] = true;
      std::vector<int> cycleEdges;
      for (int neighbor: adjTab[n]) {
        if (!visited[neighbor]) {
          removeCycles(neighbor);
        } else if (recStack[neighbor]) {
          cycleEdges.push_back(neighbor); // Cycle detected
        }
      }
      if (!cycleEdges.empty()) {
        for (auto e: cycleEdges) {
          adjTab[n].erase(e); // Remove the back edge to break the cycle
        }
      }
    }
    recStack[n] = false; // Backtrack
  };

  for (int node = 0; node < NumNodes(); node++) {
    if (!visited[node]) {
      removeCycles(node);
    }
  }
}

void GraphPlus::tryMakeReachFromEntry() {
  const int entryNode = 0;
  for (int node = entryNode + 1; node < NumNodes(); node++) {
    if (!IsReachable(entryNode, node)) {
      std::vector<int> candidates;
      for (int i = entryNode; i < node; i++) {
        if (IsReachable(entryNode, i)) {
          candidates.push_back(i);
        }
      }
      for (auto candidate: candidates) {
        if (candidate == entryNode) {
          continue;
        }
        if (adjTab[candidate].size() < maxOutdeg) {
          addEdge(candidate, node); // TODO Don't we stop if we succeed?
        }
      }
    }
  }
}

void GraphPlus::tryMakeReachToExit() {
  const int exitNode = NumNodes() - 1;
  for (int node = exitNode - 1; node >= 0; node--) {
    if (!IsReachable(node, exitNode)) {
      std::vector<int> candidates;
      for (int i = node + 1; i < NumNodes(); i++) {
        if (IsReachable(i, exitNode)) {
          candidates.push_back(i);
        }
      }
      bool found = false;
      for (auto candidate: candidates) {
        if (candidate == exitNode) {
          continue;
        }
        if (adjTab[candidate].size() < maxOutdeg) {
          addEdge(node, candidate);
          found = true; // TODO Don't we stop if we succeed?
        }
      }
      if (!found) {
        addEdge(node, exitNode);
      }
    }
  }
}

void GraphPlus::unsafeEnforceReachFromEntry() {
  const int entryNode = 0;
  for (int node = 1; node < NumNodes(); node++) {
    if (!IsReachable(entryNode, node)) {
      std::vector<int> candidates;
      for (int i = 1; i < node; i++) {
        if (IsReachable(entryNode, i)) {
          candidates.push_back(i);
        }
      }
      bool found = false;
      for (auto candidate: candidates) {
        if (addEdge(candidate, node)) {
          found = true;
          break;
        }
      }
      if (!found) {
        // We force the connection between the entry and the node
        // This may break the max outdegree constraints on the entry
        adjTab[entryNode].insert(node);
      }
    }
  }
}

void GraphPlus::unsafeEnforceReachToExit() {
  const int exitNode = NumNodes() - 1;
  for (int node = NumNodes() - 2; node >= 0; node--) {
    if (IsReachable(node, exitNode)) {
      continue;
    }
    std::vector<int> candidates;
    for (int i = node + 1; i < exitNode; i++) {
      if (IsReachable(i, exitNode)) {
        candidates.push_back(i);
      }
    }
    bool found = false;
    for (auto candidate: candidates) {
      if (addEdge(node, candidate)) {
        found = true;
        break;
      }
    }
    if (!found && !addEdge(node, exitNode)) {
      // It must be the case the i's outdegree reaches the limit.
      // We remove anyone successor and directly branch to the exit.
      Assert(
          adjTab[node].size() == maxOutdeg,
          "The outdegree of node %d is not reaching the limit "
          "but we're unable to add any edges",
          node
      );
      adjTab[node].erase(adjTab[node].begin());
      Assert(
          addEdge(node, exitNode),
          "We removed an edge from node %d's successor but we're unable to branch it to the exit",
          node
      );
    }
  }
}
