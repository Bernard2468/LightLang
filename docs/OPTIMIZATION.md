# LightLang IR Optimization

The optimizer performs conservative semantics-preserving transformations:

- constant folding;
- typed constant propagation;
- algebraic simplification;
- Boolean simplification;
- constant string concatenation;
- constant branch elimination;
- unreachable-code cleanup after unconditional jumps;
- redundant-goto removal;
- unreferenced-label removal;
- dead constant-temporary cleanup;
- local common subexpression elimination.

Safety rules preserve trapping operations such as division by zero and signed integer
overflow rather than folding them incorrectly.

## Common subexpression elimination

Within a single basic block, a repeated pure computation over operands that have not
changed must yield the same value, so later occurrences are removed and their
temporaries rewritten to the first result:

```
%t2 = a * b          %t2 = a * b
%t3 = a * b     ->
%t4 = %t2 + %t3      %t4 = %t2 + %t2
```

Correctness rests on three rules:

1. Availability is discarded at every label, jump, call, return, and function
   boundary, so the pass never reuses a value across a control-flow edge. This keeps
   it sound without a full dataflow analysis.
2. Any instruction that writes a name invalidates every recorded expression computed
   from that name, so `a = a + 10` prevents reuse of an earlier `a * b`.
3. Trapping operations stay safe. If the first evaluation would trap, execution stops
   there and the eliminated duplicate was never reachable, so no trap is lost.

Stage 8 is function-aware. Parameter types are tracked, function arguments can receive
propagated constants, function-call results retain their declared return type, and
constant state is cleared across CALL and function boundaries. This prevents unsafe
interprocedural assumptions while preserving local optimizations.
