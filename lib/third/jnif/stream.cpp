//
// Created by Luigi on 07.10.17.
//

#include <fstream>
#include <iostream>
#include "jnif/jnif.hpp"

namespace jnif {

  namespace stream {

    IClassFileStream::IClassFileStream(const char *fileName) {
      std::ifstream ifs(fileName, std::ios::in | std::ios::binary | std::ios::ate);

      if (!ifs.is_open()) {
        int m;
        ifs >> m;
        throw "File not opened!";
      }

      ifs.seekg(0, std::ios::end);
      u4 fileSize = ifs.tellg();
      u1 *buffer = new u1[fileSize];

      ifs.seekg(0, std::ios::beg);
      if (!ifs.read((char *) buffer, fileSize)) {
        throw "File not opened!";
      }

      parser::ClassFileParser::parse(buffer, fileSize, this);

      delete[] buffer;
    }

    OClassFileStream::OClassFileStream(const char *fileName, ClassFile *classFile) {
      classFile->computeFrames(nullptr);
      std::cout << "File size: " << classFile->computeSize() << std::endl;
      std::cout << "Code size: " << classFile->getMethod("f0")->codeAttr()->codeLen << std::endl;
      std::cout << "Max stack: " << classFile->getMethod("f0")->codeAttr()->maxStack << std::endl;
      std::cout << "Max locals: " << classFile->getMethod("f0")->codeAttr()->maxLocals << std::endl;
      classFile->write(std::string(fileName));
    }
  } // namespace stream
} // namespace jnif
