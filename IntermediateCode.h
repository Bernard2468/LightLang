#ifndef INTERMEDIATE_CODE_H
#define INTERMEDIATE_CODE_H

#include <string>
#include <vector>

using namespace std;

enum class IROpCode
{
    DECLARE,
    ASSIGN,
    UNARY,
    BINARY,
    PRINT,
    LABEL,
    GOTO,
    IF_FALSE_GOTO,
    FUNCTION_BEGIN,
    PARAM,
    ARG,
    CALL,
    RETURN,
    FUNCTION_END
};

struct IRInstruction
{
    IROpCode opcode;
    string destination;
    string operand1;
    string operand2;
    string operation;
    string dataType;
    string label;
    int sourceLine;

    string toString() const;
};

class IntermediateCode
{
private:
    vector<IRInstruction> instructions;

public:
    void emit(const IRInstruction& instruction);
    const vector<IRInstruction>& getInstructions() const;
    void print() const;
};

#endif
