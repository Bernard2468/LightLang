#ifndef BYTECODE_FILE_H
#define BYTECODE_FILE_H

#include <stdexcept>
#include <string>
#include "Bytecode.h"

using namespace std;

class BytecodeFileError : public runtime_error
{
public:
    explicit BytecodeFileError(const string& message);
};

class BytecodeFile
{
private:
    static string opcodeToName(BytecodeOpCode opcode);
    static BytecodeOpCode nameToOpcode(const string& name);
    static void validate(const BytecodeProgram& program);

public:
    static void write(const BytecodeProgram& program, const string& filename);
    static BytecodeProgram read(const string& filename);
};

#endif
