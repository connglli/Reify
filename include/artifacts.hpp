// MIT License
//
// Copyright (c) 2025
//
// Kavya Chopra (chopra.kavya04@gmail.com)
// Cong Li (cong.li@inf.ethz.ch)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef REIFY_ARTIFACTS_HPP
#define REIFY_ARTIFACTS_HPP

#include <filesystem>
#include "lib/dbgutils.hpp"

#define FUNCTION_NAME_PREFIX "func"
#define PROGRAM_NAME_PREFIX "prog"
#define JAVA_CLASS_NAME_PREFIX "Class"

#define FILENAME_FUNCTION_C "func.c"
#define FILENAME_MAIN_C "main.c"
#define FILENAME_MAPPING_JSONL "inout.jsonl"
#define FILENAME_VARSTATE_JSONL "varstate.jsonl"
#define FILENAME_SEXPRESSION "func.sexp"
#define FILENAME_FUNC_LOGGING "func.log"
#define FILENAME_PROG_LOGGING "prog.log"
#define FILENAME_CHECKSUM_C "chksum.c"
#define FILENAME_PROTOTYPES_H "proto.h"

namespace fs = std::filesystem;

/////////////////////////////////////////////////////
/// Leaf Function Artifacts
/////////////////////////////////////////////////////

struct FunArts {
  std::string uuid;
  std::string sno;
  fs::path output;

  FunArts(const std::string &uuid, const std::string &sno, const std::string &output) :
      uuid(uuid), sno(sno), output(output) {}

  FunArts(const fs::path &dir) {
    Assert(
        IsTestDir(dir), "The given directory \"%s\" is not a valid function artifact directory",
        dir.c_str()
    );
    std::string dirName = dir.filename().string();
    size_t firstUnderscore = dirName.find_first_of('_');
    size_t lastUnderscore = dirName.find_last_of('_');
    Assert(
        firstUnderscore != std::string::npos && lastUnderscore != std::string::npos &&
            firstUnderscore != lastUnderscore,
        "The function directory name \"%s\" is malformed", dirName.c_str()
    );
    uuid = dirName.substr(firstUnderscore + 1, lastUnderscore - firstUnderscore - 1);
    sno = dirName.substr(lastUnderscore + 1);
    output = dir.parent_path();
  }

  static bool IsTestDir(const fs::path &dir) {
    return fs::is_directory(dir) &&
           dir.filename().string().starts_with(std::string(FUNCTION_NAME_PREFIX) + "_") &&
           fs::exists(dir / FILENAME_FUNCTION_C) && fs::exists(dir / FILENAME_MAPPING_JSONL);
  }

  fs::path GetTestDir() const {
    return output / (std::string(FUNCTION_NAME_PREFIX) + "_" + uuid + "_" + sno);
  }

  std::string GetFunName() const {
    return std::string(FUNCTION_NAME_PREFIX) + "_" + uuid + "_" + sno;
  }

  std::string GetJavaClsName() const {
    return std::string(JAVA_CLASS_NAME_PREFIX) + "_" + uuid + "_" + sno;
  }

  fs::path GetFunPath() const { return GetTestDir() / FILENAME_FUNCTION_C; }

  fs::path GetSexpPath() const { return GetTestDir() / FILENAME_SEXPRESSION; }

  fs::path GetJavaClsPath() const { return GetTestDir() / (GetJavaClsName() + ".class"); }

  fs::path GetMapPath() const { return GetTestDir() / FILENAME_MAPPING_JSONL; }

  fs::path GetVarStatePath() const { return GetTestDir() / FILENAME_VARSTATE_JSONL; }

  fs::path GetMainPath() const { return GetTestDir() / FILENAME_MAIN_C; }

  fs::path GetLogPath(bool devnull = true) const {
    if (devnull) {
      return {"/dev/null"};
    } else {
      return GetTestDir() / FILENAME_FUNC_LOGGING;
    }
  }
};

/////////////////////////////////////////////////////
/// Whole Program Artifacts
/////////////////////////////////////////////////////

struct ProgArts {
  std::string uuid;
  std::string sno;
  fs::path output;

  ProgArts(const std::string &uuid, const std::string &sno, const std::string &output) :
      uuid(uuid), sno(sno), output(output) {}

  fs::path GetTestDir() const {
    return output / (std::string(PROGRAM_NAME_PREFIX) + "_" + uuid + "_" + sno);
  }

  static bool IsTestDir(const fs::path &dir) {
    return fs::is_directory(dir) &&
           dir.filename().string().starts_with(std::string(PROGRAM_NAME_PREFIX) + "_") &&
           fs::exists(dir / FILENAME_MAIN_C) && fs::exists(dir / FILENAME_PROTOTYPES_H) &&
           fs::exists(dir / FILENAME_CHECKSUM_C);
  }

  fs::path GetFunPath(const std::string &funName) const { return GetTestDir() / (funName + ".c"); }

  fs::path GetMainPath() const { return GetTestDir() / FILENAME_MAIN_C; }

  fs::path GetChksumPath() const { return GetTestDir() / FILENAME_CHECKSUM_C; }

  fs::path GetProtoPath() const { return GetTestDir() / FILENAME_PROTOTYPES_H; }

  fs::path GetLogPath(bool devnull = true) const {
    if (devnull) {
      return {"/dev/null"};
    } else {
      return this->GetTestDir() / FILENAME_PROG_LOGGING;
    }
  }
};

#endif // REIFY_ARTIFACTS_HPP
