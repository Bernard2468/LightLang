#include "BytecodeFile.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_map>

using namespace std;

namespace
{
    const string BYTECODE_MAGIC = "LIGHTLANG_BYTECODE";
    const unsigned int BYTECODE_VERSION = 2;
    const size_t MAX_BYTECODE_INSTRUCTIONS = 1000000;

    bool isKnownType(const string& type)
    {
        return type == "int" || type == "float" ||
               type == "bool" || type == "string";
    }
}

BytecodeFileError::BytecodeFileError(const string& message)
    : runtime_error("Bytecode file error: " + message)
{
}

string BytecodeFile::opcodeToName(BytecodeOpCode opcode)
{
    switch (opcode)
    {
        case BytecodeOpCode::DECLARE: return "DECLARE";
        case BytecodeOpCode::BIND_PARAM: return "BIND_PARAM";
        case BytecodeOpCode::PUSH_INT: return "PUSH_INT";
        case BytecodeOpCode::PUSH_FLOAT: return "PUSH_FLOAT";
        case BytecodeOpCode::PUSH_BOOL: return "PUSH_BOOL";
        case BytecodeOpCode::PUSH_STRING: return "PUSH_STRING";
        case BytecodeOpCode::LOAD: return "LOAD";
        case BytecodeOpCode::LOAD_TEMP: return "LOAD_TEMP";
        case BytecodeOpCode::STORE: return "STORE";
        case BytecodeOpCode::STORE_TEMP: return "STORE_TEMP";
        case BytecodeOpCode::POSITIVE: return "POSITIVE";
        case BytecodeOpCode::NEGATE: return "NEGATE";
        case BytecodeOpCode::NOT: return "NOT";
        case BytecodeOpCode::ADD: return "ADD";
        case BytecodeOpCode::SUBTRACT: return "SUBTRACT";
        case BytecodeOpCode::MULTIPLY: return "MULTIPLY";
        case BytecodeOpCode::DIVIDE: return "DIVIDE";
        case BytecodeOpCode::MODULO: return "MODULO";
        case BytecodeOpCode::EQUAL: return "EQUAL";
        case BytecodeOpCode::NOT_EQUAL: return "NOT_EQUAL";
        case BytecodeOpCode::LESS: return "LESS";
        case BytecodeOpCode::LESS_EQUAL: return "LESS_EQUAL";
        case BytecodeOpCode::GREATER: return "GREATER";
        case BytecodeOpCode::GREATER_EQUAL: return "GREATER_EQUAL";
        case BytecodeOpCode::AND: return "AND";
        case BytecodeOpCode::OR: return "OR";
        case BytecodeOpCode::PRINT: return "PRINT";
        case BytecodeOpCode::JUMP: return "JUMP";
        case BytecodeOpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case BytecodeOpCode::CALL: return "CALL";
        case BytecodeOpCode::RETURN: return "RETURN";
        case BytecodeOpCode::HALT: return "HALT";
    }

    throw BytecodeFileError("cannot serialize an unknown opcode.");
}

BytecodeOpCode BytecodeFile::nameToOpcode(const string& name)
{
    static const unordered_map<string, BytecodeOpCode> opcodes = {
        {"DECLARE", BytecodeOpCode::DECLARE},
        {"BIND_PARAM", BytecodeOpCode::BIND_PARAM},
        {"PUSH_INT", BytecodeOpCode::PUSH_INT},
        {"PUSH_FLOAT", BytecodeOpCode::PUSH_FLOAT},
        {"PUSH_BOOL", BytecodeOpCode::PUSH_BOOL},
        {"PUSH_STRING", BytecodeOpCode::PUSH_STRING},
        {"LOAD", BytecodeOpCode::LOAD},
        {"LOAD_TEMP", BytecodeOpCode::LOAD_TEMP},
        {"STORE", BytecodeOpCode::STORE},
        {"STORE_TEMP", BytecodeOpCode::STORE_TEMP},
        {"POSITIVE", BytecodeOpCode::POSITIVE},
        {"NEGATE", BytecodeOpCode::NEGATE},
        {"NOT", BytecodeOpCode::NOT},
        {"ADD", BytecodeOpCode::ADD},
        {"SUBTRACT", BytecodeOpCode::SUBTRACT},
        {"MULTIPLY", BytecodeOpCode::MULTIPLY},
        {"DIVIDE", BytecodeOpCode::DIVIDE},
        {"MODULO", BytecodeOpCode::MODULO},
        {"EQUAL", BytecodeOpCode::EQUAL},
        {"NOT_EQUAL", BytecodeOpCode::NOT_EQUAL},
        {"LESS", BytecodeOpCode::LESS},
        {"LESS_EQUAL", BytecodeOpCode::LESS_EQUAL},
        {"GREATER", BytecodeOpCode::GREATER},
        {"GREATER_EQUAL", BytecodeOpCode::GREATER_EQUAL},
        {"AND", BytecodeOpCode::AND},
        {"OR", BytecodeOpCode::OR},
        {"PRINT", BytecodeOpCode::PRINT},
        {"JUMP", BytecodeOpCode::JUMP},
        {"JUMP_IF_FALSE", BytecodeOpCode::JUMP_IF_FALSE},
        {"CALL", BytecodeOpCode::CALL},
        {"RETURN", BytecodeOpCode::RETURN},
        {"HALT", BytecodeOpCode::HALT}
    };

    auto found = opcodes.find(name);

    if (found == opcodes.end())
    {
        throw BytecodeFileError("unknown opcode '" + name + "'.");
    }

    return found->second;
}

void BytecodeFile::validate(const BytecodeProgram& program)
{
    const vector<BytecodeInstruction>& instructions = program.getInstructions();

    if (instructions.empty())
    {
        throw BytecodeFileError("bytecode program is empty.");
    }

    if (instructions.size() > MAX_BYTECODE_INSTRUCTIONS)
    {
        throw BytecodeFileError("bytecode program exceeds the supported instruction limit.");
    }

    if (instructions.back().opcode != BytecodeOpCode::HALT)
    {
        throw BytecodeFileError("bytecode program must end with HALT.");
    }

    for (size_t index = 0; index < instructions.size(); index++)
    {
        const BytecodeInstruction& instruction = instructions[index];

        if (instruction.sourceLine < 0)
        {
            throw BytecodeFileError("instruction " + to_string(index) +
                                    " has an invalid negative source line.");
        }

        if ((instruction.opcode == BytecodeOpCode::JUMP ||
             instruction.opcode == BytecodeOpCode::JUMP_IF_FALSE ||
             instruction.opcode == BytecodeOpCode::CALL) &&
            instruction.target >= instructions.size())
        {
            throw BytecodeFileError("instruction " + to_string(index) +
                                    " has an out-of-range jump target.");
        }

        if (instruction.opcode == BytecodeOpCode::DECLARE ||
            instruction.opcode == BytecodeOpCode::BIND_PARAM)
        {
            string name = instruction.opcode == BytecodeOpCode::DECLARE
                              ? "DECLARE" : "BIND_PARAM";
            if (instruction.operand.empty())
            {
                throw BytecodeFileError(name + " instruction " + to_string(index) +
                                        " has an empty variable name.");
            }

            if (!isKnownType(instruction.dataType))
            {
                throw BytecodeFileError(name + " instruction " + to_string(index) +
                                        " has unknown type '" + instruction.dataType + "'.");
            }
        }

        if (instruction.opcode == BytecodeOpCode::RETURN &&
            !isKnownType(instruction.dataType))
        {
            throw BytecodeFileError("RETURN instruction " + to_string(index) +
                                    " has unknown return type '" +
                                    instruction.dataType + "'.");
        }

        if (instruction.opcode == BytecodeOpCode::CALL)
        {
            if (instruction.operand.empty())
            {
                throw BytecodeFileError("CALL instruction " + to_string(index) +
                                        " has an empty function name.");
            }

            try
            {
                size_t consumed = 0;
                unsigned long long count = stoull(instruction.dataType, &consumed);
                if (consumed != instruction.dataType.size() || count > 1024)
                    throw BytecodeFileError("CALL instruction " + to_string(index) +
                                            " has an invalid argument count.");
            }
            catch (const BytecodeFileError&)
            {
                throw;
            }
            catch (...)
            {
                throw BytecodeFileError("CALL instruction " + to_string(index) +
                                        " has an invalid argument count.");
            }
        }
    }
}

void BytecodeFile::write(const BytecodeProgram& program, const string& filename)
{
    validate(program);

    ofstream file(filename, ios::out | ios::trunc);

    if (!file.is_open())
    {
        throw BytecodeFileError("could not create '" + filename + "'.");
    }

    const vector<BytecodeInstruction>& instructions = program.getInstructions();

    file << BYTECODE_MAGIC << " " << BYTECODE_VERSION << "\n";
    file << "COUNT " << instructions.size() << "\n";

    for (const BytecodeInstruction& instruction : instructions)
    {
        file << opcodeToName(instruction.opcode) << " "
             << instruction.target << " "
             << instruction.sourceLine << " "
             << quoted(instruction.dataType) << " "
             << quoted(instruction.operand) << "\n";
    }

    if (!file.good())
    {
        throw BytecodeFileError("failed while writing '" + filename + "'.");
    }
}

BytecodeProgram BytecodeFile::read(const string& filename)
{
    ifstream file(filename);

    if (!file.is_open())
    {
        throw BytecodeFileError("could not open '" + filename + "'.");
    }

    string headerLine;

    if (!getline(file, headerLine))
    {
        throw BytecodeFileError("missing or malformed file header.");
    }

    istringstream headerInput(headerLine);
    string magic;
    unsigned int version = 0;

    if (!(headerInput >> magic >> version))
    {
        throw BytecodeFileError("missing or malformed file header.");
    }

    headerInput >> ws;
    if (!headerInput.eof())
    {
        throw BytecodeFileError("unexpected data in file header.");
    }

    if (magic != BYTECODE_MAGIC)
    {
        throw BytecodeFileError("invalid file signature.");
    }

    if (version != 1 && version != BYTECODE_VERSION)
    {
        throw BytecodeFileError("unsupported bytecode version " +
                                to_string(version) + ".");
    }

    string countLine;

    if (!getline(file, countLine))
    {
        throw BytecodeFileError("missing or malformed instruction count.");
    }

    istringstream countInput(countLine);
    string countKeyword;
    unsigned long long rawCount = 0;

    if (!(countInput >> countKeyword >> rawCount) || countKeyword != "COUNT")
    {
        throw BytecodeFileError("missing or malformed instruction count.");
    }

    countInput >> ws;
    if (!countInput.eof())
    {
        throw BytecodeFileError("unexpected data after instruction count.");
    }

    if (rawCount == 0 || rawCount > MAX_BYTECODE_INSTRUCTIONS)
    {
        throw BytecodeFileError("instruction count is outside the supported range.");
    }

    if (rawCount > numeric_limits<size_t>::max())
    {
        throw BytecodeFileError("instruction count cannot be represented on this platform.");
    }

    BytecodeProgram program;
    size_t instructionCount = static_cast<size_t>(rawCount);

    for (size_t index = 0; index < instructionCount; index++)
    {
        string line;

        if (!getline(file, line))
        {
            throw BytecodeFileError("file ended before instruction " +
                                    to_string(index) + ".");
        }

        if (line.empty())
        {
            throw BytecodeFileError("instruction " + to_string(index) +
                                    " is empty.");
        }

        istringstream input(line);
        string opcodeName;
        unsigned long long rawTarget = 0;
        long long rawSourceLine = 0;
        string dataType;
        string operand;

        if (!(input >> opcodeName >> rawTarget >> rawSourceLine >>
              quoted(dataType) >> quoted(operand)))
        {
            throw BytecodeFileError("malformed instruction " +
                                    to_string(index) + ".");
        }

        input >> ws;
        if (!input.eof())
        {
            throw BytecodeFileError("unexpected trailing data on instruction " +
                                    to_string(index) + ".");
        }

        if (rawTarget > numeric_limits<size_t>::max())
        {
            throw BytecodeFileError("jump target on instruction " +
                                    to_string(index) +
                                    " cannot be represented on this platform.");
        }

        if (rawSourceLine < 0 || rawSourceLine > numeric_limits<int>::max())
        {
            throw BytecodeFileError("source line on instruction " +
                                    to_string(index) + " is outside the supported range.");
        }

        program.emit({nameToOpcode(opcodeName),
                      operand,
                      dataType,
                      static_cast<size_t>(rawTarget),
                      static_cast<int>(rawSourceLine)});
    }

    string trailing;
    while (getline(file, trailing))
    {
        if (trailing.find_first_not_of(" \t\r\n") != string::npos)
        {
            throw BytecodeFileError("unexpected content after the declared instructions.");
        }
    }

    validate(program);
    return program;
}
