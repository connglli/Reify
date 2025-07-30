//
// Created by Luigi on 10.10.17.
//

#include "jnif/jnif.hpp"
#include "zip/unzip.h"

namespace jnif {

  namespace jar {

    JarFile::JarFile(const char *zipPath) {
      _uf = unzOpen64(zipPath);
      if (_uf == nullptr) {
        throw JarException("Can't open file");
      }
    }

    JarFile::~JarFile() { unzClose(_uf); }

    int JarFile::forEach(void *args, int jarid, ZipCallback callback) {
      unz_global_info64 gi;

      int err = unzGetGlobalInfo64(_uf, &gi);
      if (err != UNZ_OK) {
        fprintf(stderr, "error %d with zipfile in unzGetGlobalInfo \n", err);
        return -1;
      }

      for (uLong i = 0; i < gi.number_entry; i++) {
        if (_extractCurrentFile(args, jarid, callback) != UNZ_OK) {
          break;
        }

        if (i + 1 < gi.number_entry) {
          err = unzGoToNextFile(_uf);
          if (err != UNZ_OK) {
            fprintf(stderr, "error %d with zipfile in unzGoToNextFile\n", err);
            break;
          }
        }
      }

      return gi.number_entry;
    }

    int JarFile::_extractCurrentFile(void *args, int jarid, ZipCallback callback) {
      char filename_inzip[256];
      int err = UNZ_OK;

      unz_file_info64 file_info;
      err = unzGetCurrentFileInfo64(
          _uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0
      );
      int size = file_info.uncompressed_size;

      if (err != UNZ_OK) {
        fprintf(stderr, "error %d with zipfile in unzGetCurrentFileInfo\n", err);
        return err;
      }

      auto isSuffix = [](const std::string &suffix, const std::string &text) {
        auto res = std::mismatch(suffix.rbegin(), suffix.rend(), text.rbegin());
        return res.first == suffix.rend();
      };

      if (!isSuffix(".class", std::string(filename_inzip))) {
        return 0;
      }

      void *buf = malloc(size);
      if (buf == nullptr) {
        fprintf(stderr, "PANIC: Error allocating memory\n");
        exit(1);
      }

      err = unzOpenCurrentFilePassword(_uf, nullptr);
      if (err != UNZ_OK) {
        printf("error %d with zipfile in unzOpenCurrentFilePassword\n", err);
        // return UNZ_INTERNALERROR;
      }

      err = unzReadCurrentFile(_uf, buf, size);
      if (err < 0) {
        printf("error %d with zipfile in unzReadCurrentFile\n", err);
      } else if (err == size) {
        err = UNZ_OK;
        callback(args, jarid, buf, size, filename_inzip);
      } else {
        printf("error while reading: %d", err);
      }

      if (err == UNZ_OK) {
        err = unzCloseCurrentFile(_uf);
        if (err != UNZ_OK) {
          printf("error %d with zipfile in unzCloseCurrentFile\n", err);
        }
      } else
        unzCloseCurrentFile(_uf);

      free(buf);
      return err;
    }
  } // namespace jar
} // namespace jnif
