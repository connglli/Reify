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

#include "lib/ctrlflow.hpp"

#include <queue>

#include "lib/logger.hpp"
#include "lib/random.hpp"

std::vector<int> BiLoop::SampleOneIter(int stepLimit, bool consistent, bool inclExit) const {
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
  // Sync all bimpos and their start index in basicblocks array
  // Also clear and update the flattened basicblocks array
  bimpos.clear();
  indexInBbls.clear();
  basicblocks.clear();
  for (auto bi = 0; bi < numBimpos(); bi++) {
    // Currently, each bimpo is a indeed basic block
    bimpos.push_back(std::make_unique<BiBlock>());
    // So each bimpo occupy only 1 basic block
    indexInBbls.push_back(bi);
    // Sync the basicblocks array
    basicblocks.emplace_back(BblSketch::Type::BLOCK_ORDINARY);
    basicblocks.back().successors.insert(
        // We fix the successors order of the each basic block from now on
        basicblocks.back().successors.begin(), graph.GetAdj(bi).begin(), graph.GetAdj(bi).end()
    );
  }
}

void CfgSketch::GenerateReduLoop(const int numBbls) {
  // Randomly select a block and transform it into a loop, except for the entry and exit
  const auto rand = Random::Get().Uniform(1, numBimpos() - 2);
  int biInd;
  do {
    biInd = rand();
    if (bimpos[biInd]->GetType() == Bimpo::Type::BLOCK) {
      break;
    }
  } while (true);
  // TODO: Currently, we did not support nested loops as otherwise
  // we'll often get into cycles when sampling executions.
  bimpos[biInd] = std::make_unique<BiLoop>(numBbls, /*allowNestedLoops=*/false);

  // Update the flattened basicblocks array.
  // Previously, the basicblocks array is:
  //   [flatBiInd-1          : a               ],
  //   [flatBiInd            : bimpo's block   ],
  //   [flatBiInd+1          : b               ]
  // Currently, it should change to:
  //   [flatBiInd-1          : a               ],
  //   [flatBiInd            : loopGraph.entry ],
  //   ...
  //   [flatBiInd+loopSize-2 : loopGraph.latch ],
  //   [flatBiInd+loopSize-1 : loopGraph.exit  ],
  //   [flatBiInd+loopSize   : b               ]

  // Specifically, we should flatten all basic blocks into basicblocks
  // starting at the index of the old bimpo in the basicblocks array
  const auto flatBiInd = indexInBbls[biInd];
  // The insertion point in the basicblocks array is flatBiInd
  const auto flatBiIt = basicblocks.begin() + flatBiInd;

  // Now we are going to construct the basicblocks array of the newly inserted
  // loop. The newly inserted loop's basic blocks will be inserted starting from
  // flatBiInd. So it is obvious that the basicbloks-array index of all
  // bimpos after biInd will march by the number of blocks in the loop
  const auto *loop = dynamic_cast<const BiLoop *>(bimpos[biInd].get());
  const int loopSize = loop->NumBbls();
  for (int i = biInd + 1; i < numBimpos(); i++) {
    indexInBbls[i] = indexInBbls[i] + loopSize - 1;
  }

  // Since blkSuccessorsInLoop will be inserted into the basicblocks array,
  // all blocks' successors that are > flatBiInd should march loopSize-1:
  for (auto &bbl: basicblocks) {
    for (auto &succ: bbl.successors) {
      if (succ > flatBiInd) {
        succ = succ + loopSize - 1;
      }
    }
  }

  // Map each block in the loop into a BblSketch.
  // Since blkSuccessorsInLoop will be inserted into the basicblocks array,
  // all blocks in the loop's original adjacent table should march by flatBiInd
  std::vector<BblSketch> loopBbls;
  for (auto i = 0; i < loopSize; i++) {
    if (i == loop->GetEntry()) {
      loopBbls.emplace_back(BblSketch::Type::BLOCK_LOOP_HEAD);
    } else if (i == loop->GetLatch()) {
      loopBbls.emplace_back(BblSketch::Type::BLOCK_LOOP_LATCH);
    } else if (i == loop->GetExit()) {
      loopBbls.emplace_back(BblSketch::Type::BLOCK_LOOP_EXIT);
    } else {
      loopBbls.emplace_back(BblSketch::Type::BLOCK_ORDINARY);
    }
    // We fix the successors order of the each basic block from now on
    for (const auto &succ: loop->GetSuccessor(i)) {
      loopBbls.back().successors.push_back(succ + flatBiInd);
    }
  }
  // The loop latch should branch to the header of the loop
  Assert(
      loopBbls[loop->GetLatch()].successors.size() == 1,
      "The latch of the loop should have only 1 successor, i.e., its exit"
  );
  loopBbls[loop->GetLatch()].successors.insert(
      // We always ensure that the first successor of the latch is the entry
      loopBbls[loop->GetLatch()].successors.begin(), flatBiInd
  );
  Assert(
      !loop->GetSuccessor(loop->GetLatch()).contains(loop->GetEntry()),
      "The original loop graph's latch should remain untouched"
  );
  // The loop exiting node should branch to all the old successors
  Assert(
      loopBbls[loop->GetExit()].successors.empty(),
      "The exit of the loop should not have any successor"
  );
  loopBbls[loop->GetExit()].successors.insert(
      loopBbls[loop->GetExit()].successors.begin(), flatBiIt->successors.begin(),
      flatBiIt->successors.end()
  );
  Assert(
      loop->GetSuccessor(loop->GetExit()).empty(),
      "The original loop graph's exit should remain untouched"
  );

  // Insert loopBbls into the basicblocks array
  basicblocks.erase(flatBiIt);
  basicblocks.insert(flatBiIt, loopBbls.begin(), loopBbls.end());
}

std::vector<int>
CfgSketch::SampleExec(const int stepLimit, const bool consistent, const int maxLoopIter) {
  const double decayGamma = 0.5; // TODO: Move to global options
  const auto continueLoop = Random::Get().UniformReal();
  // We first sample a coarse-grained execution which dose not execute any loops
  std::vector<int> coarseExec = graph.SampleGraph(consistent, stepLimit);
  std::vector<int> fineExec;
  for (int biInd: coarseExec) {
    int restSteps = stepLimit - static_cast<int>(fineExec.size());
    if (restSteps <= 0) {
      break;
    }
    const auto &bi = bimpos[biInd];
    if (bi->GetType() == Bimpo::Type::BLOCK) {
      // This is a real basic block, we directly append it
      fineExec.push_back(indexInBbls[biInd]);
    } else if (bi->GetType() == Bimpo::Type::LOOP) {
      // This is a loop, we'd sample an execution in the loop, then we'd
      // decide whether we re-execute the loop by one more iteration.
      const BiLoop *loop = dynamic_cast<const BiLoop *>(bi.get());
      double prob = continueLoop(), threshold = 1.0;
      bool executedAtLeastOnce = false;
      for (int it = 0; it < maxLoopIter && restSteps > 0 && prob < threshold; it++) {
        // TODO: Do we keep the execution of each loop iteration when consistent=true?
        // Append the loop execution to the fine-grained execution
        // Update the id of each block in the loop to match that in successors
        for (const auto &blk: loop->SampleOneIter(restSteps, consistent, /*inclExit=*/false)) {
          fineExec.push_back(blk + indexInBbls[biInd]);
        }
        threshold = decayGamma * threshold;
        prob = continueLoop();
        restSteps = stepLimit - static_cast<int>(fineExec.size());
        executedAtLeastOnce = true;
      }
      if (executedAtLeastOnce) {
        // Append the exit of the loop to the fine execution
        fineExec.push_back(loop->GetExit() + indexInBbls[biInd]);
      }
    } else {
      Assert(false, "Unknown bimpo type %d", bi->GetType());
    }
  }
  return fineExec;
}

void CfgSketch::Print() const {
  Log::Get().OpenSection("CfgSketch: Unflattened CFG Sketch");
  for (int i = 0; i < numBimpos(); i++) {
    Log::Get().Out() << "Bimpo: type=" << bimpos[i]->GetType() << ", id=" << i
                     << ", size=" << bimpos[i]->NumBbls() << ", start=" << indexInBbls[i]
                     << std::endl;
  }
  Log::Get().CloseSection();

  Log::Get().OpenSection("CfgSketch: Flattened CFG Sketch");
  for (int u = 0; u < basicblocks.size(); u++) {
    Log::Get().Out() << "Block: " << u << " -> [";
    int j = 0;
    for (const auto v: basicblocks[u].successors) {
      Log::Get().Out() << v;
      if (j != basicblocks[u].successors.size() - 1) {
        Log::Get().Out() << ", ";
      }
      j++;
    }
    Log::Get().Out() << "]" << std::endl;
  }
  Log::Get().CloseSection();
}
