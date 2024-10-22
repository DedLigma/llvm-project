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

bool PointerFixedAddressCheck::isPointerAddressFixed(const Expr *RVal) {
  if (const auto *InitList = llvm::dyn_cast<InitListExpr>(RVal)) {
    if (InitList->getNumInits() == 1) {
      RVal = InitList->getInit(0)->IgnoreImpCasts();
    }
  } else {
    RVal = RVal->IgnoreImpCasts();
  }

  if (isa<CXXNullPtrLiteralExpr>(RVal->IgnoreCasts())) {
    return false;
  }

  if (const auto *IntLiteral =
          llvm::dyn_cast<IntegerLiteral>(RVal->IgnoreCasts())) {
    if (IntLiteral->getValue() == 0) {
      return false;
    }
  }

  if (!RVal->IgnoreCasts()->getType()->isPointerType() &&
      RVal->getType()->isPointerType() && RVal->isPRValue() &&
      !isa<CallExpr>(RVal)) {
    if (const auto *UnaryOp =
            llvm::dyn_cast<UnaryOperator>(RVal->IgnoreCasts())) {
      if (UnaryOp->getOpcode() == UO_AddrOf) {
        return false;
      }
    }
    return true;
  }
  return false;
}

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

  Finder->addMatcher(cxxConstructorDecl().bind("pointerConstructorCall"), this);

  Finder->addMatcher(cxxConstructExpr(hasDeclaration(cxxConstructorDecl().bind(
                                          "pointerConstructorCall")))
                         .bind("pointerConstructorCall"),
                     this);
}

void PointerFixedAddressCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *PtrAssignment =
      Result.Nodes.getNodeAs<BinaryOperator>("pointerAssignment");
  if (PtrAssignment) {
    const auto *RVal = PtrAssignment->getRHS();
    if (isPointerAddressFixed(RVal)) {
      diag(PtrAssignment->getExprLoc(), "pointerAssignment",
           DiagnosticIDs::Warning);
    }
  }

  const auto *PtrInitialization =
      Result.Nodes.getNodeAs<VarDecl>("pointerInitialization");
  if (PtrInitialization) {
    const auto *InitVal = PtrInitialization->getInit();
    if (isPointerAddressFixed(InitVal)) {
      diag(PtrInitialization->getLocation(), "pointerInitialization",
           DiagnosticIDs::Warning);
    }
  }

  const auto *PtrDereference =
      Result.Nodes.getNodeAs<UnaryOperator>("pointerDereference");
  if (PtrDereference) {
    const auto *InitVal = PtrDereference->getSubExpr();
    if (isPointerAddressFixed(InitVal)) {
      diag(PtrDereference->getExprLoc(), "PtrDereference",
           DiagnosticIDs::Warning);
    }
  }

  const auto *PtrFuncCall = Result.Nodes.getNodeAs<CallExpr>("pointerFuncCall");
  if (PtrFuncCall) {
    for (const auto *Arg : PtrFuncCall->arguments()) {
      if (isPointerAddressFixed(Arg)) {
        diag(Arg->getExprLoc(), "PtrFuncCall", DiagnosticIDs::Warning);
      }
    }
  }

  const auto *ConstructorDecl =
      Result.Nodes.getNodeAs<CXXConstructorDecl>("pointerConstructorCall");
  if (ConstructorDecl) {
    for (const auto *Init : ConstructorDecl->inits()) {
      const auto *InitExpr = Init->getInit()->IgnoreImpCasts();
      if (isPointerAddressFixed(InitExpr)) {
        diag(ConstructorDecl->getLocation(), "pointerInitialization",
             DiagnosticIDs::Warning);
      }
    }
  }

  const auto *PtrConstructorCall =
      Result.Nodes.getNodeAs<CXXConstructExpr>("pointerConstructorCall");
  if (PtrConstructorCall) {
    for (const auto *Arg : PtrConstructorCall->arguments()) {
      if (isPointerAddressFixed(Arg)) {
        diag(PtrConstructorCall->getExprLoc(), "pointerConstructorCall",
             DiagnosticIDs::Warning);
      }
    }
  }
}

} // namespace clang::tidy::misc
