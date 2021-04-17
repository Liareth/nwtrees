#pragma once

#include <nwtrees/NWTrees.hpp>

#include <array>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace nwtrees
{
    struct NameBufferEntry
    {
        int idx;
        int len;
    };

    enum class Keyword : uint8_t
    {
        Action,
        Break,
        Case,
        Const,
        Default,
        Do,
        Effect,
        Else,
        Event,
        Float,
        For,
        If,
        Int,
        ItemProperty,
        Location,
        Object,
        Return,
        String,
        Struct,
        Switch,
        Talent,
        Vector,
        Void,
        While,

        EnumCount
    };

    struct Identifier
    {
        NameBufferEntry str;
    };

    struct Literal
    {
        enum Type
        {
            String,
            Int,
            Float
        } type;

        union
        {
            NameBufferEntry str;
            int integer;
            float flt;
        };
    };

    enum class Punctuator : uint8_t
    {
        // See WG14/N1256 6.4.6 with some exclusions and additions
        Amp,
        AmpAmp,
        AmpEquals,
        Asterisk,
        AsteriskEquals,
        Caret,
        CaretEquals,
        Colon,
        ColonColon,
        Comma,
        Dot,
        DotDotDot,
        Equal,
        EqualEqual,
        Exclamation,
        ExclamationEquals,
        Greater,
        GreaterEquals,
        GreaterGreater,
        GreaterGreaterEquals,
        LeftCurlyBracket,
        LeftParen,
        LeftSquareBracket,
        Less,
        LessEquals,
        LessLess,
        LessLessEquals,
        Minus,
        MinusEquals,
        MinusMinus,
        Modulo,
        ModuloEquals,
        Pipe,
        PipeEquals,
        PipePipe,
        Plus,
        PlusEquals,
        PlusPlus,
        Question,
        RightCurlyBracket,
        RightParen,
        RightSquareBracket,
        Semicolon,
        Slash,
        SlashEquals,
        Tilde,

        EnumCount
    };

    struct DebugData
    {
        int line;
        int column_start;
        int column_end;
    };

    struct DebugRange
    {
        int line;
        int index_start;
        int index_end;
    };

    struct Token
    {
        enum Type : uint8_t
        {
            Keyword,
            Identifier,
            Literal,
            Punctuator,

            EnumCount
        } type;

        union
        {
            nwtrees::Keyword keyword;
            nwtrees::Identifier identifier;
            nwtrees::Literal literal;
            nwtrees::Punctuator punctuator;
        };

        DebugData debug;
    };

    struct LexerError
    {

    };

    struct LexerOutput
    {
        std::vector<Token> tokens;
        std::vector<char> names;
        std::vector<Error> errors;
        std::vector<DebugRange> debug_ranges;
    };

    LexerOutput lexer(const char* data);

#define TREES_TK(str, enum) std::make_pair(std::string_view(str), enum)

    // Must match order of Keyword enum.
    static constexpr inline std::array keywords
    {
        TREES_TK("action", Keyword::Action),
        TREES_TK("break", Keyword::Break),
        TREES_TK("case", Keyword::Case),
        TREES_TK("const", Keyword::Const),
        TREES_TK("default", Keyword::Default),
        TREES_TK("do", Keyword::Do),
        TREES_TK("effect", Keyword::Effect),
        TREES_TK("else", Keyword::Else),
        TREES_TK("event", Keyword::Event),
        TREES_TK("float", Keyword::Float),
        TREES_TK("for", Keyword::For),
        TREES_TK("if", Keyword::If),
        TREES_TK("int", Keyword::Int),
        TREES_TK("itemproperty", Keyword::ItemProperty),
        TREES_TK("location", Keyword::Location),
        TREES_TK("object", Keyword::Object),
        TREES_TK("return", Keyword::Return),
        TREES_TK("string", Keyword::String),
        TREES_TK("struct", Keyword::Struct),
        TREES_TK("switch", Keyword::Switch),
        TREES_TK("talent", Keyword::Talent),
        TREES_TK("vector", Keyword::Vector),
        TREES_TK("void", Keyword::Void),
        TREES_TK("while", Keyword::While),
    }; static_assert((size_t)Keyword::EnumCount == keywords.size());

    // Must match order of Punctuator enum.
    static constexpr inline std::array punctuators
    {
        TREES_TK("&", Punctuator::Amp),
        TREES_TK("&&", Punctuator::AmpAmp),
        TREES_TK("&=", Punctuator::AmpEquals),
        TREES_TK("*", Punctuator::Asterisk),
        TREES_TK("*=", Punctuator::AsteriskEquals),
        TREES_TK("^", Punctuator::Caret),
        TREES_TK("^=", Punctuator::CaretEquals),
        TREES_TK(":", Punctuator::Colon),
        TREES_TK("::", Punctuator::ColonColon),
        TREES_TK(",", Punctuator::Comma),
        TREES_TK(".", Punctuator::Dot),
        TREES_TK("...", Punctuator::DotDotDot),
        TREES_TK("=", Punctuator::Equal),
        TREES_TK("==", Punctuator::EqualEqual),
        TREES_TK("!", Punctuator::Exclamation),
        TREES_TK("!=", Punctuator::ExclamationEquals),
        TREES_TK(">", Punctuator::Greater),
        TREES_TK(">=", Punctuator::GreaterEquals),
        TREES_TK(">>", Punctuator::GreaterGreater),
        TREES_TK(">>=", Punctuator::GreaterGreaterEquals),
        TREES_TK("{", Punctuator::LeftCurlyBracket),
        TREES_TK("(", Punctuator::LeftParen),
        TREES_TK("[", Punctuator::LeftSquareBracket),
        TREES_TK("<", Punctuator::Less),
        TREES_TK("<=", Punctuator::LessEquals),
        TREES_TK("<<", Punctuator::LessLess),
        TREES_TK("<<=", Punctuator::LessLessEquals),
        TREES_TK("-", Punctuator::Minus),
        TREES_TK("-=", Punctuator::MinusEquals),
        TREES_TK("--", Punctuator::MinusMinus),
        TREES_TK("%", Punctuator::Modulo),
        TREES_TK("%=", Punctuator::ModuloEquals),
        TREES_TK("|", Punctuator::Pipe),
        TREES_TK("|=", Punctuator::PipeEquals),
        TREES_TK("||", Punctuator::PipePipe),
        TREES_TK("+", Punctuator::Plus),
        TREES_TK("+=", Punctuator::PlusEquals),
        TREES_TK("++", Punctuator::PlusPlus),
        TREES_TK("?", Punctuator::Question),
        TREES_TK("}", Punctuator::RightCurlyBracket),
        TREES_TK(")", Punctuator::RightParen),
        TREES_TK("]", Punctuator::RightSquareBracket),
        TREES_TK(";", Punctuator::Semicolon),
        TREES_TK("/", Punctuator::Slash),
        TREES_TK("/=", Punctuator::SlashEquals),
        TREES_TK("~", Punctuator::Tilde),
    }; static_assert((size_t)Punctuator::EnumCount == punctuators.size());

#undef TREES_TK
}
