#include "SemanticAnalyzer.h"


bool SemanticAnalyzer::analyze(Module& module)
{
    errors.clear();
    module.accept(*this);
    return errors.empty();
}


const std::vector<std::string>& SemanticAnalyzer::getErrors() const
{
    return errors;
}


void SemanticAnalyzer::addError(const std::string& error)
{
    errors.push_back(error);
}
