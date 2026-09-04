#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include "Types.h"

using namespace std;

struct Symbol
{
    string name;
    ValueType type;
    int declarationLine;
    int scopeDepth;
};

class SymbolTable
{
private:
    vector<unordered_map<string, Symbol>> scopes;
    vector<Symbol> declarations;

public:
    SymbolTable();

    void enterScope();
    void exitScope();

    int currentScopeDepth() const;
    bool isDeclaredInCurrentScope(const string& name) const;
    bool declare(const string& name, ValueType type, int declarationLine);

    const Symbol* lookup(const string& name) const;
    const vector<Symbol>& getDeclarations() const;
};

#endif
