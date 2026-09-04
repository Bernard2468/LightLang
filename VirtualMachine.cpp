#include "VirtualMachine.h"
#include <cmath>
#include <limits>
#include <sstream>
#include <iostream>

using namespace std;

VMRuntimeError::VMRuntimeError(int line, const string& message)
    : runtime_error(line > 0
                        ? "Runtime error at line " + to_string(line) + ": " + message
                        : "Runtime error: " + message)
{
}

VirtualMachine::VirtualMachine(size_t maxSteps)
    : frames(), stack(), instructionPointer(0), executedSteps(0),
      maxInstructionSteps(maxSteps), maxCallDepth(1024)
{
}

void VirtualMachine::runtimeError(const BytecodeInstruction& instruction,
                                  const string& message) const
{
    throw VMRuntimeError(instruction.sourceLine, message);
}

RuntimeValue VirtualMachine::pop(const BytecodeInstruction& instruction)
{
    if (stack.empty())
    {
        runtimeError(instruction, "operand stack underflow.");
    }

    RuntimeValue value = stack.back();
    stack.pop_back();
    return value;
}

void VirtualMachine::push(const RuntimeValue& value)
{
    stack.push_back(value);
}


VirtualMachine::CallFrame& VirtualMachine::currentFrame()
{
    if (frames.empty())
        throw runtime_error("VM internal error: no active call frame.");
    return frames.back();
}

const VirtualMachine::CallFrame& VirtualMachine::currentFrame() const
{
    if (frames.empty())
        throw runtime_error("VM internal error: no active call frame.");
    return frames.back();
}

VirtualMachine::VariableSlot* VirtualMachine::findVariable(const string& name)
{
    for (auto frame = frames.rbegin(); frame != frames.rend(); ++frame)
    {
        auto found = frame->variables.find(name);
        if (found != frame->variables.end()) return &found->second;
    }
    return nullptr;
}

const VirtualMachine::VariableSlot* VirtualMachine::findVariable(const string& name) const
{
    for (auto frame = frames.rbegin(); frame != frames.rend(); ++frame)
    {
        auto found = frame->variables.find(name);
        if (found != frame->variables.end()) return &found->second;
    }
    return nullptr;
}

ValueType VirtualMachine::parseType(const string& text,
                                    const BytecodeInstruction& instruction) const
{
    if (text == "int") return ValueType::INT;
    if (text == "float") return ValueType::FLOAT;
    if (text == "bool") return ValueType::BOOL;
    if (text == "string") return ValueType::STRING;

    runtimeError(instruction, "unknown runtime type '" + text + "'.");
    return ValueType::ERROR;
}

RuntimeValue VirtualMachine::defaultValue(ValueType type) const
{
    if (type == ValueType::INT) return RuntimeValue(0LL);
    if (type == ValueType::FLOAT) return RuntimeValue(0.0);
    if (type == ValueType::BOOL) return RuntimeValue(false);
    if (type == ValueType::STRING) return RuntimeValue(string(""));

    throw runtime_error("VM internal error: cannot create default value.");
}

RuntimeValue VirtualMachine::convertForVariable(
    const RuntimeValue& value,
    ValueType destinationType,
    const BytecodeInstruction& instruction) const
{
    if (value.getType() == destinationType)
    {
        return value;
    }

    if (destinationType == ValueType::FLOAT && value.getType() == ValueType::INT)
    {
        return RuntimeValue(static_cast<double>(value.asInt()));
    }

    runtimeError(instruction,
                 "cannot store value of type '" + valueTypeToString(value.getType()) +
                 "' in variable of type '" + valueTypeToString(destinationType) + "'.");
    return RuntimeValue();
}

string VirtualMachine::decodeStringLiteral(
    const string& text,
    const BytecodeInstruction& instruction) const
{
    if (text.size() < 2 || text.front() != '"' || text.back() != '"')
    {
        runtimeError(instruction, "malformed string constant in bytecode.");
    }

    string result;

    for (size_t index = 1; index + 1 < text.size(); index++)
    {
        char character = text[index];

        if (character != '\\')
        {
            result += character;
            continue;
        }

        if (index + 1 >= text.size() - 1)
        {
            runtimeError(instruction, "malformed escape sequence in bytecode string.");
        }

        index++;
        char escaped = text[index];

        switch (escaped)
        {
            case '\\': result += '\\'; break;
            case '"': result += '"'; break;
            case 'n': result += '\n'; break;
            case 't': result += '\t'; break;
            case 'r': result += '\r'; break;
            default:
                result += '\\';
                result += escaped;
                break;
        }
    }

    return result;
}

RuntimeValue VirtualMachine::parseIntLiteral(
    const string& text,
    const BytecodeInstruction& instruction) const
{
    try
    {
        size_t consumed = 0;
        long long value = stoll(text, &consumed);

        if (consumed != text.size())
        {
            runtimeError(instruction, "invalid integer constant '" + text + "'.");
        }

        return RuntimeValue(value);
    }
    catch (const invalid_argument&)
    {
        runtimeError(instruction, "invalid integer constant '" + text + "'.");
    }
    catch (const out_of_range&)
    {
        runtimeError(instruction, "integer constant is outside the 64-bit signed range: '" +
                                  text + "'.");
    }

    return RuntimeValue();
}

RuntimeValue VirtualMachine::parseFloatLiteral(
    const string& text,
    const BytecodeInstruction& instruction) const
{
    try
    {
        size_t consumed = 0;
        double value = stod(text, &consumed);

        if (consumed != text.size() || !isfinite(value))
        {
            runtimeError(instruction, "invalid finite float constant '" + text + "'.");
        }

        return RuntimeValue(value);
    }
    catch (const invalid_argument&)
    {
        runtimeError(instruction, "invalid float constant '" + text + "'.");
    }
    catch (const out_of_range&)
    {
        runtimeError(instruction, "float constant is outside the supported range: '" +
                                  text + "'.");
    }

    return RuntimeValue();
}

long long VirtualMachine::checkedAdd(
    long long left,
    long long right,
    const BytecodeInstruction& instruction) const
{
    if ((right > 0 && left > numeric_limits<long long>::max() - right) ||
        (right < 0 && left < numeric_limits<long long>::min() - right))
    {
        runtimeError(instruction, "integer overflow during addition.");
    }

    return left + right;
}

long long VirtualMachine::checkedSubtract(
    long long left,
    long long right,
    const BytecodeInstruction& instruction) const
{
    if ((right > 0 && left < numeric_limits<long long>::min() + right) ||
        (right < 0 && left > numeric_limits<long long>::max() + right))
    {
        runtimeError(instruction, "integer overflow during subtraction.");
    }

    return left - right;
}

long long VirtualMachine::checkedMultiply(
    long long left,
    long long right,
    const BytecodeInstruction& instruction) const
{
    if (left == 0 || right == 0)
    {
        return 0;
    }

    if ((left == -1 && right == numeric_limits<long long>::min()) ||
        (right == -1 && left == numeric_limits<long long>::min()))
    {
        runtimeError(instruction, "integer overflow during multiplication.");
    }

    if (left > 0)
    {
        if ((right > 0 && left > numeric_limits<long long>::max() / right) ||
            (right < 0 && right < numeric_limits<long long>::min() / left))
        {
            runtimeError(instruction, "integer overflow during multiplication.");
        }
    }
    else
    {
        if ((right > 0 && left < numeric_limits<long long>::min() / right) ||
            (right < 0 && right < numeric_limits<long long>::max() / left))
        {
            runtimeError(instruction, "integer overflow during multiplication.");
        }
    }

    return left * right;
}

RuntimeValue VirtualMachine::numericBinary(
    BytecodeOpCode opcode,
    const RuntimeValue& left,
    const RuntimeValue& right,
    const BytecodeInstruction& instruction) const
{
    if (opcode == BytecodeOpCode::ADD &&
        left.getType() == ValueType::STRING &&
        right.getType() == ValueType::STRING)
    {
        return RuntimeValue(left.asString() + right.asString());
    }

    if (!left.isNumeric() || !right.isNumeric())
    {
        runtimeError(instruction, "numeric operation received non-numeric operands.");
    }

    if (opcode == BytecodeOpCode::MODULO)
    {
        if (left.getType() != ValueType::INT || right.getType() != ValueType::INT)
        {
            runtimeError(instruction, "modulo requires int operands.");
        }

        long long divisor = right.asInt();

        if (divisor == 0)
        {
            runtimeError(instruction, "modulo by zero.");
        }

        if (left.asInt() == numeric_limits<long long>::min() && divisor == -1)
        {
            runtimeError(instruction, "integer overflow during modulo.");
        }

        return RuntimeValue(left.asInt() % divisor);
    }

    bool floating = left.getType() == ValueType::FLOAT ||
                    right.getType() == ValueType::FLOAT;

    if (!floating)
    {
        long long leftInt = left.asInt();
        long long rightInt = right.asInt();

        if (opcode == BytecodeOpCode::ADD)
            return RuntimeValue(checkedAdd(leftInt, rightInt, instruction));
        if (opcode == BytecodeOpCode::SUBTRACT)
            return RuntimeValue(checkedSubtract(leftInt, rightInt, instruction));
        if (opcode == BytecodeOpCode::MULTIPLY)
            return RuntimeValue(checkedMultiply(leftInt, rightInt, instruction));
        if (opcode == BytecodeOpCode::DIVIDE)
        {
            if (rightInt == 0)
            {
                runtimeError(instruction, "division by zero.");
            }

            if (leftInt == numeric_limits<long long>::min() && rightInt == -1)
            {
                runtimeError(instruction, "integer overflow during division.");
            }

            return RuntimeValue(leftInt / rightInt);
        }
    }

    double leftNumber = left.asNumber();
    double rightNumber = right.asNumber();

    if (opcode == BytecodeOpCode::DIVIDE && rightNumber == 0.0)
    {
        runtimeError(instruction, "division by zero.");
    }

    double result = 0.0;

    if (opcode == BytecodeOpCode::ADD) result = leftNumber + rightNumber;
    else if (opcode == BytecodeOpCode::SUBTRACT) result = leftNumber - rightNumber;
    else if (opcode == BytecodeOpCode::MULTIPLY) result = leftNumber * rightNumber;
    else if (opcode == BytecodeOpCode::DIVIDE) result = leftNumber / rightNumber;
    else runtimeError(instruction, "unsupported numeric opcode.");

    if (!isfinite(result))
    {
        runtimeError(instruction, "floating-point operation produced a non-finite value.");
    }

    return RuntimeValue(result);
}

RuntimeValue VirtualMachine::compareBinary(
    BytecodeOpCode opcode,
    const RuntimeValue& left,
    const RuntimeValue& right,
    const BytecodeInstruction& instruction) const
{
    if (!left.isNumeric() || !right.isNumeric())
    {
        runtimeError(instruction, "comparison requires numeric operands.");
    }

    if (left.getType() == ValueType::INT && right.getType() == ValueType::INT)
    {
        long long a = left.asInt();
        long long b = right.asInt();

        if (opcode == BytecodeOpCode::LESS) return RuntimeValue(a < b);
        if (opcode == BytecodeOpCode::LESS_EQUAL) return RuntimeValue(a <= b);
        if (opcode == BytecodeOpCode::GREATER) return RuntimeValue(a > b);
        if (opcode == BytecodeOpCode::GREATER_EQUAL) return RuntimeValue(a >= b);
    }
    else
    {
        double a = left.asNumber();
        double b = right.asNumber();

        if (opcode == BytecodeOpCode::LESS) return RuntimeValue(a < b);
        if (opcode == BytecodeOpCode::LESS_EQUAL) return RuntimeValue(a <= b);
        if (opcode == BytecodeOpCode::GREATER) return RuntimeValue(a > b);
        if (opcode == BytecodeOpCode::GREATER_EQUAL) return RuntimeValue(a >= b);
    }

    runtimeError(instruction, "unsupported comparison opcode.");
    return RuntimeValue();
}

RuntimeValue VirtualMachine::equalityBinary(
    BytecodeOpCode opcode,
    const RuntimeValue& left,
    const RuntimeValue& right,
    const BytecodeInstruction& instruction) const
{
    bool equal = false;

    if (left.isNumeric() && right.isNumeric())
    {
        if (left.getType() == ValueType::INT && right.getType() == ValueType::INT)
            equal = left.asInt() == right.asInt();
        else
            equal = left.asNumber() == right.asNumber();
    }
    else if (left.getType() == right.getType())
    {
        if (left.getType() == ValueType::BOOL)
            equal = left.asBool() == right.asBool();
        else if (left.getType() == ValueType::STRING)
            equal = left.asString() == right.asString();
        else
            runtimeError(instruction, "unsupported equality operand type.");
    }
    else
    {
        runtimeError(instruction, "equality received incompatible operand types.");
    }

    return RuntimeValue(opcode == BytecodeOpCode::EQUAL ? equal : !equal);
}

void VirtualMachine::executeInstruction(
    const BytecodeInstruction& instruction,
    const vector<BytecodeInstruction>& instructions)
{
    switch (instruction.opcode)
    {
        case BytecodeOpCode::DECLARE:
        {
            CallFrame& frame = currentFrame();
            if (frame.variables.find(instruction.operand) != frame.variables.end())
            {
                runtimeError(instruction, "duplicate runtime variable '" +
                                          instruction.operand + "'.");
            }
            ValueType type = parseType(instruction.dataType, instruction);
            frame.variables.emplace(instruction.operand,
                                    VariableSlot{type, defaultValue(type)});
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::BIND_PARAM:
        {
            if (frames.size() <= 1)
                runtimeError(instruction, "parameter binding executed outside a function call.");

            CallFrame& frame = currentFrame();
            if (frame.nextArgument >= frame.arguments.size())
                runtimeError(instruction, "function received fewer arguments than required.");
            if (frame.variables.find(instruction.operand) != frame.variables.end())
                runtimeError(instruction, "duplicate runtime parameter '" + instruction.operand + "'.");

            ValueType type = parseType(instruction.dataType, instruction);
            RuntimeValue value = convertForVariable(frame.arguments[frame.nextArgument],
                                                    type, instruction);
            frame.nextArgument++;
            frame.variables.emplace(instruction.operand, VariableSlot{type, value});
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::PUSH_INT:
            push(parseIntLiteral(instruction.operand, instruction));
            instructionPointer++;
            break;

        case BytecodeOpCode::PUSH_FLOAT:
            push(parseFloatLiteral(instruction.operand, instruction));
            instructionPointer++;
            break;

        case BytecodeOpCode::PUSH_BOOL:
            if (instruction.operand == "true") push(RuntimeValue(true));
            else if (instruction.operand == "false") push(RuntimeValue(false));
            else runtimeError(instruction, "invalid bool constant in bytecode.");
            instructionPointer++;
            break;

        case BytecodeOpCode::PUSH_STRING:
            push(RuntimeValue(decodeStringLiteral(instruction.operand, instruction)));
            instructionPointer++;
            break;

        case BytecodeOpCode::LOAD:
        {
            const VariableSlot* found = findVariable(instruction.operand);
            if (found == nullptr)
                runtimeError(instruction, "unknown runtime variable '" + instruction.operand + "'.");
            push(found->value);
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::LOAD_TEMP:
        {
            const auto& temporaries = currentFrame().temporaries;
            auto found = temporaries.find(instruction.operand);
            if (found == temporaries.end())
                runtimeError(instruction, "unknown compiler temporary '" + instruction.operand + "'.");
            push(found->second);
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::STORE:
        {
            RuntimeValue value = pop(instruction);
            VariableSlot* found = findVariable(instruction.operand);
            if (found == nullptr)
                runtimeError(instruction, "store to unknown runtime variable '" + instruction.operand + "'.");
            found->value = convertForVariable(value, found->declaredType, instruction);
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::STORE_TEMP:
            currentFrame().temporaries[instruction.operand] = pop(instruction);
            instructionPointer++;
            break;

        case BytecodeOpCode::POSITIVE:
        {
            RuntimeValue value = pop(instruction);
            if (!value.isNumeric()) runtimeError(instruction, "unary '+' requires a numeric operand.");
            push(value);
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::NEGATE:
        {
            RuntimeValue value = pop(instruction);
            if (value.getType() == ValueType::INT)
            {
                if (value.asInt() == numeric_limits<long long>::min())
                    runtimeError(instruction, "integer overflow during unary negation.");
                push(RuntimeValue(-value.asInt()));
            }
            else if (value.getType() == ValueType::FLOAT)
            {
                double result = -value.asFloat();
                if (!isfinite(result)) runtimeError(instruction, "non-finite result during unary negation.");
                push(RuntimeValue(result));
            }
            else runtimeError(instruction, "unary '-' requires a numeric operand.");
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::NOT:
        {
            RuntimeValue value = pop(instruction);
            if (value.getType() != ValueType::BOOL)
                runtimeError(instruction, "unary '!' requires a bool operand.");
            push(RuntimeValue(!value.asBool()));
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::ADD:
        case BytecodeOpCode::SUBTRACT:
        case BytecodeOpCode::MULTIPLY:
        case BytecodeOpCode::DIVIDE:
        case BytecodeOpCode::MODULO:
        {
            RuntimeValue right = pop(instruction);
            RuntimeValue left = pop(instruction);
            push(numericBinary(instruction.opcode, left, right, instruction));
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::EQUAL:
        case BytecodeOpCode::NOT_EQUAL:
        {
            RuntimeValue right = pop(instruction);
            RuntimeValue left = pop(instruction);
            push(equalityBinary(instruction.opcode, left, right, instruction));
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::LESS:
        case BytecodeOpCode::LESS_EQUAL:
        case BytecodeOpCode::GREATER:
        case BytecodeOpCode::GREATER_EQUAL:
        {
            RuntimeValue right = pop(instruction);
            RuntimeValue left = pop(instruction);
            push(compareBinary(instruction.opcode, left, right, instruction));
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::AND:
        case BytecodeOpCode::OR:
        {
            RuntimeValue right = pop(instruction);
            RuntimeValue left = pop(instruction);
            if (left.getType() != ValueType::BOOL || right.getType() != ValueType::BOOL)
                runtimeError(instruction, "logical operation requires bool operands.");
            bool result = instruction.opcode == BytecodeOpCode::AND
                              ? left.asBool() && right.asBool()
                              : left.asBool() || right.asBool();
            push(RuntimeValue(result));
            instructionPointer++;
            break;
        }

        case BytecodeOpCode::PRINT:
            cout << pop(instruction).toString() << endl;
            instructionPointer++;
            break;

        case BytecodeOpCode::JUMP:
            if (instruction.target >= instructions.size())
                runtimeError(instruction, "jump target is outside bytecode program.");
            instructionPointer = instruction.target;
            break;

        case BytecodeOpCode::JUMP_IF_FALSE:
        {
            RuntimeValue condition = pop(instruction);
            if (condition.getType() != ValueType::BOOL)
                runtimeError(instruction, "conditional jump requires a bool value.");
            if (!condition.asBool())
            {
                if (instruction.target >= instructions.size())
                    runtimeError(instruction, "conditional jump target is outside bytecode program.");
                instructionPointer = instruction.target;
            }
            else instructionPointer++;
            break;
        }

        case BytecodeOpCode::CALL:
        {
            if (instruction.target >= instructions.size())
                runtimeError(instruction, "function call target is outside bytecode program.");
            if (frames.size() >= maxCallDepth)
                runtimeError(instruction, "maximum function call depth exceeded.");

            size_t argumentCount = 0;
            try
            {
                size_t consumed = 0;
                unsigned long long raw = stoull(instruction.dataType, &consumed);
                if (consumed != instruction.dataType.size() || raw > 1024)
                    runtimeError(instruction, "invalid function argument count in bytecode.");
                argumentCount = static_cast<size_t>(raw);
            }
            catch (const exception&)
            {
                runtimeError(instruction, "invalid function argument count in bytecode.");
            }

            if (stack.size() < argumentCount)
                runtimeError(instruction, "operand stack does not contain all function arguments.");

            vector<RuntimeValue> arguments(argumentCount);
            for (size_t index = argumentCount; index > 0; index--)
                arguments[index - 1] = pop(instruction);

            CallFrame frame;
            frame.returnAddress = instructionPointer + 1;
            frame.arguments = move(arguments);
            frame.nextArgument = 0;
            frame.functionName = instruction.operand;
            frames.push_back(move(frame));
            instructionPointer = instruction.target;
            break;
        }

        case BytecodeOpCode::RETURN:
        {
            if (frames.size() <= 1)
                runtimeError(instruction, "return executed outside a function call.");

            RuntimeValue value = pop(instruction);
            ValueType returnType = parseType(instruction.dataType, instruction);
            value = convertForVariable(value, returnType, instruction);

            CallFrame& frame = currentFrame();
            if (frame.nextArgument != frame.arguments.size())
                runtimeError(instruction, "function argument count does not match parameter count.");

            size_t returnAddress = frame.returnAddress;
            frames.pop_back();
            push(value);
            instructionPointer = returnAddress;
            break;
        }

        case BytecodeOpCode::HALT:
            if (frames.size() != 1)
                runtimeError(instruction, "function ended without executing a return statement.");
            instructionPointer = instructions.size();
            break;
    }
}

void VirtualMachine::execute(const BytecodeProgram& program)
{
    frames.clear();
    stack.clear();
    instructionPointer = 0;
    executedSteps = 0;

    CallFrame globalFrame;
    globalFrame.returnAddress = 0;
    globalFrame.nextArgument = 0;
    globalFrame.functionName = "<global>";
    frames.push_back(move(globalFrame));

    const vector<BytecodeInstruction>& instructions = program.getInstructions();

    while (instructionPointer < instructions.size())
    {
        if (executedSteps >= maxInstructionSteps)
            throw VMRuntimeError(0, "execution step limit exceeded; possible infinite loop.");
        executedSteps++;
        executeInstruction(instructions[instructionPointer], instructions);
    }

    if (frames.size() != 1)
        throw VMRuntimeError(0, "function call stack was not empty when execution finished.");
    if (!stack.empty())
        throw VMRuntimeError(0, "operand stack was not empty when execution finished.");
}
