# Reify: Agent Guideline

+ Knowledge Background: Optimizing Compilers, Program Generation, SMT theories
+ Implementation Language: C++ 20 (source: ./src)
+ Scripting Language: Python 3.12 (source: ./scripts, virtualenv: ./venv)

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

## Design Principles

The SMT solver takes most of Reify's running time. So every design choice should first ask: **does this make the solver's job easier?**

But Reify is a random program generator. If we only do what makes the solver happy, we generate boring programs that don't stress compilers. So the real rule is:

> **Spend the solver's effort on the shape of the program (CFG and path), not on the math inside each statement.** Make each statement cheap to solve. Get interesting programs by combining cheap statements along complex paths, not by writing one fancy statement.

### SymIR: pick operations the solver can handle fast

- **Keep expressions in a form the solver finds cheap.** Linear shapes (sums of small terms) solve fastest. Don't push toward richer expression forms just to look more interesting — richness should come from longer paths and bigger CFGs.
- **Ration non-linearity between program variables.** The existing `coef * var` term form (where `coef` is itself a symbol the solver picks) already puts a product of two unknowns into every query — that's the baseline, not a violation. The rule is about *adding more*: operations like `var * var`, `var / var`, `var % var`, and symbolic shifts should be used sparingly when introduced. Prefer one symbolic side and a small constant on the other. Don't put them in every expression.
- **Keep symbolic shift amounts in a small set of values.** The bigger the set, the more the solver has to try.
- **Keep symbolic indexing as simple as possible.** Prefer concrete indices, or pin a symbolic index to a single value. Avoid general array-theory reasoning unless we really need it.
- **Don't widen bit-widths without a reason.** A wider type makes every operation cost more inside the solver. Use the smallest width that still triggers the bug class we care about.
- **Every partial op needs a guard.** Operations like division, remainder, and shift have inputs that cause UB. They must ship with constraints that rule those inputs out.
- **A new op has to pay rent.** Before adding one, ask: can we already get the same bugs by combining existing ops along a more interesting CFG or path? If yes, do not add the op.

### Symbolic Execution: keep constraints simple

Some constraint shapes make the solver work much harder. Use them as little as possible:

- **ITE (if-then-else):** each ITE forces the solver to try both arms.
- **Large disjunctions / OR chains:** each branch is another case to try.
- **Quantifiers** (rules of the form "for all X" or "there exists X"): often push the query into undecidable territory.
- **Cardinality constraints** (e.g., "at most K of these are zero"): expensive even when K is small.
- **Fresh symbols:** every new symbol adds a dimension to the search space.
- **Mixing theories** (bit-vectors + arithmetic + arrays in one query): the solver pays for theory combination.

Concrete reminders:

- **Gate values with the path condition, not by merging branches.** Single-path symbolic execution naturally avoids most ITE. Don't drift toward branch merging "for elegance".
- **Simplify ITE while building it, not later.** Patterns like `ite(true, a, b) → a`, `ite(c, true, b) → c or b`, `ite(c, a, false) → c and a` should be done by helpers in the encoder, so the solver never sees them.
- **Pull shared parts out of ITE arms.** `ite(c, f+a, f+b)` should be written as `f + ite(c, a, b)`. Smaller arms, more sharing.
- **If you must use ITE, put the easy-to-decide condition on the outside.** The solver can prune the outer branch fast and avoid digging into the inner one.
- **Prefer enumeration over quantifiers.** If a property has to hold "for all X" over a small domain, enumerate X and assert each case instead.
- **Treat cardinality constraints as a performance decision from day one.** Narrow the counter width, tighten the bound, and reuse across attempts.
- **Don't introduce fresh symbols you don't need.** If a value can be written in terms of existing symbols, reuse them. Every fresh symbol is another variable the solver has to assign.
- **Stay in one theory per constraint when possible.** If you must mix, do it at the boundary, not inside every expression.
- **Reuse identical constraints, don't re-assert them.** When adding a new constraint kind, make sure equivalent forms get the same identity so the encoder can drop duplicates.
- **Don't build an ITE just to give a "don't care" value a name.** Leave the symbol unconstrained; the path condition decides whether anyone reads it.

### One rule that covers both parts

- **Measure before you believe.** Always check perf changes with `scripts/rysmith.py --limit N --disable-shuffle`. Past intuition has been wrong here — don't trust a change until the numbers say so.

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
│   ├── program.cpp              # Modeling and generating a whole program
│   ├── symexec.cpp              # Symbolically executing a SymIR program following an EP
│   ├── ubfree.cpp               # Collecting UB-free constraints
│   ├── ubinject.cpp             # Injecting UB into unreachable basic blocks
│   ├── ubbase.cpp
│   ├── random.cpp
│   ├── cchksum.c
│   ├── chksum.cpp
│   ├── logger.cpp
│   └── third/                   # Third-party dependencies
├── src/
│   ├── symircc.cpp              # CLI entry to compile a SymIR program into a C program
│   ├── rysmith.cpp              # CLI entry to generate a leaf function
│   └── rylink.cpp               # CLI entry to generate a set of whole programs
├── scripts/
│   ├── rysmith.py               # Script to generate a set of leaf functions
│   ├── rylink.py                # Script to generate a set of whole functions
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
