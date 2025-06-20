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

#include "lib/graphplus.hpp"

#include <iostream>
#include <map>
#include <queue>

#include "lib/random.hpp"

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
  auto randTarget = Random::Get().Uniform(0, NumNodes() - 1);
  auto randTarNum = Random::Get().Uniform(2, maxOutdeg);

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

  if (acyclic) {
    ensureNoCycles();
    Assert(!HasCycles(), "The generated graph still has cycles even though we removed some");
  }

  tryMakeReachFromEntry();
  tryMakeReachToExit();

  if (!IsReachable(0, NumNodes() - 1)) {
    std::cerr << "Warning: The generated graph is not reachable from the entry to the exit"
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

std::vector<int> Loop::SampleOneIter(int stepLimit, bool consistent, bool inclExit) const {
  for (int tries = 0; tries < 100; tries++) {
    auto exec = graph.SampleGraph(consistent, stepLimit);
    if (exec.back() == GetExit()) {
      if (!inclExit) {
        exec.pop_back();
      }
      return exec;
    }
  }
  Assert(false, "Failed to sample a valid execution from the loop after 100 tries");
  return {};
}

void CfgSketch::Generate() {
  graph.Reset();
  graph.Generate();
  // Sync all imposters and their start index in successors
  // Also clear and update the flattened successors table
  imposters.clear();
  indexInSuccessorsOf.clear();
  successors.clear();
  for (auto impo = 0; impo < numImposters(); impo++) {
    // Currently, each imposter is a indeed basic block
    imposters.push_back(std::make_unique<Block>());
    // So each imposter occupy only 1 basic block
    indexInSuccessorsOf.push_back(impo);
    const auto &adj = graph.GetAdj(impo);
    std::cout << "Updating: " << impo << " -> [";
    int j = 0;
    for (auto v: adj) {
      std::cout << v;
      if (j != adj.size() - 1) {
        std::cout << ", ";
      }
      j++;
    }
    std::cout << "]" << std::endl;
    // Sync the successors table
    successors.emplace_back(graph.GetAdj(impo).begin(), graph.GetAdj(impo).end());
  }
}

void CfgSketch::GenerateReduLoop(const int numBbls) {
  // Randomly select a block and transform it into a loop, except for the entry and exit
  const auto rand = Random::Get().Uniform(1, numImposters() - 2);
  int impoIndex;
  do {
    impoIndex = rand();
    if (imposters[impoIndex]->GetType() == BblImpo::Type::BLOCK) {
      break;
    }
  } while (true);
  // TODO: Currently, we did not support nested loops as otherwise
  // we'll often Get into cycles when sampling executions.
  imposters[impoIndex] = std::make_unique<Loop>(numBbls, /*allowNestedLoops=*/false);

  // Update the flattened successors table.
  // Previously, the successors table is:
  //   [flatImpoIndex-1          : a               ],
  //   [flatImpoIndex            : imposter's block],
  //   [flatImpoIndex+1          : b               ]
  // Currently, it should change to:
  //   [flatImpoIndex-1          : a               ],
  //   [flatImpoIndex            : loopGraph.entry ],
  //   ...
  //   [flatImpoIndex+loopSize-2 : loopGraph.latch ],
  //   [flatImpoIndex+loopSize-1 : loopGraph.exit  ],
  //   [flatImpoIndex+loopSize   : b               ]

  // Specifically, we should flatten all basic blocks into successors starting
  // at the index of the old imposter in the successors table
  const auto flatImpoIndex = indexInSuccessorsOf[impoIndex];
  // The insertion point in the successors table is flatImpoIndex
  const auto flatImpoIter = successors.begin() + flatImpoIndex;

  // Now we are going to construct the successors table of the newly inserted
  // loop. The newly inserted loop's basic blocks will be inserted starting from
  // flatImpoIndex. So it is obvious that the successors-table index of all
  // imposters after impoIndex will march by the number of blocks in the loop
  const auto *loop = dynamic_cast<const Loop *>(imposters[impoIndex].get());
  const int loopSize = loop->NumBbls();
  for (int i = impoIndex + 1; i < numImposters(); i++) {
    indexInSuccessorsOf[i] = indexInSuccessorsOf[i] + loopSize - 1;
  }
  std::cout << "...... Replacing " << impoIndex << " with loop of size " << loopSize << " ......"
            << std::endl;

  // Since blkSuccessorsInLoop will be inserted into the successors table,
  // all blocks' successors that are > flatImpoIndex should march loopSize-1:
  for (std::set<int> &blkSuccessors: successors) {
    std::set<int> toUpdate;
    for (auto &succ: blkSuccessors) {
      if (succ > flatImpoIndex) {
        toUpdate.insert(succ);
      }
    }
    for (auto &succ: toUpdate) {
      blkSuccessors.erase(succ);
      blkSuccessors.insert(succ + loopSize - 1);
    }
  }

  // Since blkSuccessorsInLoop will be inserted into the successors table,
  // all blocks in the loop's original adjacent table should march by flatImpoIndex
  auto blkSuccessorsInLoop = std::vector<std::set<int>>(loopSize);
  for (int bbl = 0; bbl < loopSize; bbl++) {
    for (const auto &succ: loop->GetSuccessor(bbl)) {
      blkSuccessorsInLoop[bbl].insert(succ + flatImpoIndex);
    }
  }
  // The loop latch should branch to the header of the loop
  Assert(
      blkSuccessorsInLoop[loop->GetLatch()].size() == 1,
      "The latch of the loop should have only 1 successor, i.e., its exit"
  );
  blkSuccessorsInLoop[loop->GetLatch()].insert(flatImpoIndex);
  Assert(
      !loop->GetSuccessor(loop->GetLatch()).contains(loop->GetEntry()),
      "The original loop graph's latch should remain untouched"
  );
  // The loop exiting node should branch to all the old successors
  Assert(
      blkSuccessorsInLoop[loop->GetExit()].size() == 0,
      "The exit of the loop should not have any successor"
  );
  blkSuccessorsInLoop[loop->GetExit()].insert(flatImpoIter->begin(), flatImpoIter->end());
  Assert(
      loop->GetSuccessor(loop->GetExit()).empty(),
      "The original loop graph's exit should remain untouched"
  );

  // Insert blkSuccessorsInLoop into the successors table
  successors.erase(flatImpoIter);
  successors.insert(flatImpoIter, blkSuccessorsInLoop.begin(), blkSuccessorsInLoop.end());
}

std::vector<int>
CfgSketch::SampleExec(const int stepLimit, const bool consistent, const int maxLoopIter) {
  const double decayGamma = 0.5;
  auto continueLoop = Random::Get().UniformReal();
  // We first sample a coarse-grained execution which dose not execute any loops
  std::vector<int> coarseExec = graph.SampleGraph(consistent, stepLimit);
  std::vector<int> fineExec;
  for (int i = 0; i < coarseExec.size(); i++) {
    int restSteps = stepLimit - static_cast<int>(fineExec.size());
    if (restSteps <= 0) {
      break;
    }
    const auto impoIndex = coarseExec[i];
    const auto &impo = imposters[impoIndex];
    if (impo->GetType() == BblImpo::Type::BLOCK) {
      // This is a real basic block, we directly append it
      fineExec.push_back(indexInSuccessorsOf[impoIndex]);
    } else if (impo->GetType() == BblImpo::Type::LOOP) {
      // This is a loop, we'd sample an execution in the loop, then we'd
      // decide whether we re-execute the loop by one more iteration.
      const Loop *loop = dynamic_cast<const Loop *>(impo.get());
      double prob = continueLoop(), threshold = 1.0;
      bool executedAtLeastOnce = false;
      for (int it = 0; it < maxLoopIter && restSteps > 0 && prob < threshold; it++) {
        // TODO: Do we keep the execution of each loop iteration when consistent=true?
        auto loopExec = loop->SampleOneIter(consistent, restSteps, /*inclExit=*/false);
        // Append the loop execution to the fine-grained execution
        // Update the id of each block in the loop to match that in successors
        for (const auto &blk: loopExec) {
          fineExec.push_back(blk + indexInSuccessorsOf[impoIndex]);
        }
        threshold = decayGamma * threshold;
        prob = continueLoop();
        restSteps = stepLimit - static_cast<int>(fineExec.size());
        executedAtLeastOnce = true;
      }
      if (executedAtLeastOnce) {
        // Append the exit of the loop to the fine execution
        fineExec.push_back(loop->GetExit() + indexInSuccessorsOf[impoIndex]);
      }
    } else {
      Assert(false, "Unknown imposter type %d", impo->GetType());
    }
  }
  return fineExec;
}

void CfgSketch::Print() {
  std::cout << "======== Unflattened CFG Sketch ========" << std::endl;
  std::cout << ": " << std::endl;
  for (int i = 0; i < numImposters(); i++) {
    std::cout << "Imposter: type=" << imposters[i]->GetType() << ", id=" << i
              << ", size=" << imposters[i]->NumBbls() << ", start=" << indexInSuccessorsOf[i]
              << std::endl;
  }
  std::cout << "======== Flattened CFG Sketch ========" << std::endl;
  for (int u = 0; u < successors.size(); u++) {
    std::cout << "Block: " << u << " -> [";
    int j = 0;
    for (auto v: successors[u]) {
      std::cout << v;
      if (j != successors[u].size() - 1) {
        std::cout << ", ";
      }
      j++;
    }
    std::cout << "]" << std::endl;
  }
}
