#include "Bytecode.h"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

string BytecodeInstruction::toString(size_t index) const
{
    ostringstream output;
    output << setw(4) << setfill('0') << index << "  ";

    switch (opcode)
    {
        case BytecodeOpCode::DECLARE: output << "DECLARE " << dataType << " " << operand; break;
        case BytecodeOpCode::BIND_PARAM: output << "BIND_PARAM " << dataType << " " << operand; break;
        case BytecodeOpCode::PUSH_INT: output << "PUSH_INT " << operand; break;
        case BytecodeOpCode::PUSH_FLOAT: output << "PUSH_FLOAT " << operand; break;
        case BytecodeOpCode::PUSH_BOOL: output << "PUSH_BOOL " << operand; break;
        case BytecodeOpCode::PUSH_STRING: output << "PUSH_STRING " << operand; break;
        case BytecodeOpCode::LOAD: output << "LOAD " << operand; break;
        case BytecodeOpCode::LOAD_TEMP: output << "LOAD_TEMP " << operand; break;
        case BytecodeOpCode::STORE: output << "STORE " << operand; break;
        case BytecodeOpCode::STORE_TEMP: output << "STORE_TEMP " << operand; break;
        case BytecodeOpCode::POSITIVE: output << "POSITIVE"; break;
        case BytecodeOpCode::NEGATE: output << "NEGATE"; break;
        case BytecodeOpCode::NOT: output << "NOT"; break;
        case BytecodeOpCode::ADD: output << "ADD"; break;
        case BytecodeOpCode::SUBTRACT: output << "SUBTRACT"; break;
        case BytecodeOpCode::MULTIPLY: output << "MULTIPLY"; break;
        case BytecodeOpCode::DIVIDE: output << "DIVIDE"; break;
        case BytecodeOpCode::MODULO: output << "MODULO"; break;
        case BytecodeOpCode::EQUAL: output << "EQUAL"; break;
        case BytecodeOpCode::NOT_EQUAL: output << "NOT_EQUAL"; break;
        case BytecodeOpCode::LESS: output << "LESS"; break;
        case BytecodeOpCode::LESS_EQUAL: output << "LESS_EQUAL"; break;
        case BytecodeOpCode::GREATER: output << "GREATER"; break;
        case BytecodeOpCode::GREATER_EQUAL: output << "GREATER_EQUAL"; break;
        case BytecodeOpCode::AND: output << "AND"; break;
        case BytecodeOpCode::OR: output << "OR"; break;
        case BytecodeOpCode::PRINT: output << "PRINT"; break;
        case BytecodeOpCode::JUMP:
            output << "JUMP " << setw(4) << setfill('0') << target;
            break;
        case BytecodeOpCode::JUMP_IF_FALSE:
            output << "JUMP_IF_FALSE " << setw(4) << setfill('0') << target;
            break;
        case BytecodeOpCode::CALL:
            output << "CALL " << operand << " " << setw(4) << setfill('0') << target
                   << " args=" << dataType;
            break;
        case BytecodeOpCode::RETURN:
            output << "RETURN " << dataType;
            break;
        case BytecodeOpCode::HALT: output << "HALT"; break;
    }

    return output.str();
}

void BytecodeProgram::emit(const BytecodeInstruction& instruction)
{
    instructions.push_back(instruction);
}

vector<BytecodeInstruction>& BytecodeProgram::getMutableInstructions()
{
    return instructions;
}

const vector<BytecodeInstruction>& BytecodeProgram::getInstructions() const
{
    return instructions;
}

void BytecodeProgram::print() const
{
    for (size_t index = 0; index < instructions.size(); index++)
        cout << instructions[index].toString(index) << endl;
}
