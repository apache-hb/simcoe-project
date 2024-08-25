#pragma once

// IWYU pragma: begin_exports

#include <vector>
#include <utility>
#include <filesystem>
#include <fstream>

#include <fmtlib/format.h>

#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Attr.h>

#include <clang/Sema/Sema.h>
#include <clang/Sema/SemaDiagnostic.h>
#include <clang/Sema/ParsedAttr.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>

// IWYU pragma: end_exports
