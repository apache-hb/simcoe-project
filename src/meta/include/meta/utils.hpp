#pragma once

namespace clang {
    class Sema;
    class Decl;
    class ParsedAttr;
}

namespace sm::meta {
    // these `is` prefixed functions dont report errors when they return false

    // decl is a class and has a `meta` annotation
    // used to check that a class is *already* reflected
    bool isReflectedClass(const clang::Decl &D);

    // decl is a class and has a `meta_interface` annotation
    // used to check that a class is *already* reflected
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

    // verify that `isReflectedInterface` returns true
    bool verifyValidReflectInterface(const clang::Sema &S, const clang::Decl &D, const clang::ParsedAttr &Attr);
    bool verifyValidReflectClass(const clang::Sema &Sema, const clang::Decl &D, const clang::ParsedAttr &Attr);
}
