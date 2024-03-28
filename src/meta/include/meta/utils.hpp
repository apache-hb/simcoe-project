#pragma once

#include "clang/Basic/Diagnostic.h"

namespace clang {
    class Sema;
    class Decl;
    class ParsedAttr;
    class DiagnosticsEngine;
}

namespace sm::meta {
    constexpr inline const char *kClassTag = "meta_class";
    constexpr inline const char *kInterfaceTag = "meta_interface";
    constexpr inline const char *kEnumTag = "meta_enum";
    constexpr inline const char *kPropertyTag = "meta_property";
    constexpr inline const char *kMethodTag = "meta_method";
    constexpr inline const char *kCaseTag = "meta_case";

    // these `is` prefixed functions dont report errors when they return false

    // decl is a class and has a `meta` annotation
    // used to check that a class is *already* reflected
    bool isReflectedClass(const clang::Decl &D);
    bool isReflectedEnum(const clang::Decl &D);
    bool isReflectedInterface(const clang::Decl &D);

    // decl is a class
    // and has a virtual destructor
    // used to check that a class can be reflected
    bool isInterface(const clang::Decl &D);

    // decl is a class
    // if it inherits from a class, make sure that class is reflected
    // same for interfaces
    bool canBeReflected(const clang::Decl &D);

    // these `verify` prefixed functions report errors when they return false

    // verify that `isReflectedClass` returns true
    bool verifyClassIsReflected(clang::Sema &S, const clang::Decl &D, const clang::ParsedAttr &Attr);
    bool verifyEnumIsReflected(clang::Sema &S, const clang::Decl &D, const clang::ParsedAttr &Attr);

    bool verifyValidReflectInterface(
        clang::DiagnosticsEngine &DE,
        const clang::Decl &D,
        clang::SourceLocation loc);

    bool verifyValidReflectClass(
        clang::DiagnosticsEngine &DE,
        const clang::Decl &D,
        clang::SourceLocation loc);

    bool verifyValidReflectEnum(
        clang::DiagnosticsEngine &DE,
        const clang::Decl &D,
        clang::SourceLocation loc);
}
