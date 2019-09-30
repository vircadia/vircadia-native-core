#include "Unzipper.h"

#include <QDir>
#include <QDebug>
#include <miniz/miniz.h>

#ifdef Q_OS_MACOS
#include <unistd.h>
#include <sys/stat.h>
#endif

Unzipper::Unzipper(const QString& zipFilePath, const QDir& outputDirectory) :
    _zipFilePath(zipFilePath), _outputDirectory(outputDirectory) {
}

void Unzipper::run() {
    qDebug() << "Reading zip file" << _zipFilePath << ", extracting to" << _outputDirectory.absolutePath();

    _outputDirectory.mkpath(_outputDirectory.absolutePath());

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    auto status = mz_zip_reader_init_file(&zip_archive, _zipFilePath.toUtf8().data(), 0);

    if (!status) {
        auto zip_error = mz_zip_get_last_error(&zip_archive);
        auto zip_error_msg = mz_zip_get_error_string(zip_error);
        emit finished(true, "Failed to initialize miniz: " + QString::number(zip_error) + " " + zip_error_msg);
        return;
    }

    int fileCount = (int)mz_zip_reader_get_num_files(&zip_archive);

    qDebug() << "Zip archive has a file count of " << fileCount;

    if (fileCount == 0) {
        mz_zip_reader_end(&zip_archive);
        emit finished(false, "");
        return;
    }

    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, 0, &file_stat)) {
        mz_zip_reader_end(&zip_archive);
        emit finished(true, "Zip archive cannot be stat'd");
        return;
    }

    uint64_t totalSize = 0;
    uint64_t totalCompressedSize = 0;
    //bool _shouldFail = false;
    for (int i = 0; i < fileCount; i++) {
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;

        QString filename = file_stat.m_filename;
        QString fullFilename = _outputDirectory.filePath(filename);
        if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            if (!_outputDirectory.mkpath(fullFilename)) {
                mz_zip_reader_end(&zip_archive);
                emit finished(true, "Unzipping error while creating folder: " + fullFilename);
                return;
            }
            continue;
        }

        constexpr uint16_t FILE_PERMISSIONS_MASK { 0777 };

        uint16_t mod_attr = (file_stat.m_external_attr >> 16) & FILE_PERMISSIONS_MASK;
        uint16_t filetype_attr = (file_stat.m_external_attr >> 16) & S_IFMT;
#ifdef Q_OS_MACOS
        bool is_symlink = filetype_attr == S_IFLNK;
#else
        bool is_symlink = false;
#endif

        if (is_symlink) {
#ifdef Q_OS_MACOS
            size_t size;
            auto data = mz_zip_reader_extract_to_heap(&zip_archive, i, &size, 0);
            auto target = QString::fromUtf8((char*)data, size);

            qDebug() << "Extracted symlink: " << size << target;

            symlink(target.toUtf8().data(), fullFilename.toUtf8().data());
#else
            emit finished(true, "Error unzipping symlink");
            return;
#endif
        } else if (mz_zip_reader_extract_to_file(&zip_archive, i, fullFilename.toUtf8().data(), 0)) {
            totalCompressedSize += (uint64_t)file_stat.m_comp_size;
            totalSize += (uint64_t)file_stat.m_uncomp_size;
            emit progress((float)totalCompressedSize / (float)zip_archive.m_archive_size);
        } else {
            emit finished(true, "Unzipping error unzipping file: " + fullFilename);
            return;
        }
#ifdef Q_OS_MACOS
        chmod(fullFilename.toUtf8().data(), mod_attr);
#endif
    }

    qDebug() << "Done unzipping archive, total size:" << totalSize;

    mz_zip_reader_end(&zip_archive);

    emit finished(false, "");
}
