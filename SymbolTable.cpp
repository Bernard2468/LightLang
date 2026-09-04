#include "SymbolTable.h"

using namespace std;

SymbolTable::SymbolTable()
{
    enterScope();
}

void SymbolTable::enterScope()
{
    scopes.emplace_back();
}

void SymbolTable::exitScope()
{
    if (scopes.size() > 1)
    {
        scopes.pop_back();
    }
}

int SymbolTable::currentScopeDepth() const
{
    return static_cast<int>(scopes.size()) - 1;
}

bool SymbolTable::isDeclaredInCurrentScope(const string& name) const
{
    if (scopes.empty())
    {
        return false;
    }

    const auto& currentScope = scopes.back();
    return currentScope.find(name) != currentScope.end();
}

bool SymbolTable::declare(const string& name,
                          ValueType type,
                          int declarationLine)
{
    if (isDeclaredInCurrentScope(name))
    {
        return false;
    }

    Symbol symbol{name, type, declarationLine, currentScopeDepth()};
    scopes.back().emplace(name, symbol);
    declarations.push_back(symbol);
    return true;
}

const Symbol* SymbolTable::lookup(const string& name) const
{
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope)
    {
        auto found = scope->find(name);

        if (found != scope->end())
        {
            return &found->second;
        }
    }

    return nullptr;
}

const vector<Symbol>& SymbolTable::getDeclarations() const
{
    return declarations;
}
