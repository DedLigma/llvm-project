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
#include "clang/AST/Stmt.h"
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
  Finder->addMatcher(varDecl(hasType(pointerType()), hasInitializer(expr()))
                         .bind("pointerInitialization"),
                     this);

  Finder->addMatcher(returnStmt(hasReturnValue(expr(hasType(pointerType()))))
                         .bind("pointerReturn"),
                     this);

  Finder->addMatcher(
      binaryOperator(anyOf(hasOperatorName("="), hasOperatorName("+"),hasOperatorName("-"), hasOperatorName("*"),hasOperatorName("/"))).bind("pointerOperators"),
      this);

  Finder->addMatcher(
      callExpr(hasArgument(0, hasType(pointerType()))).bind("pointerFuncCall"),
      this);

  Finder->addMatcher(cxxConstructorDecl().bind("pointerConstructorCall"), this);

  Finder->addMatcher(cxxConstructExpr(hasDeclaration(cxxConstructorDecl()))
                         .bind("pointerConstructorCall"),
                     this);

  Finder->addMatcher(fieldDecl().bind("pointerFieldInitialization"), this);
}

void PointerFixedAddressCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *PtrOperators =
      Result.Nodes.getNodeAs<BinaryOperator>("pointerOperators");
  if (PtrOperators) {
    const auto *RVal = PtrOperators->getRHS();
    const auto *LVal = PtrOperators->getLHS();
    if (RVal && isPointerAddressFixed(RVal)) {
      diag(RVal->getBeginLoc(), "Operation with pointer with fixed address",
           DiagnosticIDs::Warning);
    }
    if (LVal && isPointerAddressFixed(LVal)) {
      diag(LVal->getBeginLoc(), "Operation with pointer with fixed address",
           DiagnosticIDs::Warning);
    }
  }

  const auto *PtrInitialization =
      Result.Nodes.getNodeAs<VarDecl>("pointerInitialization");
  if (PtrInitialization) {
    const auto *InitVal = PtrInitialization->getInit();
    if (InitVal && isPointerAddressFixed(InitVal)) {
      diag(PtrInitialization->getLocation(),
           "Initializing the pointer with the fixed address",
           DiagnosticIDs::Warning);
    }
  }

  const auto *PtrReturn = Result.Nodes.getNodeAs<ReturnStmt>("pointerReturn");
  if (PtrReturn) {
    const auto *RetVal = PtrReturn->getRetValue();
    if (RetVal && isPointerAddressFixed(RetVal)) {
      diag(PtrReturn->getReturnLoc(),
           "The return value of a pointer is a fixed address",
           DiagnosticIDs::Warning);
    }
  }

  const auto *PtrFuncCall = Result.Nodes.getNodeAs<CallExpr>("pointerFuncCall");
  if (PtrFuncCall) {
    for (const auto *Arg : PtrFuncCall->arguments()) {
      if (Arg && isPointerAddressFixed(Arg)) {
        diag(Arg->getExprLoc(),
             "The pointer in the argument has a fixed address",
             DiagnosticIDs::Warning);
      }
    }
  }

  const auto *ConstructorDecl =
      Result.Nodes.getNodeAs<CXXConstructorDecl>("pointerConstructorCall");
  if (ConstructorDecl) {
    for (const auto *Init : ConstructorDecl->inits()) {
      const auto *InitExpr = Init->getInit()->IgnoreImpCasts();
      if (InitExpr && isPointerAddressFixed(InitExpr)) {
        diag(ConstructorDecl->getLocation(),
             "The initialization list contains a fixed pointer address",
             DiagnosticIDs::Warning);
      }
    }
  }

  const auto *PtrConstructorCall =
      Result.Nodes.getNodeAs<CXXConstructExpr>("pointerConstructorCall");
  if (PtrConstructorCall) {
    for (const auto *Arg : PtrConstructorCall->arguments()) {
      if (Arg && isPointerAddressFixed(Arg)) {
        diag(PtrConstructorCall->getExprLoc(),
             "Constructor for class contains a fixed pointer address",
             DiagnosticIDs::Warning);
      }
    }
  }

  const auto *PtrFieldInitialization =
      Result.Nodes.getNodeAs<FieldDecl>("pointerFieldInitialization");
  if (PtrFieldInitialization) {
    const auto *InitExpr =
        PtrFieldInitialization->getInClassInitializer()->IgnoreImpCasts();
    if (InitExpr && isPointerAddressFixed(InitExpr)) {
      diag(PtrFieldInitialization->getLocation(),
           "Field in class has initialization with fixed address",
           DiagnosticIDs::Warning);
    }
  }
}

} // namespace clang::tidy::misc
