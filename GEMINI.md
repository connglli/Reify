# Reify: Gemini Guideline

+ Knowledge Background: Optimizing Compilers, Program Generation, SMT theories
+ Implementation Language: C++ 20 (source: ./src)
+ Scripting Language: Python 3.12 (source: ./scripts, virtualenv: ./scripts/venv)

## Project Overview

This project implements a random program generator called Reify.

Unlike generators such as Csmith and YARPGen that generate programs based on syntactic rules only, Reify generate programs centering on semantic rules. It distinguishes two distinct semantics:
(1) compile-time semantics (what a program can do), represented by the control flow graph (CFG), and
(2) runtime semantics (what a program actually does), represented by execution paths (EP) within the CFG.

For a CFG $g$ and an EP $\pi$ on it, Reify constructs a program $P$ guaranteed to be well-behaved with respect to a specific input $i$ and output $o$. This means that $P$'s CFG is $g$ and when executing with $i$, $P$ deterministically follows the $\pi$ to produce the expected output $o$.

Reify separates the generation process into two stages:
1. Leaf Function Generation: Generate a leaf function and its input/output satisfying a CFG and an EP on it.
2. Whole Program Generation: Compose leaf functions into a whole program via peepwhole rewrite.

Reify implements these with help of a symbolic intermediate language called SymIR. SymIR incorporates symbols to represent unknown values. Reify leverage symbolic execution to reason about program behaviors with symbols. Solving the collected constraints via SMT solvers to find concrete values for symbols to satisfy desired behaviors.

Technical details of the two stages and the IR are presented in [./DOCS.md](./DOCS.md).

## Project Structure

```
.
├── Makefile
├── include/
│   ├── cxxopts.hpp
│   ├── global.hpp               # Global variables and options
│   ├── json.hpp
│   ├── jnif/
│   ├── zip/
│   └── lib/                     # Reify's headers (.hpp)
├── lib/
│   ├── lang.cpp                 # SymIR's implementation
│   ├── lowers.cpp               # SymIR's lowers: SymIR to S expressions, C, Java bytecode, etc.
│   ├── parsers.cpp              # SymIR's parser: parsing S expressions into SymIR
│   ├── graph.cpp                # Modeling and generating a graph
│   ├── ctrlflow.cpp             # Modeling and generating a CFG and its EPs
│   ├── function.cpp             # Modeling and generating a leaf function
│   ├── program.cpp              # Modeling and generating a whole progra
│   ├── ubfexec.cpp              # Solving a SymIR program following an EP
│   ├── ubfree.cpp               # Collecting UB-free constraints
│   ├── ubinject.cpp             # Injecting UB into unreachable basic blocks
│   ├── random.cpp
│   ├── cchksum.c
│   ├── chksum.cpp
│   ├── logger.cpp
│   └── third/                   # Third-party dependencies
├── src/
│   ├── func_gen.cpp             # CLI entry to generate a leaf function
│   └── prog_gen.cpp             # CLI entry to generate a set of whole programs
├── scripts/
│   ├── fgen.py                  # Script to generate a set of leaf functions
│   ├── fuzz.py                  # Script to fuzz GCC or LLVM
│   ├── fuzz_jvm.sh              # Script to fuzzing HotSpot or OpenJ9
│   └── ...
└── ...
```

## Testing - TDD Approach

ALWAYS create five failing tests first before implementing any feature or fixing any bug.

1. Write five tests that exposes the bug or demonstrates the desired behavior
2. Run the tests one by one separately to confirm it fails
3. Implement the fix/feature
4. Run the tests one by one separately to confirm it passes
5. Add the smallest test that covers the case, in the right place

Never:

1. Disable failing tests
2. Change test cases to avoid hitting bugs
3. Use workarounds to allow failing tests to pass without fixing the issue.
4. Implement features without a test demonstrating them first

## Dependency Management

1. For C++, the dependencies are managed manually. Whenever a new dependency is introduced, update README.md to indicate users how to install it.
2. For Python, the dependencies are managed by virtualenv and by `requirements.txt` and `requirements.dev.txt`.
   + Virtual env: our virtual environment is always at `./venv` and is activated by `source venv/bin/activate`
   + `requirements.txt`: these are dependencies used to run our scripts
   + `requirements.dev.txt`: these are dependencies used during development such as pre-commit hooks
   + Whenever a new dependency is introduced, ALWAYS add it into the corresponding dependency management file with exact versions.

## Best Practices

1. Use git to track your activities reasonably.
2. Follow Conventional Commit to organize your commits.
3. Keep README.md, TODO.md, and related docs updated.
4. Fix all compiler warnings.
5. Organize the repository in a good strucuture.
6. Write high-quality comments.

### Before starting to work:
1. Recall what has been done previously, if it's not in your context or memory. You may use `git log` or similar commands to preview the past activities. When necessary, use `git show` or similar commands to see the detail of a concerned activity.
2. If the task is difficult and you expect to work it for a long time, remember to save your changes periodically and reasonably with concise commit message title and high-quality commit message body.

### Before saving changes:
1. ALWAYS clear all warnings prompted by the compiler
2. ALWAYS format all your code with clang-format
3. ALWAYS ensure all tests pass (except for timeouts, for now)
4. ALWAYS check what has changed with `git status`
5. ALWAYS split changes into small changes that can be tracked better
6. ALWAYS include a type, a title (less than 50 characters), and a body in commit messages according to Conventional Commit like below:
   ```
   <type>[optional scope]: <title>

   <body>

   [optional footer(s)]
   ```
   The title should be concise while the body should summary the changes in more detail.
