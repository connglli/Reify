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

// Function to check if a node has a path to a given target
bool Graph::HasPath(int start, int target) const {
  std::vector<bool> visited(numNodes, false);
  std::queue<int> q;
  q.push(start);
  visited[start] = true;

  while (!q.empty()) {
    int curr = q.front();
    q.pop();
    if (curr == target)
      return true;
    for (int neighbor: adjTab[curr]) {
      if (!visited[neighbor]) {
        visited[neighbor] = true;
        q.push(neighbor);
      }
    }
  }
  return false;
}

void Graph::Generate() {
  auto rand = Random::Get().Uniform(0, numNodes-1);
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

  // Ensure all nodes have a path to the last node while obeying outdegree <= 2
  // Ensure all nodes are reachable from 0 while obeying outdegree <= 2
  enforceReachabilityFromStart();
  enforceReachabilityToLast();
}

std::vector<int> Graph::SampleWalk(int start, int end, int max_steps) const {
  auto rand = Random::Get().Uniform();

  std::vector<int> walk;
  max_steps = std::max(max_steps, 50 * numNodes); // Ensure max_steps is at least 10 times the number of nodes
  int current_node = start;
  walk.push_back(current_node);

  int steps = 0;
  while (current_node != end && steps < max_steps) {
    const std::set<int> &neighbors = adjTab[current_node];
    if (neighbors.empty()) {
      break; // No more neighbors to move to
    }

    // Randomly choose a neighbor to walk to
    auto it = neighbors.begin();
    std::advance(it, rand() % neighbors.size()); // Advance random steps

    current_node = *it;
    walk.push_back(current_node);

    steps++;
  }

  // If we did not reach the end, the walk is incomplete
  return walk;
}

std::vector<int> Graph::SampleConsistentWalk(int start, int end, int max_steps) const {
  auto rand = Random::Get().Uniform();

  std::vector<int> walk;
  max_steps = std::max(max_steps, 50 * numNodes); // Ensure enough steps
  int current_node = start;
  walk.push_back(current_node);

  std::map<int, std::set<int>> visited_neighbors; // Tracks visited neighbors for each node
  std::map<int, int> locked_neighbor;             // Locks a node to one of its neighbors once both are visited

  int steps = 0;
  while (current_node != end && steps < max_steps) {
    const std::set<int> &neighbors = adjTab[current_node];
    if (neighbors.empty()) {
      break; // No more neighbors to move to
    }

    // Check if this node already has a locked neighbor
    if (locked_neighbor.find(current_node) != locked_neighbor.end()) {
      // If locked, always go to the locked neighbor
      current_node = locked_neighbor[current_node];
    } else {
      // Otherwise, randomly choose a neighbor
      auto it = neighbors.begin();
      std::advance(it, rand() % neighbors.size()); // Advance random steps

      int next_node = *it;

      // Record the visit
      visited_neighbors[current_node].insert(next_node);

      // If the node had outdegree 2 and now has visited both, lock to the last one
      if (neighbors.size() == 2 && visited_neighbors[current_node].size() == 2) {
        locked_neighbor[current_node] = next_node; // Lock to the first visited neighbor
      }

      current_node = next_node;
    }

    walk.push_back(current_node);

    steps++;
  }

  return walk;
}

void Graph::addEdge(int u, int v) {
  if (u != v && adjTab[u].size() < 2) {
    // Ensure outdegree <= 2
    adjTab[u].insert(v);
  }
}

void Graph::enforceReachabilityToLast() {
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

void Graph::enforceReachabilityFromStart() {
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
      // if (!candidates.empty() && adj[candidates[0]].size() < 2) {
      //     int target = candidates[rand() % candidates.size()];
      //     addEdge(target, i);
      // }
    }
  }
}

std::vector<std::vector<int>> Graph::SampleKDisjointWalksAndBackpatchGraph(int start, int end, int k) {
  auto rand = Random::Get().Uniform();

  // k paths must only have the start and end node in common
  // for others, we make random connections between the nodes in order to make them disjoint
  std::vector<std::vector<int>> walks;
  int tries = 0;
  std::set<int> non_visited_nodes;
  for (int i = 0; i < numNodes; i++) {
    non_visited_nodes.insert(i);
  }
  while (walks.size() < k && tries < 1000) {
    std::vector<int> walk;
    // let's sample a walk from the unvisited nodes
    int current_node = start;
    int num_nodes_in_walk = 0;
    walk.push_back(current_node);
    // look for a neighbour in the unvisited nodes, if not, then we create a new edge after arbitrarily choosing a
    // node from the unvisited nodes
    while (current_node != end || num_nodes_in_walk >= 50 * numNodes) {
      const std::set<int> &neighbors = adjTab[current_node];
      // check if any of the current node's neighbours are in the unvisited nodes
      std::set<int> unvisited_neighbors;
      for (auto neighbor: neighbors) {
        if (non_visited_nodes.find(neighbor) != non_visited_nodes.end()) {
          unvisited_neighbors.insert(neighbor);
        }
      }
      if (!unvisited_neighbors.empty()) {
        auto it = unvisited_neighbors.begin();
        std::advance(it, rand() % unvisited_neighbors.size()); // Random step
        current_node = *it;
      } else {
        // choose a random node from the unvisited nodes
        auto it = non_visited_nodes.begin();
        std::advance(it, rand() % non_visited_nodes.size()); // Random step
        adjTab[current_node].insert(*it);
        current_node = *it;
      }
      walk.push_back(current_node);
      num_nodes_in_walk++;
    }
    // remove all the nodes in the walk from the non_visited_nodes set
    for (auto node: non_visited_nodes) {
      if (node != start && node != end) {
        non_visited_nodes.erase(node);
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

std::vector<std::vector<int>> Graph::GetKDistinctWalks(int start, int end, int k) const {
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

std::vector<std::vector<int>> Graph::GetKDistinctConsistentWalks(int start, int end, int k) const {
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
