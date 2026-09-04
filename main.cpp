#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "AST.h"
#include "Bytecode.h"
#include "BytecodeFile.h"
#include "BytecodeGenerator.h"
#include "Lexer.h"
#include "IRGenerator.h"
#include "IntermediateCode.h"
#include "Optimizer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "SymbolTable.h"
#include "Token.h"
#include "Types.h"
#include "VirtualMachine.h"

using namespace std;

namespace
{
    enum class CommandMode
    {
        COMPILE_VERBOSE,
        COMPILE_AND_RUN,
        COMPILE_TO_FILE,
        EXECUTE_BYTECODE,
        INSPECT_BYTECODE,
        HELP
    };

    struct CommandLineOptions
    {
        CommandMode mode;
        string inputFilename;
        string outputFilename;
    };

    void printBanner()
    {
        cout << "======================================" << endl;
        cout << "       LightLang Compiler v2.0       " << endl;
        cout << "======================================" << endl;
    }

    void printUsage()
    {
        cout << "Usage:" << endl;
        cout << "  lightlang <source.lw>" << endl;
        cout << "  lightlang --run <source.lw>" << endl;
        cout << "  lightlang compile <source.lw>" << endl;
        cout << "  lightlang compile <source.lw> -o <output.lbc>" << endl;
        cout << "  lightlang run <source.lw>" << endl;
        cout << "  lightlang exec <program.lbc>" << endl;
        cout << "  lightlang inspect <program.lbc>" << endl;
        cout << "  lightlang --help" << endl;
    }

    string defaultBytecodeFilename(const string& sourceFilename)
    {
        size_t separator = sourceFilename.find_last_of("/\\");
        size_t dot = sourceFilename.find_last_of('.');

        if (dot == string::npos ||
            (separator != string::npos && dot < separator))
        {
            return sourceFilename + ".lbc";
        }

        return sourceFilename.substr(0, dot) + ".lbc";
    }

    bool parseCommandLine(int argc, char* argv[], CommandLineOptions& options)
    {
        if (argc == 2)
        {
            string first = argv[1];

            if (first == "--help" || first == "-h" || first == "help")
            {
                options = {CommandMode::HELP, "", ""};
                return true;
            }

            options = {CommandMode::COMPILE_VERBOSE, first, ""};
            return true;
        }

        if (argc == 3)
        {
            string command = argv[1];
            string input = argv[2];

            if (command == "--run" || command == "run")
            {
                options = {CommandMode::COMPILE_AND_RUN, input, ""};
                return true;
            }

            if (command == "compile")
            {
                options = {CommandMode::COMPILE_TO_FILE,
                           input,
                           defaultBytecodeFilename(input)};
                return true;
            }

            if (command == "exec")
            {
                options = {CommandMode::EXECUTE_BYTECODE, input, ""};
                return true;
            }

            if (command == "inspect")
            {
                options = {CommandMode::INSPECT_BYTECODE, input, ""};
                return true;
            }

            return false;
        }

        if (argc == 5 && string(argv[1]) == "compile" &&
            string(argv[3]) == "-o")
        {
            if (string(argv[4]).empty())
            {
                return false;
            }

            options = {CommandMode::COMPILE_TO_FILE, argv[2], argv[4]};
            return true;
        }

        return false;
    }

    int executeBytecodeFile(const string& filename, bool inspectOnly)
    {
        try
        {
            BytecodeProgram bytecode = BytecodeFile::read(filename);

            cout << endl;
            cout << "BYTECODE FILE" << endl;
            cout << "--------------------------------------" << endl;
            bytecode.print();
            cout << "--------------------------------------" << endl;
            cout << "Bytecode file loaded successfully: " << filename << endl;

            if (inspectOnly)
            {
                return 0;
            }

            cout << endl;
            cout << "PROGRAM OUTPUT" << endl;
            cout << "--------------------------------------" << endl;

            VirtualMachine virtualMachine;
            virtualMachine.execute(bytecode);

            cout << "--------------------------------------" << endl;
            cout << "Bytecode execution completed successfully." << endl;
            return 0;
        }
        catch (const VMRuntimeError& exception)
        {
            cout << endl;
            cout << exception.what() << endl;
            cout << "Program execution stopped: runtime error detected." << endl;
            return 2;
        }
        catch (const BytecodeFileError& exception)
        {
            cout << endl;
            cout << exception.what() << endl;
            return 1;
        }
        catch (const exception& exception)
        {
            cout << endl;
            cout << "Internal compiler error: " << exception.what() << endl;
            return 1;
        }
    }
}

int main(int argc, char* argv[])
{
    printBanner();

    CommandLineOptions options;

    if (!parseCommandLine(argc, argv, options))
    {
        printUsage();
        return 1;
    }

    if (options.mode == CommandMode::HELP)
    {
        printUsage();
        return 0;
    }

    if (options.mode == CommandMode::EXECUTE_BYTECODE)
    {
        return executeBytecodeFile(options.inputFilename, false);
    }

    if (options.mode == CommandMode::INSPECT_BYTECODE)
    {
        return executeBytecodeFile(options.inputFilename, true);
    }

    bool runProgram = options.mode == CommandMode::COMPILE_AND_RUN;
    bool saveBytecode = options.mode == CommandMode::COMPILE_TO_FILE;
    string filename = options.inputFilename;

    ifstream file(filename);

    if (!file.is_open())
    {
        cout << "Error: Could not open source file '" << filename << "'." << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string sourceCode = buffer.str();
    file.close();

    Lexer lexer(sourceCode);
    vector<Token> tokens = lexer.tokenize();

    bool lexicalError = false;

    cout << endl;
    cout << "TOKENS" << endl;
    cout << "--------------------------------------" << endl;

    for (const Token& token : tokens)
    {
        if (token.type == TokenType::END_OF_FILE)
        {
            break;
        }

        cout << "Line " << token.line
             << " | " << tokenTypeToString(token.type)
             << " | " << token.lexeme << endl;

        if (token.type == TokenType::UNKNOWN)
        {
            lexicalError = true;
        }
    }

    cout << "--------------------------------------" << endl;

    if (lexicalError)
    {
        cout << "Compilation stopped: lexical error detected." << endl;
        return 1;
    }

    cout << "Lexical analysis completed successfully." << endl;

    try
    {
        Parser parser(tokens);
        Program program = parser.parse();

        cout << endl;
        cout << "ABSTRACT SYNTAX TREE" << endl;
        cout << "--------------------------------------" << endl;
        program.print();
        cout << "--------------------------------------" << endl;
        cout << "Syntax analysis completed successfully." << endl;

        SemanticAnalyzer semanticAnalyzer;
        bool semanticSuccess = semanticAnalyzer.analyze(program);

        if (!semanticSuccess)
        {
            cout << endl;
            cout << "SEMANTIC ERRORS" << endl;
            cout << "--------------------------------------" << endl;

            for (const string& error : semanticAnalyzer.getErrors())
            {
                cout << error << endl;
            }

            cout << "--------------------------------------" << endl;
            cout << "Compilation stopped: semantic error detected." << endl;
            return 1;
        }

        cout << endl;
        cout << "SYMBOL TABLE" << endl;
        cout << "-----------------------------------------------" << endl;
        cout << left
             << setw(8) << "Scope"
             << setw(16) << "Name"
             << setw(12) << "Type"
             << "Declared Line" << endl;
        cout << "-----------------------------------------------" << endl;

        for (const Symbol& symbol : semanticAnalyzer.getSymbolTable().getDeclarations())
        {
            cout << left
                 << setw(8) << symbol.scopeDepth
                 << setw(16) << symbol.name
                 << setw(12) << valueTypeToString(symbol.type)
                 << symbol.declarationLine << endl;
        }

        cout << "-----------------------------------------------" << endl;

        cout << endl;
        cout << "FUNCTION TABLE" << endl;
        cout << "---------------------------------------------------------------" << endl;
        cout << left
             << setw(18) << "Name"
             << setw(12) << "Returns"
             << setw(12) << "Parameters"
             << "Declared Line" << endl;
        cout << "---------------------------------------------------------------" << endl;

        for (const FunctionSymbol& function : semanticAnalyzer.getFunctionDeclarations())
        {
            cout << left
                 << setw(18) << function.name
                 << setw(12) << valueTypeToString(function.returnType)
                 << setw(12) << function.parameterTypes.size()
                 << function.declarationLine << endl;
        }

        cout << "---------------------------------------------------------------" << endl;
        cout << "Semantic analysis completed successfully." << endl;
        cout << "Front-end compilation completed successfully." << endl;

        IRGenerator irGenerator;
        IntermediateCode intermediateCode = irGenerator.generate(program);

        cout << endl;
        cout << "THREE-ADDRESS INTERMEDIATE CODE" << endl;
        cout << "--------------------------------------" << endl;
        intermediateCode.print();
        cout << "--------------------------------------" << endl;
        cout << "Intermediate-code generation completed successfully." << endl;

        Optimizer optimizer;
        IntermediateCode optimizedCode = optimizer.optimize(intermediateCode);

        cout << endl;
        cout << "OPTIMIZED THREE-ADDRESS CODE" << endl;
        cout << "--------------------------------------" << endl;
        optimizedCode.print();
        cout << "--------------------------------------" << endl;
        cout << "Intermediate-code optimization completed successfully." << endl;

        BytecodeGenerator bytecodeGenerator;
        BytecodeProgram bytecode = bytecodeGenerator.generate(optimizedCode);

        cout << endl;
        cout << "TARGET BYTECODE" << endl;
        cout << "--------------------------------------" << endl;
        bytecode.print();
        cout << "--------------------------------------" << endl;
        cout << "Target-code generation completed successfully." << endl;

        if (saveBytecode)
        {
            BytecodeFile::write(bytecode, options.outputFilename);
            cout << "Bytecode saved successfully: " << options.outputFilename << endl;
        }

        if (runProgram)
        {
            cout << endl;
            cout << "PROGRAM OUTPUT" << endl;
            cout << "--------------------------------------" << endl;

            VirtualMachine virtualMachine;
            virtualMachine.execute(bytecode);

            cout << "--------------------------------------" << endl;
            cout << "Program execution completed successfully." << endl;
        }

        cout << endl;
        cout << "Compilation completed successfully." << endl;
    }
    catch (const VMRuntimeError& exception)
    {
        cout << endl;
        cout << exception.what() << endl;
        cout << "Program execution stopped: runtime error detected." << endl;
        return 2;
    }
    catch (const ParserError& exception)
    {
        cout << endl;
        cout << exception.what() << endl;
        cout << "Compilation stopped: syntax error detected." << endl;
        return 1;
    }
    catch (const BytecodeFileError& exception)
    {
        cout << endl;
        cout << exception.what() << endl;
        return 1;
    }
    catch (const exception& exception)
    {
        cout << endl;
        cout << "Internal compiler error: " << exception.what() << endl;
        return 1;
    }

    return 0;
}
