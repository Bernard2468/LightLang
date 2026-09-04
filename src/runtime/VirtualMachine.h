#ifndef VIRTUAL_MACHINE_H
#define VIRTUAL_MACHINE_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "backend/Bytecode.h"
#include "runtime/RuntimeValue.h"
#include "common/Types.h"

using namespace std;

class VMRuntimeError : public runtime_error
{
public:
    VMRuntimeError(int line, const string& message);
};

class VirtualMachine
{
private:
    struct VariableSlot
    {
        ValueType declaredType;
        RuntimeValue value;
    };

    struct CallFrame
    {
        unordered_map<string, VariableSlot> variables;
        unordered_map<string, RuntimeValue> temporaries;
        size_t returnAddress;
        vector<RuntimeValue> arguments;
        size_t nextArgument;
        string functionName;
    };

    vector<CallFrame> frames;
    vector<RuntimeValue> stack;
    size_t instructionPointer;
    size_t executedSteps;
    size_t maxInstructionSteps;
    size_t maxCallDepth;

    void runtimeError(const BytecodeInstruction& instruction,
                      const string& message) const;
    RuntimeValue pop(const BytecodeInstruction& instruction);
    void push(const RuntimeValue& value);
    CallFrame& currentFrame();
    const CallFrame& currentFrame() const;
    VariableSlot* findVariable(const string& name);
    const VariableSlot* findVariable(const string& name) const;

    ValueType parseType(const string& text,
                        const BytecodeInstruction& instruction) const;
    RuntimeValue defaultValue(ValueType type) const;
    RuntimeValue convertForVariable(const RuntimeValue& value,
                                    ValueType destinationType,
                                    const BytecodeInstruction& instruction) const;

    string decodeStringLiteral(const string& text,
                               const BytecodeInstruction& instruction) const;
    RuntimeValue parseIntLiteral(const string& text,
                                 const BytecodeInstruction& instruction) const;
    RuntimeValue parseFloatLiteral(const string& text,
                                   const BytecodeInstruction& instruction) const;

    long long checkedAdd(long long left, long long right,
                         const BytecodeInstruction& instruction) const;
    long long checkedSubtract(long long left, long long right,
                              const BytecodeInstruction& instruction) const;
    long long checkedMultiply(long long left, long long right,
                              const BytecodeInstruction& instruction) const;

    RuntimeValue numericBinary(BytecodeOpCode opcode,
                               const RuntimeValue& left,
                               const RuntimeValue& right,
                               const BytecodeInstruction& instruction) const;
    RuntimeValue compareBinary(BytecodeOpCode opcode,
                               const RuntimeValue& left,
                               const RuntimeValue& right,
                               const BytecodeInstruction& instruction) const;
    RuntimeValue equalityBinary(BytecodeOpCode opcode,
                                const RuntimeValue& left,
                                const RuntimeValue& right,
                                const BytecodeInstruction& instruction) const;

    void executeInstruction(const BytecodeInstruction& instruction,
                            const vector<BytecodeInstruction>& instructions);

public:
    explicit VirtualMachine(size_t maxSteps = 10000000);
    void execute(const BytecodeProgram& program);
};

#endif
