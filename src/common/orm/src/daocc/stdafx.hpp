#pragma once

// IWYU pragma: begin_exports

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Comment.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/ParsedAttr.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <fmtlib/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <fstream>
#include <filesystem>
#include <map>

// IWYU pragma: end_exports
