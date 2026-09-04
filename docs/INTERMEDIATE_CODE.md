# LightLang Three-Address Intermediate Representation

LightLang lowers the AST into a compact three-address representation before target-code
generation. Compiler temporaries use `%tN`; control-flow labels use `LN`.

Core forms:

```text
declare int x
x = 5
%t1 = x * 2
ifFalse %t1 goto L1
goto L2
L1:
print x
```

Stage 8 adds function forms:

```text
function int add:
param int a
param int b
%t1 = a + b
return %t1
end function add

arg 5
arg 7
%t2 = call add, 2
print %t2
```

Function bodies are emitted after top-level executable IR. The bytecode generator places
a main-program HALT before function bodies and patches CALL instructions to the correct
function entry addresses.
