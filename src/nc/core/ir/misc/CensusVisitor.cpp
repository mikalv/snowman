/* The file is part of Snowman decompiler.             */
/* See doc/licenses.txt for the licensing information. */

//
// SmartDec decompiler - SmartDec is a native code to C/C++ decompiler
// Copyright (C) 2015 Alexander Chernov, Katerina Troshina, Yegor Derevenets,
// Alexander Fokin, Sergey Levin, Leonid Tsvetkov
//
// This file is part of SmartDec decompiler.
//
// SmartDec decompiler is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SmartDec decompiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SmartDec decompiler.  If not, see <http://www.gnu.org/licenses/>.
//

#include "CensusVisitor.h"

#include <nc/common/Foreach.h>

#include <nc/core/ir/BasicBlock.h>
#include <nc/core/ir/Function.h>
#include <nc/core/ir/Functions.h>
#include <nc/core/ir/Statements.h>
#include <nc/core/ir/Term.h>

#include <nc/core/ir/cconv/CallAnalyzer.h>
#include <nc/core/ir/cconv/CallsData.h>
#include <nc/core/ir/cconv/EnterHook.h>
#include <nc/core/ir/cconv/ReturnAnalyzer.h>

namespace nc {
namespace core {
namespace ir {
namespace misc {

void CensusVisitor::operator()(const Functions *functions) {
    assert(functions != NULL);

    foreach (auto function, functions->functions()) {
        (*this)(function);
    }
}

void CensusVisitor::operator()(const Function *function) {
    assert(function != NULL);

    assert(currentFunction_ == NULL);
    currentFunction_ = function;

    foreach (const BasicBlock *basicBlock, function->basicBlocks()) {
        (*this)(basicBlock);
    }

    if (callsData()) {
        if (cconv::EnterHook *enterHook = callsData()->getEnterHook(function)) {
            enterHook->visitChildStatements(*this);
            enterHook->visitChildTerms(*this);
        }
    }

    currentFunction_ = NULL;
}

void CensusVisitor::operator()(const BasicBlock *basicBlock) {
    assert(basicBlock != NULL);

    foreach (const Statement *statement, basicBlock->statements()) {
        (*this)(statement);
    }
}

void CensusVisitor::operator()(const Statement *statement) {
    assert(statement != NULL);

    statements_.push_back(statement);
    statement->visitChildTerms(*this);

    if (callsData()) {
        switch (statement->kind()) {
            case Statement::CALL:
                if (cconv::CallAnalyzer *callAnalyzer = callsData()->getCallAnalyzer(statement->asCall())) {
                    callAnalyzer->visitChildStatements(*this);
                    callAnalyzer->visitChildTerms(*this);
                }
                break;
            case Statement::RETURN: {
                if (cconv::ReturnAnalyzer *returnAnalyzer = callsData()->getReturnAnalyzer(currentFunction_, statement->asReturn())) {
                    returnAnalyzer->visitChildStatements(*this);
                    returnAnalyzer->visitChildTerms(*this);
                }
                break;
            }
        }
    }
}

void CensusVisitor::operator()(const Term *term) {
    assert(term != NULL);

    terms_.push_back(term);
    term->visitChildTerms(*this);
}

void CensusVisitor::clear() {
    statements_.clear();
    terms_.clear();
}

} // namespace misc
} // namespace ir
} // namespace core
} // namespace nc

/* vim:set et sts=4 sw=4: */
