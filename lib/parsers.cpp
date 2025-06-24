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
      } else {
        eatNonwhitespace(/*excl=*/[](char c) { return c == ')'; });
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
    for (auto i = 0; i < chs.size(); i++) {
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
    if (func != nullptr) {
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
    funBd = std::make_unique<FuncBuilder>(
        nameToken.ToStr(), SymIR::GetTypeFromSName(typeToken.ToStr())
    );
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
    const auto termType = varDef != nullptr ? varDef->GetType() : SymIR::Type::I32;
    pushArg<SymIRBuilder::TermID>(bblBd->SymTerm(op, buildCoef(coefToken, termType), varDef));
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
    const auto *typeToken = popArg<SymSexpLexer::Token>();
    Assert(
        typeToken->kind == SymSexpLexer::Token::Kind::TK_TYPE_I32,
        "The 2nd child (%s) of the param node is not an identifier, while it should be the "
        "type of the term",
        typeToken->FullInfo().c_str()
    );
    const auto *varToken = popArg<SymSexpLexer::Token>();
    Assert(
        varToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the parameter is not an identifier, while it should be the "
        "type of the term",
        varToken->FullInfo().c_str()
    );
    funBd->SymParam(varToken->ToStr(), SymIR::GetTypeFromSName(typeToken->ToStr()));
    delete typeToken;
    delete varToken;
  }

  void SymSexpParser::buildLocal() {
    const auto *typeToken = popArg<SymSexpLexer::Token>();
    Assert(
        typeToken->kind == SymSexpLexer::Token::Kind::TK_TYPE_I32,
        "The 3rd child (%s) of the param node is not an identifier, while it should be the "
        "type of the term",
        typeToken->FullInfo().c_str()
    );
    const auto *coefToken = popArg<SymSexpLexer::Token>();
    Assert(
        coefToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 2nd child (%s) of the parameter is not an identifier, while it should be the "
        "coefficient of the term",
        coefToken->FullInfo().c_str()
    );
    const auto *varToken = popArg<SymSexpLexer::Token>();
    Assert(
        varToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the parameter is not an identifier, while it should be the "
        "type of the term",
        varToken->FullInfo().c_str()
    );
    const auto localType = SymIR::GetTypeFromSName(typeToken->ToStr());
    funBd->SymLocal(varToken->ToStr(), buildCoef(coefToken, localType), localType);
    delete typeToken;
    delete coefToken;
    delete varToken;
  }

  void SymSexpParser::buildAssign() {
    const auto *eid = popArg<SymIRBuilder::ExprID>();
    const auto *varToken = popArg<SymSexpLexer::Token>();
    Assert(
        varToken->kind == SymSexpLexer::Token::Kind::TK_IDENT,
        "The 1st child (%s) of the assignment is not an identifier, while it should be the "
        "variable of the assignment",
        varToken->FullInfo().c_str()
    );
    bblBd->SymAssign(funBd->FindVar(varToken->ToStr()), *eid);
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
    func = funBd->Build();
    funBd = nullptr;
  }

  Coef *SymSexpParser::buildCoef(const SymSexpLexer::Token *token, SymIR::Type type) {
    if (token->str.starts_with(SymSexpLower::KW_COEF_VAL_SYM)) {
      std::string tempName = std::string("__c") + std::to_string(funBd->GetSymbols().size());
      return funBd->SymCoef(tempName, token->ToStr().substr(1), type);
    } else {
      return funBd->SymCoef(token->ToStr(), type);
    }
  }
} // namespace symir
