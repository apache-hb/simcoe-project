#include "contextmenu/contextmenu.hpp"

#include "core/error.hpp"

#include "db/transaction.hpp"

#include "contextmenu.dao.hpp"
#include "fmt/ranges.h"

#include <experimental/generator>

using namespace sm;

namespace cm = sm::windows::contextmenu;
namespace cmdao = sm::dao::contextmenu;

// places where file extensions are mapped to programs or mimetypes
// HKEY_CLASSES_ROOT
// HKEY_CURRENT_USER\Software\Classes
// HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\FileExts
// HKEY_LOCAL_MACHINE\SOFTWARE\Classes
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\FileExts
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Kindmap
// HKEY_CURRENT_USER\SOFTWARE\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppModel\Repository\Packages
// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppModel\Repository\Packages
// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\SystemFileAssociations
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Multimedia\WMPlayer\Extensions

// installed application lists
// i think ProgIDs is the most canonical list to enumerate when finding appx packages
// Packages seems to have multiple copies of each installed program in it
// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\PackagedCom\Package
// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppModel\PackageRepository\Packages
// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppModel\PackageRepository\Extensions\ProgIDs

// HKEY_CURRENT_USER\Software\Classes\AppXbhww3jf4qxawt50vjnfe7emd36rxsezh
struct RegKeyInfo {
    DWORD dwSubkeyCount;
    DWORD dwMaxSubkeyNameSize;

    DWORD dwValueCount;
    DWORD dwMaxValueNameSize;
    DWORD dwMaxValueDataSize;
};

static RegKeyInfo queryKeyInfo(HKEY hKey) {
    RegKeyInfo info{};
    LSTATUS status = RegQueryInfoKeyA(
        hKey, nullptr, nullptr, nullptr,
        &info.dwSubkeyCount, &info.dwMaxSubkeyNameSize,
        nullptr,
        &info.dwValueCount, &info.dwMaxValueNameSize, &info.dwMaxValueDataSize,
        nullptr, nullptr
    );

    if (status != STATUS_SUCCESS) {
        throw OsException{os_error_t(status), "RegQueryInfoKeyA"};
    }

    return info;
}

static std::experimental::generator<std::string_view> getSubkeyNames(HKEY hKey, DWORD dwMaxNameSize) {
    std::string subkeyNameString(dwMaxNameSize + 1, '\0');
    DWORD dwCounter = 0;

    while (true) {
        DWORD dwNameSize = subkeyNameString.size();
        DWORD dwIndex = dwCounter++;
        LSTATUS status = RegEnumKeyExA(
            hKey, dwIndex, subkeyNameString.data(), &dwNameSize,
            nullptr, nullptr, nullptr, nullptr
        );

        if (status == ERROR_NO_MORE_ITEMS)
            break;

        if (status != ERROR_SUCCESS)
            throw OsException{os_error_t(status), "RegEnumKeyExA + {}", dwIndex};

        co_yield std::string_view(subkeyNameString.data(), dwNameSize);
    }
}

static std::experimental::generator<std::string_view> getValueNames(HKEY hKey, DWORD dwMaxValueNameSize) {
    std::string valueNameString(dwMaxValueNameSize + 1, '\0');
    DWORD dwCounter = 0;

    while (true) {
        DWORD dwNameSize = valueNameString.size();
        DWORD dwIndex = dwCounter++;
        LSTATUS status = RegEnumValueA(
            hKey, dwIndex, valueNameString.data(), &dwNameSize,
            nullptr, nullptr, nullptr, nullptr
        );

        if (status == ERROR_NO_MORE_ITEMS)
            break;

        if (status != ERROR_SUCCESS)
            throw OsException{os_error_t(status), "RegEnumValueA + {}", dwIndex};

        co_yield std::string_view(valueNameString.data(), dwNameSize);
    }
}

static std::string regValueTypeName(DWORD dwType) {
    switch (dwType) {
    case REG_NONE: return "REG_NONE";
    case REG_SZ: return "REG_SZ";
    case REG_EXPAND_SZ: return "REG_EXPAND_SZ";
    case REG_BINARY: return "REG_BINARY";
    case REG_DWORD: return "REG_DWORD";
    case REG_MULTI_SZ: return "REG_MULTI_SZ";
    case REG_QWORD: return "REG_QWORD";
    default: return fmt::format("UNKNOWN {}", dwType);
    }
}

struct RegKeyValue {
    std::vector<BYTE> data;
    DWORD dwType = REG_NONE;

    bool isString() const noexcept {
        return dwType == REG_SZ || dwType == REG_EXPAND_SZ;
    }

    std::string_view string() const {
        CTASSERTF(isString(), "Interpreting %s as REG_SZ is invalid", regValueTypeName(dwType).c_str());

        const char *begin = reinterpret_cast<const char *>(data.data());
        const char *end = begin + data.size();

        std::string_view result{begin, end};

        while (result.ends_with('\0')) {
            result.remove_suffix(1);
        }

        while (result.starts_with('\0')) {
            result.remove_prefix(1);
        }

        return result;
    }
};

static std::optional<RegKeyValue> readRegValue(HKEY hKey, const char *name, DWORD dwMaxValueSize) {
    DWORD dwDataSize = dwMaxValueSize + 1;
    std::vector<BYTE> data(dwDataSize);
    DWORD dwType = REG_NONE;

    LSTATUS status = RegQueryValueExA(hKey, name, nullptr, &dwType, data.data(), &dwDataSize);

    switch (status) {
    case ERROR_FILE_NOT_FOUND:
        return std::nullopt;
    case ERROR_SUCCESS:
        data.resize(dwDataSize);
        return RegKeyValue { data, dwType };
    default:
        break;
    }

    std::string_view id = std::string_view{name ?: "(Default)"};
    throw OsException{os_error_t(status), "RegQueryValueExA {}", id};
}

static std::string readRegString(HKEY hKey, const char *name, DWORD dwMaxValueSize) {
    RegKeyValue value = readRegValue(hKey, name, dwMaxValueSize).value_or(RegKeyValue{});
    if (!value.isString())
        return "";

    return std::string{value.string()};
}

static std::vector<std::string> getOpenWithProgids(HKEY hKey, DWORD dwMaxValueNameSize, DWORD dwMaxValueDataSize) {
    std::vector<std::string> openWithProgids;

    std::optional<RegKeyValue> defaultValue = readRegValue(hKey, nullptr, dwMaxValueDataSize);
    if (defaultValue.has_value() && defaultValue->isString()) {
        std::string str{defaultValue->string()};
        openWithProgids.emplace_back(std::move(str));
    }

    for (std::string_view name : getValueNames(hKey, dwMaxValueNameSize)) {
        // work around .ppkg files
        // they have a weird setup where the default value contains the actual data
        // nothing else does this, its apparently part of the DISM provisioning package
        // format.
        if (name.empty())
            continue;

        std::string valueName{name};
        if (auto value = readRegValue(hKey, valueName.c_str(), dwMaxValueDataSize)) {
            if (!value->isString())
                continue;

            openWithProgids.emplace_back(std::string{name});
        }
    }

    return openWithProgids;
}

static std::map<std::string, std::string> getShellExCommands(HKEY hKey) {
    RegKeyInfo info = queryKeyInfo(hKey);
    std::map<std::string, std::string> commands;
    for (std::string_view name : getSubkeyNames(hKey, info.dwMaxSubkeyNameSize)) {
        wil::unique_hkey subkey = system::openRegistryKey(hKey, std::string{name});
        RegKeyInfo subkeyInfo = queryKeyInfo(subkey.get());
        std::string command = readRegString(subkey.get(), nullptr, subkeyInfo.dwMaxValueDataSize);
        commands.emplace(std::string{name}, std::move(command));
    }

    return commands;
}

static std::optional<cm::OpenWithProgramId> findOpenWithProgramId(HKEY hKey, const std::string& name) {
    if (wil::unique_hkey subkey = system::tryOpenRegistryKey(hKey, name.c_str())) {
        RegKeyInfo info = queryKeyInfo(subkey.get());
        std::string command = readRegString(subkey.get(), nullptr, info.dwMaxValueDataSize);
        return cm::OpenWithProgramId { name, command };
    }

    return std::nullopt;
}

// HKEY_CLASSES_ROOT
// Subkey: CompressedFolder
// Subkey: LibreOffice.CalcDocument.1
// Subkey: LibreOffice.DrawDocument.1
// Subkey: LibreOffice.ImpressDocument.1
// Subkey: LibreOffice.WriterDocument.1
// Subkey: OpenWithList
// Subkey: OpenWithProgIDs
// Subkey: OpenWithProgIds
// Subkey: OpenWithProgids
// Subkey: PersistentHandler
// Subkey: ShellEx
// Subkey: ShellNew
// Subkey: VMware.Document
// Subkey: VMware.OVAPackage
// Subkey: VMware.OVFPackage
// Subkey: VMware.Snapshot
// Subkey: VMware.SnapshotMetadata
// Subkey: VMware.SuspendState
// Subkey: VMware.TeamConfiguration
// Subkey: VMware.VMBios
// Subkey: VMware.VMPolicy
// Subkey: VMware.VMTeamMember
// Subkey: VMware.VirtualDisk
// Subkey: licfile
// Subkey: shell
// Subkey: shellex
// HKEY_CURRENT_USER\Software\Classes
// Subkey: OpenWithProgids
// Subkey: licfile
// Subkey: shell
// Subkey: shellex
static std::vector<cm::MimeType> readMimeDatabase(HKEY hKey) {
    wil::unique_hkey database = system::openRegistryKey(hKey, "MIME\\Database\\Content Type");
    RegKeyInfo info = queryKeyInfo(database.get());

    std::vector<cm::MimeType> mimetypes;
    for (std::string_view subkeyName : getSubkeyNames(database.get(), info.dwMaxSubkeyNameSize)) {
        std::string name{subkeyName};

        wil::unique_hkey contentType = system::openRegistryKey(database.get(), name);
        RegKeyInfo ctInfo = queryKeyInfo(contentType.get());

        std::string clsid = readRegString(contentType.get(), "CLSID", ctInfo.dwMaxValueDataSize);
        std::string extension = readRegString(contentType.get(), "Extension", ctInfo.dwMaxValueDataSize);
        std::string autoplay = readRegString(contentType.get(), "AutoplayContentTypeHandler", ctInfo.dwMaxValueDataSize);

        mimetypes.emplace_back(cm::MimeType {
            .contentType = name,
            .extension = extension,
            .clsid = clsid,
            .autoplay = autoplay,
        });
    }

    return mimetypes;
}

static std::vector<cm::ClassId> readClsIds(HKEY hKey) {
    wil::unique_hkey clsids = system::openRegistryKey(hKey, "CLSID");
    RegKeyInfo info = queryKeyInfo(clsids.get());

    std::vector<cm::ClassId> result;
    for (std::string_view subkeyName : getSubkeyNames(clsids.get(), info.dwMaxSubkeyNameSize)) {
        std::string clsid{subkeyName};

        wil::unique_hkey clsidKey = system::openRegistryKey(clsids.get(), clsid);
        RegKeyInfo clInfo = queryKeyInfo(clsidKey.get());
        std::string name = readRegString(clsidKey.get(), nullptr, clInfo.dwMaxValueDataSize);

        result.emplace_back(cm::ClassId {
            .clsid = clsid,
            .name = name
        });
    }

    return result;
}

struct RegistryClassReader {
    std::map<std::string, cm::OpenWithProgramId> programIds;

    cm::FileClass getFileClass(HKEY hKey, const std::string& name) {
        wil::unique_hkey subkey = system::openRegistryKey(hKey, name);
        RegKeyInfo subkeyInfo = queryKeyInfo(subkey.get());

        std::vector<std::string> openWithProgIds = [&] {
            if (wil::unique_hkey openWithProgIds = system::tryOpenRegistryKey(subkey.get(), "OpenWithProgids")) {
                RegKeyInfo openWithProgIdsInfo = queryKeyInfo(openWithProgIds.get());
                return getOpenWithProgids(openWithProgIds.get(), openWithProgIdsInfo.dwMaxValueNameSize, openWithProgIdsInfo.dwMaxValueDataSize);
            }

            return std::vector<std::string>{};
        }();

        for (const auto& progid : openWithProgIds) {
            if (progid.empty()) {
                fmt::println(stderr, "Empty ProgId found in openWithProgIds {}", name);
            }
        }

        std::map<std::string, std::string> shellExCommands = [&] {
            if (wil::unique_hkey shellEx = system::tryOpenRegistryKey(subkey.get(), "shellex")) {
                return getShellExCommands(shellEx.get());
            }

            return std::map<std::string, std::string>{};
        }();

        std::string persistentHandler = [&] {
            if (wil::unique_hkey persistentHandler = system::tryOpenRegistryKey(subkey.get(), "PersistentHandler")) {
                RegKeyInfo phInfo = queryKeyInfo(persistentHandler.get());
                return readRegString(persistentHandler.get(), nullptr, phInfo.dwMaxValueDataSize);
            }

            return std::string{};
        }();

        for (const auto& progid : openWithProgIds) {
            if (programIds.contains(progid))
                continue;

            if (std::optional<cm::OpenWithProgramId> prog = findOpenWithProgramId(hKey, progid)) {
                programIds.emplace(progid, prog.value());
            }
        }

        return cm::FileClass {
            .extension = name,
            .name = readRegString(subkey.get(), nullptr, subkeyInfo.dwMaxValueDataSize),
            .contentType = readRegString(subkey.get(), "Content Type", subkeyInfo.dwMaxValueDataSize),
            .perceivedType = readRegString(subkey.get(), "PerceivedType", subkeyInfo.dwMaxValueDataSize),
            .persistentHandler = persistentHandler,
            .openWithProgIds = std::move(openWithProgIds),
            .shellExCommands = std::move(shellExCommands),
        };
    }

    std::vector<cm::FileClass> getAllExtensionClasses(HKEY hKey) {
        RegKeyInfo info = queryKeyInfo(hKey);
        std::vector<cm::FileClass> classes;
        for (std::string_view subkeyName : getSubkeyNames(hKey, info.dwMaxSubkeyNameSize)) {
            if (!subkeyName.starts_with("."))
                continue;

            classes.emplace_back(getFileClass(hKey, std::string{subkeyName}));
        }

        return classes;
    }

    std::vector<cm::OpenWithProgramId> getPrograms() {
        std::vector<cm::OpenWithProgramId> result;
        result.reserve(programIds.size());
        for (auto& [_, progid] : programIds) {
            result.push_back(progid);
        }

        return result;
    }
};

static cm::RegistryClasses getAllClasses(HKEY hKey) {
    RegistryClassReader reader;

    std::vector<cm::FileClass> classes = reader.getAllExtensionClasses(hKey);
    cm::FileClass global = reader.getFileClass(hKey, "*");
    cm::FileClass directory = reader.getFileClass(hKey, "Directory");
    std::vector<cm::OpenWithProgramId> programs = reader.getPrograms();
    for (const auto& prog : programs) {
        fmt::println(stderr, "ProgId: {} => {}", prog.progid, prog.name);
    }

    return cm::RegistryClasses {
        .global = global,
        .directory = directory,
        .mimetypes = readMimeDatabase(hKey),
        .classes = classes,
        .clsids = readClsIds(hKey),
        .programs = programs,
    };
}

cm::ContextMenuRegistry::ContextMenuRegistry() {
    wil::unique_hkey hCurrentUserClasses = system::openRegistryKey(HKEY_CURRENT_USER, "Software\\Classes");
    global = getAllClasses(HKEY_CLASSES_ROOT);
    user = getAllClasses(hCurrentUserClasses.get());
}

static std::optional<std::string> optIfEmpty(const std::string& str) {
    return str.empty() ? std::nullopt : std::make_optional(str);
}

struct RegistryClassSerializer {
    db::Connection& db;
    db::PreparedInsertReturning<cmdao::FileClass> addFileClass;
    db::PreparedInsertReturning<cmdao::ClsId> addClsId;
    db::PreparedInsertReturning<cmdao::MimeType> addMimeType;
    db::PreparedInsertReturning<cmdao::OpenWithProgids> addProgId;

    RegistryClassSerializer(db::Connection& connection)
        : db(connection)
        , addFileClass(db.prepareInsertReturningPrimaryKey<cmdao::FileClass>())
        , addClsId(db.prepareInsertReturningPrimaryKey<cmdao::ClsId>())
        , addMimeType(db.prepareInsertReturningPrimaryKey<cmdao::MimeType>())
        , addProgId(db.prepareInsertReturningPrimaryKey<cmdao::OpenWithProgids>())
    { }

    void saveFileClass(uint64_t hiveId, const cm::FileClass& cls) {
        addFileClass.insert({
            .hiveId            = hiveId,
            .extension         = cls.extension,
            .name              = optIfEmpty(cls.name),
            .contentType       = optIfEmpty(cls.contentType),
            .perceivedType     = optIfEmpty(cls.perceivedType),
            .persistentHandler = optIfEmpty(cls.persistentHandler),
        });
    }

    void saveClassIds(uint64_t hiveId, const cm::ClassId& classId) {
        addClsId.insert({
            .hiveId = hiveId,
            .clsid = classId.clsid,
            .name = optIfEmpty(classId.name),
        });
    }

    void saveMimeType(uint64_t hiveId, const cm::MimeType& mimeType) {
        addMimeType.insert({
            .hiveId = hiveId,
            .contentType = mimeType.contentType,
            .extension = optIfEmpty(mimeType.extension),
            .clsid = optIfEmpty(mimeType.clsid),
            .autoplay = optIfEmpty(mimeType.autoplay),
        });
    }

    void saveRegistryClasses(uint64_t hiveId, const cm::RegistryClasses& classes) {
        db::Transaction tx(&db);

        for (const cm::FileClass& cls : classes.classes) {
            saveFileClass(hiveId, cls);
        }

        saveFileClass(hiveId, classes.global);
        saveFileClass(hiveId, classes.directory);

        for (const cm::ClassId& classId : classes.clsids) {
            saveClassIds(hiveId, classId);
        }

        for (const cm::MimeType& mimeType : classes.mimetypes) {
            saveMimeType(hiveId, mimeType);
        }

        for (const cm::OpenWithProgramId& program : classes.programs) {
            addProgId.insert({
                .progid = program.progid,
                .name = optIfEmpty(program.name),
            });
        }
    }
};

void cm::ContextMenuRegistry::save(db::Connection& db) {
    db.createTable(cmdao::RegistryHive::table());
    db.createTable(cmdao::FileClass::table());
    db.createTable(cmdao::OpenWithProgids::table());
    db.createTable(cmdao::ClsId::table());
    db.createTable(cmdao::MimeType::table());

    uint64_t hkcu = db.insertReturningPrimaryKey(cmdao::RegistryHive{ .name = "HKEY_CURRENT_USER\\Software\\Classes" });
    uint64_t hkcr = db.insertReturningPrimaryKey(cmdao::RegistryHive{ .name = "HKEY_CLASSES_ROOT" });

    RegistryClassSerializer serializer{db};

    serializer.saveRegistryClasses(hkcr, global);
    serializer.saveRegistryClasses(hkcu, user);
}
