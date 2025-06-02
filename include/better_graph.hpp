// MIT License
//
// Copyright (c) 2025-2025
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

#pragma once

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

class Graph {
public:
  int nodes;
  std::vector<std::set<int>> adj; // Use set to avoid duplicate edges

  Graph(int n) : nodes(n), adj(n) {}

  void add_edge(int u, int v) {
    if (u != v && adj[u].size() < 2) { // Ensure outdegree <= 2
      adj[u].insert(v);
    }
  }

  // Function to check if a node has a path to a given target
  bool has_path(int start, int target) {
    std::vector<bool> visited(nodes, false);
    std::queue<int> q;
    q.push(start);
    visited[start] = true;

    while (!q.empty()) {
      int curr = q.front();
      q.pop();
      if (curr == target)
        return true;
      for (int neighbor: adj[curr]) {
        if (!visited[neighbor]) {
          visited[neighbor] = true;
          q.push(neighbor);
        }
      }
    }
    return false;
  }

  // Ensure every node has a path to the last node while keeping outdegree <= 2
  void enforce_reachability_to_last() {
    for (int i = nodes - 2; i >= 0; i--) {
      if (!has_path(i, nodes - 1)) {
        std::vector<int> candidates;
        for (int j = i + 1; j < nodes; j++) {
          if (has_path(j, nodes - 1)) {
            candidates.push_back(j);
          }
        }
        bool found = false;
        for (auto candidate: candidates) {
          if (candidate == nodes - 1) {
            continue;
          }
          if (adj[candidate].size() < 2) {
            add_edge(i, candidate);
            found = true;
          }
        }
        if (!found) {
          add_edge(i, nodes - 1);
        }
      }
    }
  }

  // Ensure every node is reachable from 0 while keeping outdegree <= 2
  void enforce_reachability_from_start() {
    for (int i = 1; i < nodes; i++) {
      if (!has_path(0, i)) {
        std::vector<int> candidates;
        for (int j = 0; j < i; j++) {
          if (has_path(0, j)) {
            candidates.push_back(j);
          }
        }
        for (auto candidate: candidates) {
          if (candidate == 0) {
            continue;
          }
          if (adj[candidate].size() < 2) {
            add_edge(candidate, i);
          }
        }
        // if (!candidates.empty() && adj[candidates[0]].size() < 2) {
        //     int target = candidates[rand() % candidates.size()];
        //     add_edge(target, i);
        // }
      }
    }
  }

  void generate_graph() {
    srand(time(0));

    for (int i = 0; i < nodes - 1; i++) {
      if (rand() % 2 == 0 || i == nodes - 2) {
        int target = rand() % nodes;
        while (target == 0 || target == i)
          target = rand() % nodes;
        add_edge(i, target);
      } else {
        int target1 = rand() % nodes;
        int target2 = rand() % nodes;

        while (target1 == 0 || target1 == i)
          target1 = rand() % nodes;
        while (target2 == 0 || target2 == i || target2 == target1)
          target2 = rand() % nodes;

        add_edge(i, target1);
        add_edge(i, target2);
      }
    }

    // Ensure all nodes have a path to the last node while obeying outdegree <= 2


    // Ensure all nodes are reachable from 0 while obeying outdegree <= 2
    enforce_reachability_from_start();
    enforce_reachability_to_last();
  }

  std::vector<int> sample_walk(int start, int end, int max_steps = 100) {
    std::vector<int> walk;
    max_steps = std::max(max_steps, 50 * nodes); // Ensure max_steps is at least 10 times the number of nodes
    int current_node = start;
    walk.push_back(current_node);

    int steps = 0;
    while (current_node != end && steps < max_steps) {
      const std::set<int> &neighbors = adj[current_node];
      if (neighbors.empty()) {
        break; // No more neighbors to move to
      }

      // Randomly choose a neighbor to walk to
      auto it = neighbors.begin();
      std::advance(it, rand() % neighbors.size()); // Random step
      current_node = *it;

      walk.push_back(current_node);
      steps++;
    }

    // If we did not reach the end, the walk is incomplete
    return walk;
  }

  std::vector<int> sample_consistent_walk(int start, int end, int max_steps = 100) {
    std::vector<int> walk;
    max_steps = std::max(max_steps, 50 * nodes); // Ensure enough steps
    int current_node = start;
    walk.push_back(current_node);

    std::map<int, std::set<int>> visited_neighbors; // Tracks visited neighbors for each node
    std::map<int, int> locked_neighbor; // Locks a node to one of its neighbors once both are visited

    int steps = 0;
    while (current_node != end && steps < max_steps) {
      const std::set<int> &neighbors = adj[current_node];
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
        std::advance(it, rand() % neighbors.size()); // Random step
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

  std::vector<std::vector<int>> sample_k_disjoint_walks_and_backpatch_graph(int start, int end, int k) {
    // k paths must only have the start and end node in common
    // for others, we make random connections between the nodes in order to make them disjoint
    std::vector<std::vector<int>> walks;
    int tries = 0;
    std::set<int> non_visited_nodes;
    for (int i = 0; i < nodes; i++) {
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
      while (current_node != end || num_nodes_in_walk >= 50 * nodes) {
        const std::set<int> &neighbors = adj[current_node];
        // check if any of the current node's neighbours are in the unvisited nodes
        std::set<int> unvisited_neighbors;
        for (auto neighbor: neighbors) {
          if (non_visited_nodes.find(neighbor) != non_visited_nodes.end()) {
            unvisited_neighbors.insert(neighbor);
          }
        }
        if (unvisited_neighbors.size() > 0) {
          auto it = unvisited_neighbors.begin();
          std::advance(it, rand() % unvisited_neighbors.size()); // Random step
          current_node = *it;
        } else {
          // choose a random node from the unvisited nodes
          auto it = non_visited_nodes.begin();
          std::advance(it, rand() % non_visited_nodes.size()); // Random step
          adj[current_node].insert(*it);
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

  std::vector<std::vector<int>> get_k_distinct_walks(int start, int end, int k) {
    std::set<std::vector<int>> walks;
    int tries = 0;
    while (walks.size() < k && tries < 1000) {
      std::vector<int> walk = sample_walk(start, end);
      if (walk.size() > 1) {
        walks.insert(walk);
      }
      tries++;
    }
    std::vector<std::vector<int>> result(walks.begin(), walks.end());
    return result;
  }

  std::vector<std::vector<int>> get_k_distinct_consistent_walks(int start, int end, int k) {
    std::set<std::vector<int>> walks;
    int tries = 0;
    while (walks.size() < k && tries < 1000) {
      std::vector<int> walk = sample_consistent_walk(start, end);
      if (walk.size() > 1) {
        walks.insert(walk);
      }
      tries++;
    }
    std::vector<std::vector<int>> result(walks.begin(), walks.end());
    return result;
  }

  void print_graph() {
    for (int i = 0; i < nodes; i++) {
      std::cout << "Node " << i << " -> ";
      for (int j: adj[i]) {
        std::cout << j << " ";
      }
      std::cout << std::endl;
    }
  }
};
