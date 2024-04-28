#include "core/map.hpp"
#include "core/vector.hpp"
#include "stdafx.hpp"

#include "archive/fs.hpp"

#include "logs/logs.hpp"

using namespace sm;

struct DiskFileSystem final : sm::IFileSystem {
    fs::path root;

    sm::HashMap<fs::path, sm::VectorBase<byte>> cache;

    sm::View<byte> readFileData(const fs::path& path) override {
        auto it = cache.find(path);
        if (it != cache.end())
            return it->second;

        std::ifstream file(root / path, std::ios::binary);
        if (!file)
            return {};

        file.seekg(0, std::ios::end);
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        sm::VectorBase<byte> data(size);
        file.read(reinterpret_cast<char *>(data.data()), size);

        auto [result, _] = cache.emplace(path, std::move(data));

        return result->second;
    }

    DiskFileSystem(fs::path root)
        : root(std::move(root))
    { }
};

IFileSystem *sm::mountFileSystem(fs::path path) {
    return new DiskFileSystem(std::move(path));
}

struct ArchiveFileSystem final : sm::IFileSystem {
    struct archive *archive;

    sm::HashMap<fs::path, sm::VectorBase<byte>> cache;

    sm::View<byte> readFileData(const fs::path& path) override {
        auto it = cache.find(path);
        if (it == cache.end())
            return {};

        return it->second;
    }

    void init() {
        struct archive_entry *entry;
        while (true) {
            int r = archive_read_next_header(archive, &entry);
            if (r == ARCHIVE_EOF)
                break;

            if (r != ARCHIVE_OK) {
                logs::gAssets.error("failed to read archive: {}", archive_error_string(archive));
                continue;
            }

            fs::path path(archive_entry_pathname(entry));
            if (archive_entry_filetype(entry) != AE_IFREG)
                continue;

            sm::VectorBase<byte> data(archive_entry_size(entry));
            archive_read_data(archive, data.data(), data.size());

            cache.emplace(path, std::move(data));

            archive_read_data_skip(archive);
        }
    }

    ArchiveFileSystem(const fs::path& path) {
        archive = archive_read_new();
        archive_read_support_filter_all(archive);
        archive_read_support_format_all(archive);
        int r = archive_read_open_filename(archive, path.string().c_str(), 10240);
        if (r != ARCHIVE_OK) {
            logs::gAssets.error("failed to open archive: {} ({})", path.string(), archive_error_string(archive));
            archive_read_free(archive);
            return;
        }

        init();
    }

    ~ArchiveFileSystem() {
        archive_read_free(archive);
    }
};

IFileSystem *sm::mountArchive(const fs::path& path) {
    return new ArchiveFileSystem(path);
}
