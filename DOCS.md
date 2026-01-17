# Semantic Reification and Reify

Reify's technique is called *Semantic Reification*, a new paradigm for random program generation. Unlike syntactic reification, which operates primarily on syntax, semantic reification centers on program semantics. It distinguish between two kinds of semantics: compile-time semantics (what a program can do)
and runtime semantics (what a program actually does). The key insight of semantic reification is
reformulating random program generation to capture both forms of semantics as follows.
Given an *arbitrary* control flow graph (CFG) $g$ to capture compile-time semantics
and an *arbitrary* entry-to-exit $\pi$ within $g$ (called execution path or EP) to capture runtime semantics, it produces a program $P$, input $i$, and output $o$, satisfying the following criteria:
1. $P$ is both syntactically and semantically correct for $i$;
2. $g$ corresponds to the CFG of $P$; and
3. executing $P(i)$ deterministically follows $\pi$ and produces $o$.

This reformulation is motivated by two reasons.

+ Although runtime semantics are fixed for a given input, compilers must reason about all possible executions when performing optimizations. Semantic reification thus exposes compiler bugs that arise from incorrect reasoning about control flow or data flow, while still guaranteeing that every generated program behaves deterministically and remains free of UB for the given input. Here, we generally use UB to denote behaviors that are not defined by a programming language's specification. In the context of C,
this includes undefined, unspecified, and implementation-defined behavior.
+ Allowing arbitrary CFGs and execution paths results in more complex data flows. This further enriches the semantic behaviors available for compiler optimizations. In summary, semantic reification differs from existing program generators in three ways: (1) it inherently supports arbitrary control flow,
including unbounded loops and irreducible regions; (2) in the presence of them, it further ensures that a generated program is both well-defined and guaranteed to terminate under a generated input;
and (3) by producing an expected output, it also enables direct validation of compiler correctness
without relying on pseudo-oracles, such as differential or metamorphic testing.


## Implementation

Given a CFG $g$ and an EP $\pi$, Reify first populates each basic block in $g$ with random statements and terminating jumps that respect the structure of $g$. It then leverages *symbolic execution* to derive a path condition and compute an input $i$ that forces the program to follow the selected path $\pi$ and produce $o$. Importantly, this symbolic execution step explores only the single EP $\pi$, rather than all possible EPs in $P$. Therefore, it does not cause the classic path explosion problem encountered in traditional symbolic execution.

Reify separate individual/leaf function generation from whole program generation: the former generates compact, *leaf functions* that do not call other functions, while the latter rewrites these functions with inter-procedural function calls and combines them into more complex programs using arbitrary call graphs (CGs).

### Leaf Function Creation

To produce a leaf function $f$, we begin by generating a random CFG $g$ and sampling a random EP $\pi$ from $g$. Each basic block in $g$ is then populated with a sequence of statements that are free of function calls. Each block ends with a jump that connects it to its successors, ensuring consistency with the structure of $g$. Notably, every statement initially contains *symbol* s8 (such as `s1` in statement `z = s1*x + s2/y - s3;`) whose values are not yet determined. To assign concrete values, we performs symbolic execution along the selected path $\pi$, collecting both path constraints (to enforce the terminating traversal of $\pi$) and well-definedness constraints (to ensure the absence of UB). In this way, $f$’s control flow and data flow are reified through the symbols and their associated constraints. Finally, the collected constraints are solved using an SMT solver to obtain the concrete input $i$ and output $o$, resulting in a function where all symbols are replaced by concrete values consistent with $g$ and $\pi$.

### Whole-Program Generation

The above process yields a set of independent functions. To construct a complete program $P$, we first generate a random CG that defines caller-callee relationships among some of these functions. For each caller-callee pair in the CG, the caller function is then modified to invoke the callee $f$ through semantics-preserving peephole rewriting: an arbitrary constant $c$ in the caller is replaced with the expression $f(i) + (c - o)$. This ensures that the rewriting expression evaluates to the same value as the original constant while establishing an inter-procedural call.

## Example

We walk through a concrete example to illustrate how Reify generates the following C program with the specified input and output.

```c
int func(int b) {              // input=-2147483647
entry:
  int a = 0, c = 0;
B1:
  c = a * 1 + b * 1 + 2147483647;
  if (c * 1 + 0 <= 0) goto B3;
B2:
  goto exit;
B3:
  a = c * 1 + 1;
  goto B1;
exit:
  // compute a/b/c's checksum and return it
  // suppose checksum(a, b, c) = a + b + c
  return a + b + c;
}                              // output=-2147483645
```

The code defines a function `func`, which takes a parameter `b`, and consists of several basic blocks connected via `goto` statements; two local variables `a` and `c` are defined in `entry`. Basic blocks `B1` and `B3` form a loop (lines 6 and 11) that terminates once the condition on line 6 becomes false. When calling `func(-2147483647)`, the execution follows `entry -> B1 -> B3 -> B1 -> B2 -> exit` and terminates with `a=1`, `b=-2147483647`, and `c=1`.

## Overview

Reify consists of the following steps.

### S1: CFG Generation

Reify begins by generating a random CFG with five basic blocks.

```cfg
               ┌─────────┐
               │         │
               v         │
BB:entry ──> BB:B1 ──> BB:B3
               │
               v
             BB:B2 ──> BB:exit
```

The CFG includes a cycle `B1 <-> B3` that corresponds to a reducible loop.

Reify can also leverage existing CFG without generating it randomly.

### S2: Path Sampling

Reify then samples an EP from the CFG.

```ep
entry ─> B1 ─> B3 ─> B2 ─> exit
```

This path corresponds to a concrete EP, starting at its entry node and ending at its exit node. For this path, it includes repeated nodes, i.e., node `B1`. This indicates that the loop `B1 <-> B3` is executed with two iterations.

### S3: Leaf Function Creation

The next step populates the CFG into a *symbolic leaf function* using SymIR. SymIR is an intermediate representation we designed for Reify to represent symbolic functions (and programs in the future). Its goal is to easy the manipulation and reasoning of symbolic constructs. So currently, it does not support non-linear operations, pointers, or complex data structures, but these are planned for future versions.

```symir
(fun func i32 (par b i32)      // s0: input symbol
  (loc a s1 i32)               // s1: symbol
  (loc c s2 i32)               // s2: symbol

  (bbl B1
    (asn c (eadd
      (mul a s3)               // s3=symbol
      (mul b s4)               // s4=symbol
      (cst s5 i32)             // s5=symbol
    ))
    (brh B2 B3
     (gtz (eadd
       (mul c s6)              // s6=symbol
       (cst s7 i32)            // s7=symbol
     ))
    )
  )

  (bbl B2
    (goto exit)
  )

  (bbl B3
    (asn a (eadd
      (mul c s8)               // s8=symbol
      (cst s9 i32)             // s9=symbol
    ))
    (goto B1)
  )

  (bbl exit
    // compute a/b/c's checksum and return it
    // suppose checksum(a, b, c) = a + b + c
    (ret)                      // s10: output symbol
  )
)
```

It is a leaf function that does not contain any function calls. Each basic block consists of symbolic statements such as local variable declarations (`loc`) and assignments (`asn`), followed by a jump statement (`brh` and `goto`) connecting it to subsequent blocks. These statements use *symbols* (represented by `si`), which represent concrete values that are not yet known or determined. The symbols `s0` and `s10` denote the input and output of `func`, respectively.

### S4: Leaf Function Concretization

This step concretizes the symbolic function into the concrete function:

```symir
(fun func i32 (par b i32)      // s0=-2147483647
  (loc a 0 i32)                // s1=0
  (loc c 0 i32)                // s2=0

  (bbl B1
    (asn c (eadd
      (mul a 1)                // s3=1
      (mul b 0)                // s4=0
      (cst 2147483647 i32)     // s5=2147483647
    ))
    (brh B2 B3
     (gtz (eadd
       (mul c 1)              // s6=1
       (cst 0 i32)            // s7=1
     ))
    )
  )

  (bbl B2
    (goto exit)
  )

  (bbl B3
    (asn a (eadd
      (mul c 1)                // s8=1
      (cst 1 i32)              // s9=1
    ))
    (goto B1)
  )

  (bbl exit
    // compute a/b/c's checksum and return it
    // suppose checksum(a, b, c) = a + b + c
    (ret)                      // s10=-2147483645
  )
)
```

Reify assigns each symbol with a concrete value. Reify leverages *symbolic execution* to encode path conditions, computation, and well-definedness of all operations as constraints along the sampled EP. Through symbolic execution, we obtain the input-output pair for the function: when executed with `s0=-2147483647`, it produces `s10=-2147483645`.

After this, Reify will lower it into a concret C function, as shown previously.

### S5: Whole-Program Generation

Reify leverages S1 to S4 to generate an individual leaf function. The process can be independently applied multiple times to generate several functions. We then combine them into a whole program by creating a random call graph (CG) and applying *semantics-preserving peephole rewriting* according to the generated input and output.

For example, a constant `-2147483647` in a generated caller function can be rewritten by `func(-2147483647) - 2`, as `func(-2147483647)` returns `-2147483645`. This rewrite establishes an inter-procedural call relationship, while preserving the value of the original constant.

### S6: Compiler Validation

Once Reify generates a concrete program, it compiles and executes it using the generated input. It then validates whether the program output matches the generated output. For example, if compiling the program with `-O3` and running it yields `-2147483646`, which differs from the expected value `-2147483645`, Reify reports a potential compiler bug.

Of course, one can also use differential testing to validate the compiler using Reify-generated programs. For example, differential via
