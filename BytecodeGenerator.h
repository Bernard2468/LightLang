#ifndef BYTECODE_GENERATOR_H
#define BYTECODE_GENERATOR_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include "Bytecode.h"
#include "IntermediateCode.h"

using namespace std;

class BytecodeGenerator
{
private:
    struct PendingTarget
    {
        size_t instructionIndex;
        string name;
    };

    BytecodeProgram program;
    unordered_map<string, size_t> labels;
    unordered_map<string, size_t> functionEntries;
    vector<PendingTarget> pendingJumps;
    vector<PendingTarget> pendingCalls;

    bool isTemporary(const string& operand) const;
    bool isStringLiteral(const string& operand) const;
    bool isBoolLiteral(const string& operand) const;
    bool isNumericLiteral(const string& operand) const;
    bool isFloatLiteral(const string& operand) const;

    void emitOperand(const string& operand, int sourceLine);
    void emitStore(const string& destination, int sourceLine);
    BytecodeOpCode unaryOpcode(const string& operation) const;
    BytecodeOpCode binaryOpcode(const string& operation) const;
    void translateInstruction(const IRInstruction& instruction);
    void resolveTargets();

public:
    BytecodeProgram generate(const IntermediateCode& input);
};

#endif
