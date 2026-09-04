#include "IntermediateCode.h"
#include <iostream>
#include <sstream>

using namespace std;

string IRInstruction::toString() const
{
    stringstream output;

    switch (opcode)
    {
        case IROpCode::DECLARE:
            output << "declare " << dataType << " " << destination;
            break;
        case IROpCode::ASSIGN:
            output << destination << " = " << operand1;
            break;
        case IROpCode::UNARY:
            output << destination << " = " << operation << operand1;
            break;
        case IROpCode::BINARY:
            output << destination << " = " << operand1 << " " << operation << " " << operand2;
            break;
        case IROpCode::PRINT:
            output << "print " << operand1;
            break;
        case IROpCode::LABEL:
            output << label << ":";
            break;
        case IROpCode::GOTO:
            output << "goto " << label;
            break;
        case IROpCode::IF_FALSE_GOTO:
            output << "ifFalse " << operand1 << " goto " << label;
            break;
        case IROpCode::FUNCTION_BEGIN:
            output << "function " << dataType << " " << label << ":";
            break;
        case IROpCode::PARAM:
            output << "param " << dataType << " " << destination;
            break;
        case IROpCode::ARG:
            output << "arg " << operand1;
            break;
        case IROpCode::CALL:
            output << destination << " = call " << label << ", " << operation;
            break;
        case IROpCode::RETURN:
            output << "return " << operand1;
            break;
        case IROpCode::FUNCTION_END:
            output << "end function " << label;
            break;
    }

    return output.str();
}

void IntermediateCode::emit(const IRInstruction& instruction)
{
    instructions.push_back(instruction);
}

const vector<IRInstruction>& IntermediateCode::getInstructions() const
{
    return instructions;
}

void IntermediateCode::print() const
{
    for (const IRInstruction& instruction : instructions)
        cout << instruction.toString() << endl;
}
