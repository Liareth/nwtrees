#include "UnitTest.hpp"

#include <nwtrees/Lexer.hpp>

namespace
{
    template <typename T>
    std::string concat(const T& collection, const char separator = ' ')
    {
        std::string ret;
        for (const char* str : collection)
        {
            ret += str;
            ret += separator;
        }
        return ret;
    }
}

TEST_CLASS(Lexer)
{
    TEST_METHOD(Empty)
    {
        nwtrees::LexerOutput lex = nwtrees::lexer("");
        TEST_EXPECT(lex.tokens.empty());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());
    }

    TEST_METHOD(Preprocessor_Skip)
    {
        nwtrees::LexerOutput lex = nwtrees::lexer("#include <blah>");
        TEST_EXPECT(lex.tokens.empty());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());
    }

    TEST_METHOD(Comment_Skip)
    {
        nwtrees::LexerOutput lex = nwtrees::lexer("// comment 1\n/* comment 2 //\n*/// comment 3");
        TEST_EXPECT(lex.tokens.empty());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());
    }

    TEST_METHOD(Whitespace_Skip)
    {
        nwtrees::LexerOutput lex = nwtrees::lexer("    \r\n\t\t  \n\t  ");
        TEST_EXPECT(lex.tokens.empty());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());
    }

    TEST_METHOD(Keywords)
    {
        std::string keywords;

        for (const auto& [str, _] : nwtrees::keywords)
        {
            keywords += str;
            keywords += " ";
        }

        nwtrees::LexerOutput lex = nwtrees::lexer(keywords.c_str());
        TEST_EXPECT(lex.tokens.size() == nwtrees::keywords.size());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());

        for (size_t i = 0; i < lex.tokens.size(); ++i)
        {
            const nwtrees::Token& token = lex.tokens[i];
            TEST_EXPECT(token.type == nwtrees::Token::Keyword);
            TEST_EXPECT(token.keyword == nwtrees::keywords[i].second);
        }
    }

    TEST_METHOD(Identifiers)
    {
        static constexpr std::array identifiers { "integer", "floating", "stringless", "test", "obj" };
        nwtrees::LexerOutput lex = nwtrees::lexer(concat(identifiers).c_str());
        TEST_EXPECT(lex.tokens.size() == identifiers.size());
        TEST_EXPECT(lex.errors.empty());

        for (int i = 0; i < identifiers.size(); ++i)
        {
            const nwtrees::Token& token = lex.tokens[i];
            TEST_EXPECT(token.type == nwtrees::Token::Identifier);
            TEST_EXPECT(token.identifier_data.len == std::strlen(identifiers[i]));
            TEST_EXPECT(std::strncmp(lex.names.data() + token.identifier_data.idx, identifiers[i], token.identifier_data.len) == 0);
        }
    }

    TEST_METHOD(Identifiers_Invalid)
    {
        nwtrees::LexerOutput lex = nwtrees::lexer("0test");
        TEST_EXPECT(!lex.errors.empty());
    }

    TEST_METHOD(Literals_String)
    {
        static constexpr std::array literals { R"("test \" ")", R"("testnewline\n")" };
        nwtrees::LexerOutput lex = nwtrees::lexer(concat(literals, ';').c_str());
        TEST_EXPECT(lex.tokens.size() == literals.size() * 2);
        TEST_EXPECT(lex.errors.empty());

        for (int i = 0; i < literals.size(); i += 2)
        {
            const nwtrees::Token& token = lex.tokens[i];
            TEST_EXPECT(token.type == nwtrees::Token::Literal);
            TEST_EXPECT(token.literal == nwtrees::Literal::String);
            TEST_EXPECT(token.literal_data.str.len == std::strlen(literals[i]) - 2);
            TEST_EXPECT(std::strncmp(lex.names.data() + token.literal_data.str.idx, literals[i] + 1, token.literal_data.str.len - 2) == 0);
        }
    }

    TEST_METHOD(Literals_String_Concat)
    {
        static constexpr std::array literals { R"("test")", R"("test2")", R"("test3")" };
        nwtrees::LexerOutput lex = nwtrees::lexer(concat(literals).c_str());
        TEST_EXPECT(lex.tokens.size() == 1);

        const nwtrees::Token& token = lex.tokens[0];
        TEST_EXPECT(token.type == nwtrees::Token::Literal);
        TEST_EXPECT(token.literal == nwtrees::Literal::String);
        TEST_EXPECT(std::strncmp(lex.names.data() + token.literal_data.str.idx, "testtest2test3", token.literal_data.str.len) == 0);
    }

    TEST_METHOD(Literals_Int)
    {
        static constexpr std::array literals { "1", "10000", "01", "-1", "-10000", "0999", "0xFF", "+1000" };
        nwtrees::LexerOutput lex = nwtrees::lexer(concat(literals).c_str());
        TEST_EXPECT(lex.tokens.size() == literals.size());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());

        for (int i = 0; i < literals.size(); ++i)
        {
            const nwtrees::Token& token = lex.tokens[i];
            TEST_EXPECT(token.type == nwtrees::Token::Literal);
            TEST_EXPECT(token.literal == nwtrees::Literal::Int);

            const int as_int = (int)std::strtol(literals[i], nullptr, strstr(literals[i], "0x") ? 16 : 10);
            TEST_EXPECT(token.literal_data.integer == as_int);
        }
    }

    TEST_METHOD(Literals_Float)
    {
        static constexpr std::array literals { "1.0", "1.", "0.1", ".1", "-.1", "-.1e5", "+.1f", "10000f", "9e5" };
        nwtrees::LexerOutput lex = nwtrees::lexer(concat(literals).c_str());
        TEST_EXPECT(lex.tokens.size() == literals.size());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());

        for (int i = 0; i < literals.size(); ++i)
        {
            const nwtrees::Token& token = lex.tokens[i];
            TEST_EXPECT(token.type == nwtrees::Token::Literal);
            TEST_EXPECT(token.literal == nwtrees::Literal::Float);

            const float as_flt = (float)std::atof(literals[i]);
            TEST_EXPECT(token.literal_data.flt == as_flt);
        }
    }

    TEST_METHOD(Punctuators)
    {
        std::string punctuators;
        for (const auto& [str, _] : nwtrees::punctuators)
        {
            punctuators += str;
            punctuators += " ";
        }

        nwtrees::LexerOutput lex = nwtrees::lexer(punctuators.c_str());
        TEST_EXPECT(lex.tokens.size() == nwtrees::punctuators.size());
        TEST_EXPECT(lex.names.empty());
        TEST_EXPECT(lex.errors.empty());

        for (size_t i = 0; i < lex.tokens.size(); ++i)
        {
            const nwtrees::Token& token = lex.tokens[i];
            TEST_EXPECT(token.type == nwtrees::Token::Punctuator);
            TEST_EXPECT(token.punctuator == nwtrees::punctuators[i].second);
        }
    }

    TEST_METHOD(Invalid)
    {
        TEST_EXPECT(!nwtrees::lexer("`").errors.empty());
        TEST_EXPECT(!nwtrees::lexer("\\").errors.empty());
        TEST_EXPECT(!nwtrees::lexer("0c").errors.empty());
        TEST_EXPECT(!nwtrees::lexer("@@").errors.empty());
    }

    static constexpr nwtrees::Token tk(const nwtrees::Keyword type)
    {
        nwtrees::Token tk = { nwtrees::Token::Keyword };
        tk.keyword = type;
        return tk;
    }

    static constexpr nwtrees::Token tk(const nwtrees::Literal type)
    {
        nwtrees::Token tk = { nwtrees::Token::Literal };
        tk.literal = type;
        return tk;
    }

    static constexpr nwtrees::Token tk(const nwtrees::Punctuator type)
    {
        nwtrees::Token tk = { nwtrees::Token::Punctuator };
        tk.punctuator = type;
        return tk;
    }

    static constexpr nwtrees::Token tk()
    {
        return { nwtrees::Token::Identifier };
    }

    TEST_METHOD(Comprehensive)
    {
        using K = nwtrees::Keyword;
        using L = nwtrees::Literal;
        using P = nwtrees::Punctuator;

        const char* code =
        R"(
            int add(const int a, const int b)
            {
                return a + b;
            }

            void main()
            {
                const int value = add(5, 7);
                const string str = "Hello world!\n" "And me, too!";

                int value2 = value;
                value2 >>= 0x5F;
                value2 /= .1e7;
            }
        )";

        static constexpr std::array expected_tokens =
        {
            tk(K::Int), tk(), tk(P::LeftParen),
                tk(K::Const), tk(K::Int), tk(), tk(P::Comma),
                tk(K::Const), tk(K::Int), tk(),
            tk(P::RightParen),

            tk(P::LeftCurlyBracket),
                tk(K::Return), tk(), tk(P::Plus), tk(), tk(P::Semicolon),
            tk(P::RightCurlyBracket),

            tk(K::Void), tk(), tk(P::LeftParen), tk(P::RightParen),
            tk(P::LeftCurlyBracket),

                tk(K::Const), tk(K::Int), tk(), tk(P::Equal), tk(), tk(P::LeftParen),
                    tk(L::Int), tk(P::Comma), tk(L::Int),
                tk(P::RightParen), tk(P::Semicolon),

                tk(K::Const), tk(K::String), tk(), tk(P::Equal), tk(L::String), tk(P::Semicolon),

                tk(K::Int), tk(), tk(P::Equal), tk(), tk(P::Semicolon),

                tk(), tk(P::GreaterGreaterEquals), tk(L::Int), tk(P::Semicolon),

                tk(), tk(P::SlashEquals), tk(L::Float), tk(P::Semicolon),

            tk(P::RightCurlyBracket),
        };

        nwtrees::LexerOutput lex = nwtrees::lexer(code);
        TEST_EXPECT(expected_tokens.size() == lex.tokens.size());

        for (int i = 0; i < lex.tokens.size(); ++i)
        {
            const nwtrees::Token& expected_token = expected_tokens[i];
            const nwtrees::Token& actual_token = lex.tokens[i];
            const int len_to_compare = expected_token.type == nwtrees::Token::Identifier ? 1 : 2;
            TEST_EXPECT(memcmp(&expected_token, &actual_token, len_to_compare) == 0); // Checks types only, not contents
        }
    }
};
