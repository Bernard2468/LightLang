#include "BytecodeGenerator.h"
#include <cctype>
#include <limits>
#include <stdexcept>

using namespace std;

bool BytecodeGenerator::isTemporary(const string& operand) const
{
    return operand.size() >= 3 && operand[0] == '%' && operand[1] == 't';
}

bool BytecodeGenerator::isStringLiteral(const string& operand) const
{
    return operand.size() >= 2 && operand.front() == '"' && operand.back() == '"';
}

bool BytecodeGenerator::isBoolLiteral(const string& operand) const
{
    return operand == "true" || operand == "false";
}

bool BytecodeGenerator::isNumericLiteral(const string& operand) const
{
    if (operand.empty()) return false;
    size_t index = 0;
    if (operand[index] == '+' || operand[index] == '-') index++;
    if (index >= operand.size()) return false;

    bool hasDigit = false;
    bool hasDot = false;
    bool hasExponent = false;
    for (; index < operand.size(); index++)
    {
        char character = operand[index];
        if (isdigit(static_cast<unsigned char>(character)))
        {
            hasDigit = true;
            continue;
        }
        if (character == '.' && !hasDot && !hasExponent)
        {
            hasDot = true;
            continue;
        }
        if ((character == 'e' || character == 'E') && !hasExponent && hasDigit)
        {
            hasExponent = true;
            hasDigit = false;
            if (index + 1 < operand.size() &&
                (operand[index + 1] == '+' || operand[index + 1] == '-'))
                index++;
            continue;
        }
        return false;
    }
    return hasDigit;
}

bool BytecodeGenerator::isFloatLiteral(const string& operand) const
{
    return operand.find('.') != string::npos ||
           operand.find('e') != string::npos || operand.find('E') != string::npos;
}

void BytecodeGenerator::emitOperand(const string& operand, int sourceLine)
{
    if (isStringLiteral(operand))
    {
        program.emit({BytecodeOpCode::PUSH_STRING, operand, "", 0, sourceLine});
        return;
    }
    if (isBoolLiteral(operand))
    {
        program.emit({BytecodeOpCode::PUSH_BOOL, operand, "", 0, sourceLine});
        return;
    }
    if (isNumericLiteral(operand))
    {
        BytecodeOpCode opcode = isFloatLiteral(operand)
                                    ? BytecodeOpCode::PUSH_FLOAT
                                    : BytecodeOpCode::PUSH_INT;
        program.emit({opcode, operand, "", 0, sourceLine});
        return;
    }
    if (isTemporary(operand))
    {
        program.emit({BytecodeOpCode::LOAD_TEMP, operand, "", 0, sourceLine});
        return;
    }
    if (operand.empty())
        throw runtime_error("Bytecode generation error: empty operand.");
    program.emit({BytecodeOpCode::LOAD, operand, "", 0, sourceLine});
}

void BytecodeGenerator::emitStore(const string& destination, int sourceLine)
{
    if (destination.empty())
        throw runtime_error("Bytecode generation error: empty destination.");
    if (isTemporary(destination))
        program.emit({BytecodeOpCode::STORE_TEMP, destination, "", 0, sourceLine});
    else
        program.emit({BytecodeOpCode::STORE, destination, "", 0, sourceLine});
}

BytecodeOpCode BytecodeGenerator::unaryOpcode(const string& operation) const
{
    if (operation == "+") return BytecodeOpCode::POSITIVE;
    if (operation == "-") return BytecodeOpCode::NEGATE;
    if (operation == "!") return BytecodeOpCode::NOT;
    throw runtime_error("Bytecode generation error: unsupported unary operator '" + operation + "'.");
}

BytecodeOpCode BytecodeGenerator::binaryOpcode(const string& operation) const
{
    if (operation == "+") return BytecodeOpCode::ADD;
    if (operation == "-") return BytecodeOpCode::SUBTRACT;
    if (operation == "*") return BytecodeOpCode::MULTIPLY;
    if (operation == "/") return BytecodeOpCode::DIVIDE;
    if (operation == "%") return BytecodeOpCode::MODULO;
    if (operation == "==") return BytecodeOpCode::EQUAL;
    if (operation == "!=") return BytecodeOpCode::NOT_EQUAL;
    if (operation == "<") return BytecodeOpCode::LESS;
    if (operation == "<=") return BytecodeOpCode::LESS_EQUAL;
    if (operation == ">") return BytecodeOpCode::GREATER;
    if (operation == ">=") return BytecodeOpCode::GREATER_EQUAL;
    if (operation == "&&") return BytecodeOpCode::AND;
    if (operation == "||") return BytecodeOpCode::OR;
    throw runtime_error("Bytecode generation error: unsupported binary operator '" + operation + "'.");
}

void BytecodeGenerator::translateInstruction(const IRInstruction& instruction)
{
    switch (instruction.opcode)
    {
        case IROpCode::DECLARE:
            program.emit({BytecodeOpCode::DECLARE, instruction.destination,
                          instruction.dataType, 0, instruction.sourceLine});
            break;
        case IROpCode::ASSIGN:
            emitOperand(instruction.operand1, instruction.sourceLine);
            emitStore(instruction.destination, instruction.sourceLine);
            break;
        case IROpCode::UNARY:
            emitOperand(instruction.operand1, instruction.sourceLine);
            program.emit({unaryOpcode(instruction.operation), "", "", 0, instruction.sourceLine});
            emitStore(instruction.destination, instruction.sourceLine);
            break;
        case IROpCode::BINARY:
            emitOperand(instruction.operand1, instruction.sourceLine);
            emitOperand(instruction.operand2, instruction.sourceLine);
            program.emit({binaryOpcode(instruction.operation), "", "", 0, instruction.sourceLine});
            emitStore(instruction.destination, instruction.sourceLine);
            break;
        case IROpCode::PRINT:
            emitOperand(instruction.operand1, instruction.sourceLine);
            program.emit({BytecodeOpCode::PRINT, "", "", 0, instruction.sourceLine});
            break;
        case IROpCode::LABEL:
            if (labels.find(instruction.label) != labels.end())
                throw runtime_error("Bytecode generation error: duplicate label '" + instruction.label + "'.");
            labels[instruction.label] = program.getInstructions().size();
            break;
        case IROpCode::GOTO:
        {
            size_t index = program.getInstructions().size();
            program.emit({BytecodeOpCode::JUMP, instruction.label, "",
                          numeric_limits<size_t>::max(), instruction.sourceLine});
            pendingJumps.push_back({index, instruction.label});
            break;
        }
        case IROpCode::IF_FALSE_GOTO:
        {
            emitOperand(instruction.operand1, instruction.sourceLine);
            size_t index = program.getInstructions().size();
            program.emit({BytecodeOpCode::JUMP_IF_FALSE, instruction.label, "",
                          numeric_limits<size_t>::max(), instruction.sourceLine});
            pendingJumps.push_back({index, instruction.label});
            break;
        }
        case IROpCode::FUNCTION_BEGIN:
            if (functionEntries.find(instruction.label) != functionEntries.end())
                throw runtime_error("Bytecode generation error: duplicate function '" + instruction.label + "'.");
            functionEntries[instruction.label] = program.getInstructions().size();
            break;
        case IROpCode::PARAM:
            program.emit({BytecodeOpCode::BIND_PARAM, instruction.destination,
                          instruction.dataType, 0, instruction.sourceLine});
            break;
        case IROpCode::ARG:
            emitOperand(instruction.operand1, instruction.sourceLine);
            break;
        case IROpCode::CALL:
        {
            size_t index = program.getInstructions().size();
            program.emit({BytecodeOpCode::CALL, instruction.label,
                          instruction.operation,
                          numeric_limits<size_t>::max(), instruction.sourceLine});
            pendingCalls.push_back({index, instruction.label});
            emitStore(instruction.destination, instruction.sourceLine);
            break;
        }
        case IROpCode::RETURN:
            emitOperand(instruction.operand1, instruction.sourceLine);
            program.emit({BytecodeOpCode::RETURN, "", instruction.dataType, 0,
                          instruction.sourceLine});
            break;
        case IROpCode::FUNCTION_END:
            break;
    }
}

void BytecodeGenerator::resolveTargets()
{
    vector<BytecodeInstruction>& instructions = program.getMutableInstructions();

    for (const PendingTarget& jump : pendingJumps)
    {
        auto found = labels.find(jump.name);
        if (found == labels.end())
            throw runtime_error("Bytecode generation error: unresolved label '" + jump.name + "'.");
        if (jump.instructionIndex >= instructions.size())
            throw runtime_error("Bytecode generation error: invalid jump patch index.");
        instructions[jump.instructionIndex].target = found->second;
        instructions[jump.instructionIndex].operand.clear();
    }

    for (const PendingTarget& call : pendingCalls)
    {
        auto found = functionEntries.find(call.name);
        if (found == functionEntries.end())
            throw runtime_error("Bytecode generation error: unresolved function '" + call.name + "'.");
        if (call.instructionIndex >= instructions.size())
            throw runtime_error("Bytecode generation error: invalid call patch index.");
        instructions[call.instructionIndex].target = found->second;
    }
}

BytecodeProgram BytecodeGenerator::generate(const IntermediateCode& input)
{
    program = BytecodeProgram();
    labels.clear();
    functionEntries.clear();
    pendingJumps.clear();
    pendingCalls.clear();

    const vector<IRInstruction>& instructions = input.getInstructions();
    size_t firstFunction = instructions.size();
    for (size_t index = 0; index < instructions.size(); index++)
    {
        if (instructions[index].opcode == IROpCode::FUNCTION_BEGIN)
        {
            firstFunction = index;
            break;
        }
    }

    for (size_t index = 0; index < firstFunction; index++)
        translateInstruction(instructions[index]);

    // Main execution stops here; function bodies live after this HALT and are
    // entered only through CALL instructions.
    program.emit({BytecodeOpCode::HALT, "", "", 0, 0});

    for (size_t index = firstFunction; index < instructions.size(); index++)
        translateInstruction(instructions[index]);

    // A final HALT keeps the persisted bytecode structurally self-contained.
    // Correctly compiled functions return before reaching it.
    program.emit({BytecodeOpCode::HALT, "", "", 0, 0});
    resolveTargets();
    return program;
}
