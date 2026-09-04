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
- dead constant-temporary cleanup.

Safety rules preserve trapping operations such as division by zero and signed integer
overflow rather than folding them incorrectly.

Stage 8 is function-aware. Parameter types are tracked, function arguments can receive
propagated constants, function-call results retain their declared return type, and
constant state is cleared across CALL and function boundaries. This prevents unsafe
interprocedural assumptions while preserving local optimizations.
