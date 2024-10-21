//===--- PointerFixedAddressCheck.cpp - clang-tidy ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PointerFixedAddressCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/IdentifierTable.h"
#include "llvm/Support/Casting.h"

using namespace clang::ast_matchers;

namespace clang::tidy::misc {

void PointerFixedAddressCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      binaryOperator(hasOperatorName("="), hasLHS(hasType(pointerType())))
          .bind("pointerAssignment"),
      this);

  Finder->addMatcher(varDecl(hasType(pointerType()), hasInitializer(expr()))
                         .bind("pointerInitialization"),
                     this);

  Finder->addMatcher(unaryOperator(hasOperatorName("*"), hasType(pointerType()))
                         .bind("pointerDereference"),
                     this);

  Finder->addMatcher(
      callExpr(hasArgument(0, hasType(pointerType()))).bind("pointerFuncCall"),
      this);
}

void PointerFixedAddressCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *PtrAssignment =
      Result.Nodes.getNodeAs<BinaryOperator>("pointerAssignment");

  const auto *PtrInitialization =
      Result.Nodes.getNodeAs<VarDecl>("pointerInitialization");

  const auto *PtrDereference =
      Result.Nodes.getNodeAs<UnaryOperator>("pointerDereference");

  const auto *PtrFuncCall = Result.Nodes.getNodeAs<CallExpr>("pointerFuncCall");

  if (PtrAssignment) {
    const auto *Rval = PtrAssignment->getRHS()->IgnoreImpCasts();
    if (Rval->getType()->isPointerType() && Rval->isPRValue() &&
        !isa<CallExpr>(Rval) && !llvm::dyn_cast<UnaryOperator>(Rval)) {
      diag(PtrAssignment->getExprLoc(), "pointerAssignment",
           DiagnosticIDs::Warning);
    }
  }
}

// if (PtrInitialization) {
//   diag(PtrInitialization->getLocation(), "pointerInitialization",
//        DiagnosticIDs::Warning);
// }

// if (PtrDereference) {
//   diag(PtrDereference->getExprLoc(), "pointerDereference",
//        DiagnosticIDs::Warning);
// }

// if (PtrFuncCall) {
//   diag(PtrFuncCall->getExprLoc(), "pointerFuncCall",
//   DiagnosticIDs::Warning);
// }
// }

} // namespace clang::tidy::misc
