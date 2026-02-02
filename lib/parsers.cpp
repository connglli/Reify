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

#include <algorithm>

#include "lib/parsers.hpp"

// clang-format off
namespace symir {
  std::string SymSexpLexer::Token::GetKindName(Kind op) {
    // Names in this table maps one-on-one to the Kind enumeration
    static const char* kindNames[] = {
      // Parenthesis
      "TK_LPAREN",
      "TK_RPAREN",

      // Brackets
      "TK_LBRACKET",
      "TK_RBRACKET",

      // Keywords
#define XX(name, ...) "TK_KW_" #name,
      SYMIR_KEYWORD_LIST(XX)
#undef XX

      // Term operators
#define XX(name, ...) "TK_OP_TERM_" #name,
      SYMIR_TERMOP_LIST(XX)
#undef XX

      // Expr operators
#define XX(name, ...) "TK_OP_EXPR_" #name,
      SYMIR_EXPROP_LIST(XX)
#undef XX

      // Cond operators
#define XX(name, ...) "TK_OP_COND_" #name,
      SYMIR_CONDOP_LIST(XX)
#undef XX

      // Types
#define XX(name, ...) "TK_TYPE_" #name,
      SYMIR_TYPE_LIST(XX)
#undef XX

      // Coefficients and variables
      "TK_IDENT",

      // End of file/input
      "TK_EOF",

      // Unknown tokens
      "TK_UNKNOWN",
    };
    return kindNames[op];
  }

  bool SymSexpLexer::Token::IsOfKind(const std::string_view &token, Kind kind) {
    // A predicate type for kind checking
    using KindPred = std::function<bool(const std::string_view &)>;

    // Predicates in this table maps one-on-one to the Kind enumeration
    static const std::vector<KindPred> kindPreds = {
      // Parenthesis
      [](const std::string_view &str) { return str == "("; },
      [](const std::string_view &str) { return str == ")"; },

      // Brackets
      [](const std::string_view &str) { return str == "["; },
      [](const std::string_view &str) { return str == "]"; },

      // Keywords
  #define XX(name, sname) [](const std::string_view &str) { return str == #sname; },
      SYMIR_KEYWORD_LIST(XX)
  #undef XX

      // Term operators
  #define XX(name, capt, smal, ...) [](const std::string_view &str) { return str == #smal; },
      SYMIR_TERMOP_LIST(XX)
  #undef XX

      // Expr operators
  #define XX(name, capt, smal, ...) [](const std::string_view &str) { return str == "e" #smal; },
      SYMIR_EXPROP_LIST(XX)
  #undef XX

      // Cond operators
  #define XX(name, capt, smal, ...) [](const std::string_view &str) { return str == #smal; },
      SYMIR_CONDOP_LIST(XX)
  #undef XX

      // Types
  #define XX(name, cname, sname) [](const std::string_view &str) { return str == (sname); },
      SYMIR_TYPE_LIST(XX)
  #undef XX

      // Coefficients and variables
      // Since we do not support non-text operators, string, and comment, all others are identifiers
      // TODO: Support rather more complex identifiers and replace below checks with regexes
      [](const std::string_view &str) {
        return std::ranges::all_of(str, [](const char c) {
          return c == '-' || c == '+' || c == SymSexpLower::KW_COEF_VAL_SYM || c == '_' || std::isalnum(c);
        });
      },

      // End of file/input
      [](const std::string_view &str) { return false; },

      // Unknown tokens
      [](const std::string_view &str) { return true; }
    };
    return kindPreds[kind](token);
  }

  SymSexpLexer::Token::Kind SymSexpLexer::Token::ParseKind(const std::string_view& token) {
    // Since we do not support string and comments, our checkers are quite simple.
    for (auto i = 0; i < TK_UNKNOWN+1; i++) {
      Kind k = static_cast<Kind>(i);
      if (IsOfKind(token, k)) {
        return k;
      }
    }
    return TK_UNKNOWN;
  }
} // namespace symir

// clang-format on

namespace symir {
  SymSexpLexer::TokenStream SymSexpLexer::Lex() {
    return [this]() -> Token {
      // We eat all whitespace before the next token. This is save as we don't
      // support anything (comment, string, etc) that include whitespace as part of them.
      eatWhitespace();

      // End of input
      if (stream.eof()) {
        return {
            .kind = Token::TK_EOF,
            .stRow = curRow,
            .stCol = curCol,
            .edRow = curRow,
            .edCol = curCol,
            .str = ""
        };
      }

      // We record the start position
      const size_t stInd = curInd, stRow = curRow, stCol = curCol;

      // We lookahead to see where to go. We are a quite simple IR with all operators
      // being in the text form, so we don't have to lookahead many characters.
      // Otherwise, we'd build a deterministic finite automaton.
      char ch = lookahead(1)[0];
      if (ch == '(' || ch == ')') {
        eatChars(1);
      } else if (ch == '[' || ch == ']') {
        eatChars(1);
      } else {
        eatNonwhitespace(/*excl=*/[](char c) {
          return c == '(' || c == ')' || c == '[' || c == ']';
        });
      }

      // We record the ending position and take the token
      const size_t edInd = curInd, edRow = curRow, edCol = curCol;
      const std::string_view token = srcView.substr(stInd, edInd - stInd);

      return {
          .kind = Token::ParseKind(token),
          .stRow = stRow,
          .stCol = stCol,
          .edRow = edRow,
          .edCol = edCol,
          .str = token
      };
    };
  }

  std::vector<char> SymSexpLexer::lookahead(int num) {
    std::vector<char> chs;
    for (auto i = 0; !stream.eof() && i < num; i++) {
      chs.push_back(static_cast<char>(stream.get()));
    }
    for (size_t i = 0; i < chs.size(); i++) {
      stream.unget();
    }
    return chs;
  }

  void SymSexpLexer::eatWhitespace(CharMatcher incl) {
    eatCharsIf([&incl](const char ch) { return std::isspace(ch) || incl(ch); });
  }

  void SymSexpLexer::eatNonwhitespace(CharMatcher excl) {
    eatCharsIf([&excl](const char ch) { return !std::isspace(ch) && !excl(ch); });
  }

  void SymSexpLexer::eatChars(int num) {
    int i = 0;
    eatCharsIf([&i, num](const char _) { return (i++) < num; });
  }

  void SymSexpLexer::eatCharsIf(CharMatcher pred) {
    char ch;
    while (!stream.eof() && stream.get(ch)) {
      if (!pred(ch)) {
        stream.unget();
        break;
      }
      curInd++;
      if (ch == '\n') {
        curRow++;
        curCol = 0;
      } else {
        curCol++;
      }
    }
  }
} // namespace symir

namespace symir {
  void SymSexpParser::Parse() {
    if (funct != nullptr) {
      return;
    }

    const auto stream = lexer.Lex();
    SymSexpLexer::Token *lastToken = nullptr;
    while (true) {
      const auto *token = lastToken;
      if (token == nullptr) {
        token = new SymSexpLexer::Token(stream());
      } else {
        lastToken = nullptr;
      }
      Assert(token != nullptr, "No available token is obtained");
      Assert(lastToken == nullptr, "The last token is still non-null");
      const auto kind = token->kind;

      if (kind == SymSexpLexer::Token::Kind::TK_EOF) {
        delete token;
        break;
      }
      if (kind == SymSexpLexer::Token::Kind::TK_LPAREN) {
        auto opToken = stream();
        switch (opToken.kind) {
          case SymSexpLexer::Token::Kind::TK_LPAREN:
            // We should reprocess this token
            lastToken = new SymSexpLexer::Token(opToken);
            pushOp(opToken.kind);
            break;

          case SymSexpLexer::Token::Kind::TK_KW_FUN:
            prepFun(stream);
            pushOp(opToken.kind);
            break;

          case SymSexpLexer::Token::Kind::TK_KW_STRUCT:
            prepStruct(stream);
            break;

          case SymSexpLexer::Token::Kind::TK_KW_BBL:
            prepBbl(stream);
            pushOp(opToken.kind);
            break;

#define XX(name, ...) case SymSexpLexer::Token::Kind::TK_OP_EXPR_##name:
            SYMIR_EXPROP_LIST(XX)
#undef XX
            pushArg<int>(0); // We need to save the number of terms
            pushOp(opToken.kind);
            break;

          case SymSexpLexer::Token::Kind::TK_KW_VOL: {
            pushOp(opToken.kind);
            const auto nextOpToken = stream();
            Assert(
                nextOpToken.kind == SymSexpLexer::Token::Kind::TK_KW_PAR ||
                    nextOpToken.kind == SymSexpLexer::Token::Kind::TK_KW_LOC,
                "The 1st child (%s) of the volatile node is neither a parameter nor a local",
                nextOpToken.FullInfo().c_str()
            );
            pushOp(nextOpToken.kind);
            break;
          }

          default:
            pushOp(opToken.kind);
            break;
        }
      } else if (kind == SymSexpLexer::Token::Kind::TK_RPAREN) {
        const auto op = popOp();
        switch (op) {
          case SymSexpLexer::Token::Kind::TK_LPAREN:
            break;

#define XX(name, capt, ...)                                                                        \
  case SymSexpLexer::Token::Kind::TK_OP_TERM_##name:                                               \
    build##capt##Term();                                                                           \
    break;
            SYMIR_TERMOP_LIST(XX)
#undef XX

#define XX(name, capt, ...)                                                                        \
  case SymSexpLexer::Token::Kind::TK_OP_EXPR_##name:                                               \
    build##capt##Expr();                                                                           \
    break;
            SYMIR_EXPROP_LIST(XX)
#undef XX

#define XX(name, capt, ...)                                                                        \
  case SymSexpLexer::Token::Kind::TK_OP_COND_##name:                                               \
    build##capt##Cond();                                                                           \
    break;
            SYMIR_CONDOP_LIST(XX)
#undef XX

          case SymSexpLexer::Token::Kind::TK_KW_PAR:
            buildParam();
            break;

          case SymSexpLexer::Token::Kind::TK_KW_LOC:
            buildLocal();
            break;

          case SymSexpLexer::Token::Kind::TK_KW_RET:
            buildReturn();
            break;

          case SymSexpLexer::Token::Kind::TK_KW_ASS:
            buildAssign();
            break;

          case SymSexpLexer::Token::Kind::TK_KW_BRH:
            buildBranch();
            break;

          case SymSexpLexer::Token::Kind::TK_KW_GOTO:
            buildGoto();
            break;

          case SymSexpLexer::Token::Kind::TK_KW_BBL:
            buildBlock();
            break;

          case SymSexpLexer::Token::Kind::TK_KW_FUN:
            buildFunc();
            break;

          default:
            Panic("Cannot reach here");
        }
      } else if (kind == SymSexpLexer::Token::Kind::TK_LBRACKET) {
        // Bracket handling - determine context
        const auto closingOp = popOp();
        // For the new syntax Type[size], brackets come AFTER the type token
        // We need to handle them for both PAR and LOC, but also for array access in assignments
        if (closingOp == SymSexpLexer::Token::Kind::TK_KW_PAR ||
            closingOp == SymSexpLexer::Token::Kind::TK_KW_LOC) {
          // In the new syntax, brackets follow the type, so we should encounter them
          // AFTER reading the type token. The bracket contains the array dimension.
          const auto dimLenToken = stream();
          Assert(
              dimLenToken.kind == SymSexpLexer::Token::Kind::TK_IDENT,
              "The content (%s) of a bracket should be an identifier (dimension size or access "
              "index)",
              dimLenToken.FullInfo().c_str()
          );
          // Push dimension length onto operator stack
          pushOp(static_cast<SymSexpLexer::Token::Kind>(std::stoi(dimLenToken.ToStr())));
          pushOp(kind);
        } else {
          // Regular bracket for array access in other contexts
          pushOp(kind);
        }
        pushOp(closingOp);
      } else if (kind == SymSexpLexer::Token::Kind::TK_RBRACKET) {
        // DROP IT - right brackets are handled implicitly by the parser functions
      } else {
        pushArg<SymSexpLexer::Token>(*token);
      }
      delete token;
    }

    Assert(opStack.empty(), "We are finished but the opStack is not empty");
    Assert(argStack.empty(), "We are finished but the argStack is not empty");
    Assert(funBd == nullptr, "We are finished but the funBd is not null");
    Assert(bblBd == nullptr, "We are finished but the bblBd is not null");
  }

  void SymSexpParser::prepFun(const SymSexpLexer::TokenStream &stream) {
    const auto nameToken = stream();
    Assert(
        nameToken.kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the function node is not an identifier, while it "
        "should be the name of the function",
        nameToken.FullInfo().c_str()
    );
    const auto typeToken = stream();
    Assert(
        typeToken.kind == SymSexpLexer::Token::Kind::TK_TYPE_I32,
        "The 2nd child (%s) of the function node is not a type, while it "
        "should be the return type of the function",
        typeToken.FullInfo().c_str()
    );
    funBd = std::make_unique<FunctBuilder>(
        nameToken.ToStr(), SymIR::GetTypeFromSName(typeToken.ToStr())
    );
    // Add pending structs
    for (const auto &p: pendingStructs) {
      funBd->SymStruct(p.first, p.second);
    }
    pendingStructs.clear();
  }

  void SymSexpParser::prepStruct(const SymSexpLexer::TokenStream &stream) {
    const auto nameToken = stream();
    Assert(
        nameToken.kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the struct node is not an identifier", nameToken.FullInfo().c_str()
    );
    const auto fieldsStart = stream();
    Assert(
        fieldsStart.kind == SymSexpLexer::Token::Kind::TK_LPAREN, "Expected ( to start fields list"
    );
    std::vector<StructDef::Field> fields;
    while (true) {
      auto t = stream();
      if (t.kind == SymSexpLexer::Token::Kind::TK_RPAREN)
        break;
      Assert(t.kind == SymSexpLexer::Token::Kind::TK_LPAREN, "Expected ( for field definition");

      auto fName = stream();
      Assert(fName.kind == SymSexpLexer::Token::Kind::TK_IDENT, "Expected field name");

      // Parse type first (new syntax: fieldname type[dims]...)
      auto typeToken = stream();
      SymIR::Type type = SymIR::I32;
      std::string sName;
      if (typeToken.kind == SymSexpLexer::Token::Kind::TK_TYPE_I32) {
        type = SymIR::I32;
      } else if (typeToken.kind == SymSexpLexer::Token::Kind::TK_IDENT) {
        type = SymIR::STRUCT;
        sName = typeToken.ToStr();
      } else {
        Panic("Unknown field type");
      }

      // Now parse dimensions AFTER the type
      std::vector<int> shape;
      while (true) {
        auto t = stream();
        if (t.kind == SymSexpLexer::Token::Kind::TK_LBRACKET) {
          auto dimT = stream();
          Assert(dimT.kind == SymSexpLexer::Token::Kind::TK_IDENT, "Expected dimension size");
          shape.push_back(std::stoi(dimT.ToStr()));
          auto rb = stream();
          Assert(rb.kind == SymSexpLexer::Token::Kind::TK_RBRACKET, "Expected ]");
        } else {
          // Not a bracket, need to handle closing paren
          Assert(
              t.kind == SymSexpLexer::Token::Kind::TK_RPAREN, "Expected ) to end field definition"
          );
          break;
        }
      }

      SymIR::Type baseType = type;
      if (!shape.empty()) {
        type = SymIR::Type::ARRAY;
      }
      fields.push_back({fName.ToStr(), type, sName, shape, baseType});
      // Note: closing paren already consumed in the dimension parsing loop above
    }
    // Consume the closing parenthesis of the struct definition
    auto structClosing = stream();
    Assert(
        structClosing.kind == SymSexpLexer::Token::Kind::TK_RPAREN,
        "Expected ) to end struct definition"
    );

    pendingStructs.push_back({nameToken.ToStr(), fields});
  }

  void SymSexpParser::prepBbl(const SymSexpLexer::TokenStream &stream) {
    const auto labelToken = stream();
    Assert(
        labelToken.kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the basic block node is not an identifier, while it "
        "should be the label of the basic block",
        labelToken.FullInfo().c_str()
    );
    bblBd = funBd->OpenBlock(std::string(labelToken.ToStr()));
  }

  void SymSexpParser::buildTerm(Term::Op op) {
    std::vector<const SymSexpLexer::Token *> accessTokens;
    while (true) {
      const auto opKind = popOp();
      if (opKind != SymSexpLexer::Token::TK_LBRACKET) {
        pushOp(opKind); // Push it back as this is not a bracket
        break;
      }
      const auto *accToken = popArg<SymSexpLexer::Token>();
      Assert(
          accToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
          "The 3rd child (%s) of the term node is not a coefficient, while it should be the "
          "an access to a vector variable of the term",
          accToken->FullInfo().c_str()
      );
      accessTokens.insert(accessTokens.begin(), accToken);
    }
    SymSexpLexer::Token *varToken =
        op != Term::Op::OP_CST ? popArg<SymSexpLexer::Token>() : nullptr;
    Assert(
        varToken == nullptr || varToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 2nd child (%s) of the term node is not an identifier, while it should be the "
        "variable of the term",
        varToken->FullInfo().c_str()
    );
    const auto *coefToken = popArg<SymSexpLexer::Token>();
    Assert(
        coefToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the term node is not an identifier, while it should be the "
        "coefficient of the term",
        coefToken->FullInfo().c_str()
    );
    auto *numTerms = popArg<int>();
    (*numTerms)++;
    const auto varDef = varToken != nullptr ? funBd->FindVar(varToken->ToStr()) : nullptr;

    SymIR::Type termType = varDef != nullptr ? varDef->GetType() : SymIR::Type::I32;
    SymIR::Type termBaseType = varDef != nullptr ? varDef->GetBaseType() : SymIR::Type::I32;
    std::string currStruct =
        (varDef && (termType == SymIR::STRUCT || termBaseType == SymIR::STRUCT))
            ? varDef->GetStructName()
            : "";
    int remainingDims = (varDef && varDef->IsVector()) ? varDef->GetVecNumDims() : 0;

    std::vector<Coef *> vecAcc;
    for (int i = 0; i < static_cast<int>(accessTokens.size()); i++) {
      const auto *t = accessTokens[i];
      if (remainingDims > 0) {
        // Expect an index token like "#0".
        vecAcc.push_back(buildCoef(t, SymIR::Type::I32));
        remainingDims--;
        // After consuming all array dimensions, resolve to base type
        if (remainingDims == 0 && termType == SymIR::ARRAY) {
          termType = termBaseType;
        }
      } else if (t->str.starts_with("#")) {
        // Indexing token while not expecting an index: treat as indexing into an array/vector.
        remainingDims = 1;
        vecAcc.push_back(buildCoef(t, SymIR::Type::I32));
        remainingDims--;
        if (remainingDims == 0 && termType == SymIR::ARRAY) {
          termType = termBaseType;
        }
      } else if (termType == SymIR::STRUCT || termBaseType == SymIR::STRUCT) {
        // Expect a field token like "f0".
        const auto structDef = funBd->FindStruct(currStruct);
        Assert(structDef != nullptr, "Struct %s not found", currStruct.c_str());
        int idx = structDef->GetFieldIndex(t->ToStr());
        Assert(
            idx != -1, "Field %s not found in struct %s", t->ToStr().c_str(),
            structDef->GetName().c_str()
        );
        vecAcc.push_back(funBd->SymI32Const(idx));

        const auto &field = structDef->GetField(idx);
        termType = field.type;
        termBaseType = field.baseType;
        if (termType == SymIR::STRUCT)
          currStruct = field.structName;
        else if (termBaseType == SymIR::STRUCT)
          currStruct = field.structName;
        remainingDims = field.shape.size();
      } else {
        Panic(
            "Unexpected access to scalar %s (base %s), tok=%s, remainingDims=%d, currStruct=%s",
            SymIR::GetTypeSName(termType).c_str(), SymIR::GetTypeSName(termBaseType).c_str(),
            t->ToStr().c_str(), remainingDims, currStruct.c_str()
        );
      }
      delete t;
    }

    pushArg<SymIRBuilder::TermID>(bblBd->SymTerm(op, buildCoef(coefToken, termType), varDef, vecAcc)
    );
    pushArg<int>(*numTerms);
    if (varToken != nullptr) {
      delete varToken;
    }
    delete coefToken;
    delete numTerms;
  }

#define XX(name, capt, ...)                                                                        \
  void SymSexpParser::build##capt##Term() { buildTerm(Term::OP_##name); }
  SYMIR_TERMOP_LIST(XX)
#undef XX

  void SymSexpParser::buildExpr(Expr::Op op) {
    const auto *numTerms = popArg<int>();
    std::vector<SymIRBuilder::TermID> terms;
    for (auto i = 0; i < *numTerms; i++) {
      const auto *tid = popArg<SymIRBuilder::TermID>();
      terms.push_back(*tid);
      delete tid;
    }
    std::ranges::reverse(terms);
    pushArg<SymIRBuilder::ExprID>(bblBd->SymExpr(op, terms));
    delete numTerms;
  }

#define XX(name, capt, ...)                                                                        \
  void SymSexpParser::build##capt##Expr() { buildExpr(Expr::OP_##name); }
  SYMIR_EXPROP_LIST(XX)
#undef XX

  void SymSexpParser::buildCond(Cond::Op op) {
    const auto *eid = popArg<SymIRBuilder::ExprID>();
    pushArg<SymIRBuilder::CondID>(bblBd->SymCond(op, *eid));
    delete eid;
  }

#define XX(name, capt, ...)                                                                        \
  void SymSexpParser::build##capt##Cond() { buildCond(Cond::OP_##name); }
  SYMIR_CONDOP_LIST(XX)
#undef XX

  void SymSexpParser::buildParam() {
    // New syntax: (par varname Type[dim1][dim2]...)
    // Brackets are specially handled: when we see Type[10], the bracket handling code
    // pushes the size (10) and then TK_LBRACKET onto the op stack

    // Parse array dimensions that come after the type
    std::vector<int> vecShape;
    while (true) {
      const auto opKind = popOp();
      if (opKind != SymSexpLexer::Token::TK_LBRACKET) {
        pushOp(opKind); // Push it back as this is not a bracket
        break;
      }
      // The dimension size was pushed before the bracket
      const auto dimLen = static_cast<int>(popOp());
      Assert(dimLen > 0, "The dimension length (%d) should be a positive integer", dimLen);
      vecShape.insert(vecShape.begin(), dimLen);
    }

    // Parse the type token
    const auto *typeToken = popArg<SymSexpLexer::Token>();
    SymIR::Type type = SymIR::I32;
    std::string structName;
    if (typeToken->kind == SymSexpLexer::Token::Kind::TK_IDENT) {
      // Struct type
      type = SymIR::STRUCT;
      structName = typeToken->ToStr();
    } else {
      Assert(
          typeToken->kind == SymSexpLexer::Token::Kind::TK_TYPE_I32,
          "The type token (%s) of the param node should be a type (i32) or struct name",
          typeToken->FullInfo().c_str()
      );
      type = SymIR::GetTypeFromSName(typeToken->ToStr());
    }

    // Parse the variable name
    const auto *varToken = popArg<SymSexpLexer::Token>();
    Assert(
        varToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The variable name (%s) of the parameter should be an identifier",
        varToken->FullInfo().c_str()
    );

    // Check for volatile keyword
    bool isVolatile = false;
    if (const auto opKind = popOp(); opKind == SymSexpLexer::Token::TK_KW_VOL) {
      isVolatile = true;
    } else {
      pushOp(opKind); // Push it back as this is not a volatile parameter
    }

    // Create the appropriate parameter type
    if (!vecShape.empty()) {
      funBd->SymVecParam(
          varToken->ToStr(), vecShape, type, structName,
          /*isVolatile=*/isVolatile
      );
    } else if (type == SymIR::STRUCT) {
      funBd->SymStructParam(varToken->ToStr(), structName);
    } else {
      funBd->SymScaParam(varToken->ToStr(), type, /*isVolatile=*/isVolatile);
    }
    delete typeToken;
    delete varToken;
  }

  void SymSexpParser::buildLocal() {
    // Old syntax (kept for compatibility): (loc varname initvalues... Type[dim1][dim2])
    // Stack order when popping: Type (top), then inits, then dimensions markers, then varname
    // (bottom)

    // Parse array dimensions (come after type in stack order)
    std::vector<int> vecShape;
    while (true) {
      const auto opKind = popOp();
      if (opKind != SymSexpLexer::Token::TK_LBRACKET) {
        pushOp(opKind); // Push it back as this is not a bracket
        break;
      }
      // The dimension size was pushed before the bracket
      const auto dimLen = static_cast<int>(popOp());
      Assert(dimLen > 0, "The dimension length (%d) should be a positive integer", dimLen);
      vecShape.insert(vecShape.begin(), dimLen);
    }

    // Parse the type token (top of arg stack)
    const auto *typeToken = popArg<SymSexpLexer::Token>();
    SymIR::Type localType = SymIR::I32;
    std::string structName;
    if (typeToken->kind == SymSexpLexer::Token::Kind::TK_IDENT) {
      localType = SymIR::STRUCT;
      structName = typeToken->ToStr();
    } else {
      Assert(
          typeToken->kind == SymSexpLexer::Token::Kind::TK_TYPE_I32,
          "The type token (%s) of the local should be a type (i32) or struct name",
          typeToken->FullInfo().c_str()
      );
      localType = SymIR::GetTypeFromSName(typeToken->ToStr());
    }

    // Now collect initializer values (between type and varname)
    std::vector<Coef *> vecInits;

    // Build initializer list.
    // - Scalar locals: require exactly one init.
    // - Struct locals (non-vec): require full flattened init list.
    // - Vec locals with struct baseType: init list is optional and may be empty.
    // - Vec locals with scalar baseType: require one init per element.
    if (!vecShape.empty() && localType == SymIR::STRUCT) {
      // No mandatory initializer tokens here.
    } else {
      // Calculate total number of scalar initializers required.
      int scalarCount = 1;
      if (localType == SymIR::STRUCT) {
        std::function<int(const std::string &)> countStruct = [&](const std::string &sName) {
          const auto *st = funBd->FindStruct(sName);
          Assert(st != nullptr, "Struct %s not found", sName.c_str());
          int c = 0;
          for (const auto &f: st->GetFields()) {
            int fc = 1;
            for (int d: f.shape)
              fc *= d;
            if (f.type == SymIR::STRUCT) {
              fc *= countStruct(f.structName);
            } else if (f.baseType == SymIR::STRUCT) {
              // Array (or vector) of structs.
              fc *= countStruct(f.structName);
            }
            c += fc;
          }
          return c;
        };
        scalarCount = countStruct(structName);
      }

      int arraySize = 1;
      if (!vecShape.empty()) {
        arraySize = VarDef::GetVecNumEls(vecShape);
      }

      int totalInits = arraySize * scalarCount;

      // Collect init values from the arg stack (between type and varname)
      for (int i = 0; i < totalInits; i++) {
        const auto *coefToken = popArg<SymSexpLexer::Token>();
        Assert(
            coefToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
            "Expected initializer value, got %s", coefToken->FullInfo().c_str()
        );
        vecInits.insert(vecInits.begin(), buildCoef(coefToken, SymIR::I32));
        delete coefToken;
      }
    }

    // Parse the variable name (at bottom of arg stack)
    const auto *varToken = popArg<SymSexpLexer::Token>();
    Assert(
        varToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The variable name (%s) of the local should be an identifier", varToken->FullInfo().c_str()
    );

    // Check for volatile keyword
    bool isVolatile = false;
    if (const auto opKind = popOp(); opKind == SymSexpLexer::Token::TK_KW_VOL) {
      isVolatile = true;
    } else {
      pushOp(opKind); // Push it back as this is not a volatile local
    }

    // Create the appropriate local variable type
    if (vecShape.empty()) {
      if (localType == SymIR::STRUCT) {
        funBd->SymStructLocal(varToken->ToStr(), structName, vecInits);
      } else {
        funBd->SymScaLocal(varToken->ToStr(), vecInits.front(), localType, isVolatile);
      }
    } else {
      funBd->SymVecLocal(varToken->ToStr(), vecShape, vecInits, localType, structName, isVolatile);
    }
    delete typeToken;
    delete varToken;
  }

  void SymSexpParser::buildAssign() {
    const auto *eid = popArg<SymIRBuilder::ExprID>();
    std::vector<const SymSexpLexer::Token *> accessTokens;
    while (true) {
      const auto opKind = popOp();
      if (opKind != SymSexpLexer::Token::TK_LBRACKET) {
        pushOp(opKind); // Push it back as this is not a bracket
        break;
      }
      const auto *accToken = popArg<SymSexpLexer::Token>();
      Assert(
          accToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
          "The 2nd child (%s) of the assignment is not a coefficient", accToken->FullInfo().c_str()
      );
      accessTokens.insert(accessTokens.begin(), accToken);
    }
    const auto *varToken = popArg<SymSexpLexer::Token>();
    Assert(
        varToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the assignment is not an identifier, while it should be the "
        "variable of the assignment",
        varToken->FullInfo().c_str()
    );

    const auto varDef = funBd->FindVar(varToken->ToStr());

    SymIR::Type termType = varDef != nullptr ? varDef->GetType() : SymIR::Type::I32;
    SymIR::Type termBaseType = varDef != nullptr ? varDef->GetBaseType() : SymIR::Type::I32;
    std::string currStruct =
        (varDef && (termType == SymIR::STRUCT || termBaseType == SymIR::STRUCT))
            ? varDef->GetStructName()
            : "";
    int remainingDims = (varDef && varDef->IsVector()) ? varDef->GetVecNumDims() : 0;

    std::vector<Coef *> vecAcc;
    for (int i = 0; i < static_cast<int>(accessTokens.size()); i++) {
      const auto *t = accessTokens[i];
      if (remainingDims > 0) {
        vecAcc.push_back(buildCoef(t, SymIR::Type::I32));
        remainingDims--;
        // After consuming all array dimensions, resolve to base type
        if (remainingDims == 0 && termType == SymIR::ARRAY) {
          termType = termBaseType;
        }
      } else if (t->str.starts_with("#")) {
        remainingDims = 1;
        vecAcc.push_back(buildCoef(t, SymIR::Type::I32));
        remainingDims--;
        if (remainingDims == 0 && termType == SymIR::ARRAY) {
          termType = termBaseType;
        }
      } else if (termType == SymIR::STRUCT || termBaseType == SymIR::STRUCT) {
        const auto structDef = funBd->FindStruct(currStruct);
        Assert(structDef != nullptr, "Struct %s not found", currStruct.c_str());
        int idx = structDef->GetFieldIndex(t->ToStr());
        Assert(
            idx != -1, "Field %s not found in struct %s", t->ToStr().c_str(),
            structDef->GetName().c_str()
        );
        vecAcc.push_back(funBd->SymI32Const(idx));

        const auto &field = structDef->GetField(idx);
        termType = field.type;
        termBaseType = field.baseType;
        if (termType == SymIR::STRUCT)
          currStruct = field.structName;
        else if (termBaseType == SymIR::STRUCT)
          currStruct = field.structName;
        remainingDims = field.shape.size();
      } else {
        Panic("Unexpected access to scalar");
      }
      delete t;
    }

    bblBd->SymAssign(varDef, *eid, vecAcc);
    delete eid;
    delete varToken;
  }

  void SymSexpParser::buildReturn() { bblBd->SymReturn(); }

  void SymSexpParser::buildBranch() {
    const auto *cid = popArg<SymIRBuilder::CondID>();
    const auto *fToken = popArg<SymSexpLexer::Token>();
    Assert(
        fToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 2nd child (%s) of the branch is not an identifier, while it should be the "
        "label of the branch when its conditional is evaluating to false",
        fToken->FullInfo().c_str()
    );
    const auto *tToken = popArg<SymSexpLexer::Token>();
    Assert(
        tToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the branch is not an identifier, while it should be the "
        "label of the branch when its conditional is evaluating to true",
        tToken->FullInfo().c_str()
    );
    bblBd->SymBranch(tToken->ToStr(), fToken->ToStr(), *cid);
    delete fToken;
    delete tToken;
    delete cid;
  }

  void SymSexpParser::buildGoto() {
    const auto *labelToken = popArg<SymSexpLexer::Token>();
    Assert(labelToken->kind == SymSexpLexer::Token::Kind::TK_IDENT, "The first is not identity");
    bblBd->SymGoto(labelToken->ToStr());
    delete labelToken;
  }

  void SymSexpParser::buildBlock() {
    funBd->CloseBlock(bblBd);
    bblBd = nullptr;
  }

  void SymSexpParser::buildFunc() {
    funct = funBd->Build();
    funBd = nullptr;
  }

  Coef *SymSexpParser::buildCoef(const SymSexpLexer::Token *token, SymIR::Type type) {
    const auto tokenStr = token->ToStr();
    if (token->str.starts_with(SymSexpLower::KW_COEF_VAL_SYM)) {
      std::string tempName = std::string("__c") + std::to_string(funBd->GetSymbols().size());
      return funBd->SymCoef(tempName, tokenStr.substr(1), type);
    } else if (auto coef = funBd->FindSymbol(tokenStr); coef != nullptr) {
      Assert(
          typeid(*coef) == typeid(Coef), "The symbol with name \"%s\" is not a Coef",
          tokenStr.c_str()
      );
      return dynamic_cast<Coef *>(coef);
    } else {
      return funBd->SymCoef(tokenStr, type);
    }
  }
} // namespace symir
