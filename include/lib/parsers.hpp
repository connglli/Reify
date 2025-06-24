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

#ifndef GRAPHFUZZ_PARSER_HPP
#define GRAPHFUZZ_PARSER_HPP

#include <functional>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>

#include "lib/lang.hpp"
#include "lib/lowers.hpp"

namespace symir {

  /// SymSexpLexer is the lexer for the SymIR in the S Expression form.
  class SymSexpLexer {
  public:
    struct Token {
      // clang-format off
      enum Kind {
        // Parenthesis
        TK_LPAREN = 0,
        TK_RPAREN,

        // Keywords
#define XX(name, ...) TK_KW_## name,
        SYMIR_KEYWORD_LIST(XX)
#undef XX

        // Term operators
#define XX(name, ...) TK_OP_TERM_## name,
        SYMIR_TERMOP_LIST(XX)
#undef XX

        // Expr operators
#define XX(name, ...) TK_OP_EXPR_## name,
        SYMIR_EXPROP_LIST(XX)
#undef XX

        // Cond operators
#define XX(name, ...) TK_OP_COND_## name,
        SYMIR_CONDOP_LIST(XX)
#undef XX

        // Types
#define XX(name, ...) TK_TYPE_## name,
        SYMIR_TYPE_LIST(XX)
#undef XX

        // Coefficients and variables
        TK_IDENT,

        // End of file/input
        TK_EOF,

        // Unknown tokens
        TK_UNKNOWN,
      };
      // clang-format on

      const Kind kind = TK_UNKNOWN;        // Type of the token
      const size_t stRow = -1, stCol = -1; // Position of the token in the source code
      const size_t edRow = -1, edCol = -1; // Position of the token in the source code
      const std::string_view str = "";     // The actual token string

      /// Get the raw name of each token kind
      static std::string GetKindName(Kind op);

      /// Check if a token is of a specific kind
      static bool IsOfKind(const std::string_view &token, Kind kind);

      /// Parse the kind of a given token
      static Kind ParseKind(const std::string_view &token);

      /// Copy the content of the token and return
      [[nodiscard]] std::string ToStr() const { return std::string(str); }

      [[nodiscard]] std::string FullInfo() const {
        std::ostringstream oss;
        oss << "token=" << str << ", type=" << GetKindName(kind) << ", row" << stRow
            << ", col=" << stCol;
        return oss.str();
      }
    };

    using TokenStream = std::function<Token()>;
    using CharMatcher = std::function<bool(char)>;

  public:
    explicit SymSexpLexer(std::string sirCode) :
        source(std::move(sirCode)), srcView(source), stream(source) {}

    /// Return the source code that we're lexing
    const std::string &GetSource() const { return source; }

    /// Get the token stream of the lexed code
    TokenStream Lex();

    /// Reset the lexer to facilitate re-lexing
    void Reset() {
      stream = std::istringstream(source);
      curInd = 0;
      curRow = 0;
      curCol = 0;
    }

  private:
    /// Check the following num characters without forwarding our pointers
    std::vector<char> lookahead(int num = 1);

    /// Forward to eat all whitespace and characters matching the incl pattern
    void eatWhitespace(CharMatcher incl = [](char) { return false; });

    /// Forward to eat all characters that are not whitespace and not matching the excl pattern
    void eatNonwhitespace(CharMatcher excl = [](char) { return true; });

    /// Forward to each num characters
    void eatChars(int num);

    /// Forward to each all characters matching pred pattern
    void eatCharsIf(CharMatcher pred);

  private:
    const std::string source;          // The source SymIR code (S Expression) to be lexed
    const std::string_view srcView;    // The source code view
    std::istringstream stream;         // Input stream from the source code
    std::string::size_type curInd = 0; // Current index in the source code
    std::string::size_type curRow = 0; // Current row position in the source code
    std::string::size_type curCol = 0; // Current column position in the source code
  };

  /// SymSexpParser is the AST parser for the SymIR in the S Expression form.
  class SymSexpParser {

  public:
    explicit SymSexpParser(std::string sirCode) : lexer(std::move(sirCode)) {}

    /// Return the source code that we're lexing
    const std::string &GetSource() const { return lexer.GetSource(); }

    /// Return the lexer we're using
    const SymSexpLexer &GetLexer() const { return lexer; }

    /// Parse the source code into an AST returned by GetFunc
    void Parse();

    /// Return the parsed AST or nullptr
    [[nodiscard]] Func *GetFunc() const { return func.get(); }

    /// Return the parsed AST or nullptr. Different from GetFunc(), we transfer
    /// the ownership of the function to the user.
    std::unique_ptr<Func> TakeFunc() {
      lexer.Reset();
      return std::move(func);
    }

  private:
    void pushOp(SymSexpLexer::Token::Kind op) { opStack.push(op); }

    SymSexpLexer::Token::Kind popOp() {
      Assert(opStack.size() > 0, "The operator stack is empty now!");
      auto op = opStack.top();
      opStack.pop();
      return op;
    }

    template<typename Class, typename... Args>
    void pushArg(Args... args) {
      argStack.push(new Class(args...));
    }

    template<typename Class>
    Class *popArg() {
      Assert(argStack.size() > 0, "The argument stack is empty now!");
      Class *arg = static_cast<Class *>(argStack.top());
      argStack.pop();
      return arg;
    }

    void prepFun(const SymSexpLexer::TokenStream &stream);
    void prepBbl(const SymSexpLexer::TokenStream &stream);

    void buildTerm(Term::Op op);
#define XX(name, capt, ...) void build##capt##Term();
    SYMIR_TERMOP_LIST(XX)
#undef XX
    void buildExpr(Expr::Op op);
#define XX(name, capt, ...) void build##capt##Expr();
    SYMIR_EXPROP_LIST(XX)
#undef XX
    void buildCond(Cond::Op op);
#define XX(name, capt, ...) void build##capt##Cond();
    SYMIR_CONDOP_LIST(XX)
#undef XX
    void buildParam();
    void buildLocal();
    void buildAssign();
    void buildReturn();
    void buildBranch();
    void buildGoto();
    void buildBlock();
    void buildFunc();
    Coef *buildCoef(const SymSexpLexer::Token *token, SymIR::Type type);

  private:
    SymSexpLexer lexer;                   // The lexer we're using
    std::unique_ptr<Func> func = nullptr; // The AST after parsing

    // Context to parse the S expression code
    std::unique_ptr<FuncBuilder> funBd = nullptr;    // The top level function builder
    BlockBuilder *bblBd = nullptr;                   // The current block builder
    std::stack<SymSexpLexer::Token::Kind> opStack{}; // The operator stack
    std::stack<void *> argStack{};                   // The operand/argument stack
  };
} // namespace symir
#endif // GRAPHFUZZ_PARSER_HPP
