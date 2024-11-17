#pragma once

#include "system/registry.hpp"

#include "db/connection.hpp"

#include <vector>
#include <span>
#include <map>

namespace sm::windows::contextmenu {
    struct OpenWithProgramId {
        std::string progid;
        std::string name;
    };

    struct FileClass {
        std::string extension;
        std::string name;
        std::string contentType;
        std::string perceivedType;
        std::string persistentHandler;
        std::vector<std::string> openWithProgIds;
        std::map<std::string, std::string> shellExCommands;
    };

    struct ClassId {
        std::string clsid;
        std::string name;
    };

    struct MimeType {
        std::string contentType;
        std::string extension;
        std::string clsid;
        std::string autoplay;
    };

    struct RegistryClasses {
        FileClass global;
        FileClass directory;
        std::vector<MimeType> mimetypes;
        std::vector<FileClass> classes;
        std::vector<ClassId> clsids;
        std::vector<OpenWithProgramId> programs;
    };

    struct InstalledProgram {
        std::string appxProgramId;
        std::string path;
    };

    struct ContextMenuRegistry {
        RegistryClasses global;
        RegistryClasses user;

        ContextMenuRegistry();

        void save(db::Connection& db);
    };
}
