#ifndef BYTECODE_H
#define BYTECODE_H

#include <cstddef>
#include <string>
#include <vector>

using namespace std;

enum class BytecodeOpCode
{
    DECLARE,
    BIND_PARAM,
    PUSH_INT,
    PUSH_FLOAT,
    PUSH_BOOL,
    PUSH_STRING,
    LOAD,
    LOAD_TEMP,
    STORE,
    STORE_TEMP,
    POSITIVE,
    NEGATE,
    NOT,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    MODULO,
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    AND,
    OR,
    PRINT,
    JUMP,
    JUMP_IF_FALSE,
    CALL,
    RETURN,
    HALT
};

struct BytecodeInstruction
{
    BytecodeOpCode opcode;
    string operand;
    string dataType;
    size_t target;
    int sourceLine;

    string toString(size_t index) const;
};

class BytecodeProgram
{
private:
    vector<BytecodeInstruction> instructions;

public:
    void emit(const BytecodeInstruction& instruction);
    vector<BytecodeInstruction>& getMutableInstructions();
    const vector<BytecodeInstruction>& getInstructions() const;
    void print() const;
};

#endif
