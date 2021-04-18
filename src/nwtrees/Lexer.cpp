#include <nwtrees/Lexer.hpp>
#include <nwtrees/util/Assert.hpp>

#include <algorithm>
#include <string_view>
#include <unordered_map>

using namespace nwtrees;

namespace
{
    void prepare_output(LexerOutput& output);

    struct LexerInput
    {
        const char* base;
        const char* head() const { return base + offset; }
        int offset;
    };

    char seek(LexerInput& input);

    struct DebugRange
    {
        int line;
        int index_start;
        int index_end;
    };

    std::vector<DebugRange> make_debug_ranges(const char* data);
    const DebugRange& find_debug_range(const std::vector<DebugRange>& ranges, const LexerInput& input);

    struct LexerMatch
    {
        Token token;
        int length;
    };

    bool tokenize_keyword(const LexerInput& input, LexerMatch* match);
    bool tokenize_identifier(const LexerInput& input, LexerMatch* match);
    bool tokenize_literal(const LexerInput& input, LexerMatch* match);
    bool tokenize_punctuator(const LexerInput& input, LexerMatch* match);

    inline char read(const LexerInput& input) { return input.base[input.offset]; }
    inline char peek(const LexerInput& input, int count = 1) { return input.base[input.offset + count]; }

    template <typename ... Args>
    inline int skip_until(const char* tail, const Args ... terms)
    {
        static constexpr auto contains =
            [](const auto first, const auto... args) -> bool
        {
            return ((first == args) || ...);
        };

        const char* head = tail;
        while (const char ch = *head) { if (contains(ch, terms ...)) break; ++head; }
        return (int)(head - tail);
    }

    inline bool cmp(const LexerMatch& lhs, const LexerMatch& rhs) { return lhs.length > rhs.length; }
    inline bool is_whitespace(const char ch) { return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r' || ch == '\n'; }
    inline bool is_letter(const char ch) { return (ch >= 'A' && ch <= 'Z') || ch >= 'a' && ch <= 'z'; }
    inline bool is_digit(const char ch) { return ch >= '0' && ch <= '9'; }
    inline bool is_digit_hex(const char ch) { return is_digit(ch) || (ch >= 'A' && ch <= 'F') || ch >= 'a' && ch <= 'f'; }

    void prepare_output(LexerOutput& output)
    {
        output.tokens.clear();
        output.names.clear();
        output.errors.clear();
    }

    char seek(LexerInput& input)
    {
        while (const char ch = read(input))
        {
            // For now, we completely skip the preprocessor.
            if (ch == '#')
            {
                input.offset += skip_until(input.head(), '\n');
            }
            // We skip past comments.
            else if (ch == '/')
            {
                const char peek_ch = peek(input);

                // C++-style comment: skip to end of line.
                if (peek_ch == '/')
                {
                    input.offset += skip_until(input.head(), '\n');
                }
                // C-style comment: skip to matching symbol.
                else if (peek_ch == '*')
                {
                    while (read(input))
                    {
                        input.offset += std::max(1, skip_until(input.head(), '/'));

                        if (peek(input, -1) == '*')
                        {
                            ++input.offset;
                            break;
                        }
                    }
                }
                // False positive: probably an operator, just return to process.
                else
                {
                    return ch;
                }
            }
            // We skip past whitespace.
            else if (is_whitespace(ch))
            {
                ++input.offset;
            }
            // Anything else is valid to process.
            else
            {
                return ch;
            }
        }

        return '\0';
    }

    std::vector<DebugRange> make_debug_ranges(const char* data)
    {
        const char* head = data;

        int line = 0;
        int line_idx_start = 0;

        std::vector<DebugRange> ranges;

        while (const char ch = *head++)
        {
            if (ch == '\n')
            {
                const int line_idx_next = (int)(head - data);
                ranges.push_back({ line, line_idx_start, line_idx_next - 1 });
                line = line + 1;
                line_idx_start = line_idx_next;
            }
        }

        ranges.push_back({ line, line_idx_start, (int)(head - data - 1) });
        return ranges;
    }

    const DebugRange& find_debug_range(const std::vector<DebugRange>& ranges, const LexerInput& input)
    {
        DebugRange target_range;
        target_range.index_end = input.offset;
        auto iter = std::lower_bound(std::begin(ranges), std::end(ranges), target_range,
            [](const auto& lhs, const auto rhs) { return lhs.index_end < rhs.index_end; });
        NWTREES_ASSERT(iter != std::end(ranges));
        return *iter;
    }

    bool tokenize_keyword(const LexerInput& input, LexerMatch* match)
    {
        Keyword keyword;

        switch (read(input))
        {
            case 'a': keyword = Keyword::Action; break;
            case 'b': keyword = Keyword::Break; break;
            case 'c': keyword = peek(input) == 'a' ? Keyword::Case : Keyword::Const; break;
            case 'd': keyword = peek(input) == 'e' ? Keyword::Default : Keyword::Do; break;
            case 'e': switch (peek(input))
            {
                case 'f': keyword = Keyword::Effect; break;
                case 'l': keyword = Keyword::Else; break;
                case 'v': keyword = Keyword::Event; break;
                default: return false;
            }; break;
            case 'f': keyword = peek(input) == 'l' ? Keyword::Float : Keyword::For; break;
            case 'i': switch (peek(input))
            {
                case 'f': keyword = Keyword::If; break;
                case 'n': keyword = Keyword::Int; break;
                case 't': keyword = Keyword::ItemProperty; break;
                default: return false;
            }; break;
            case 'l': keyword = Keyword::Location; break;
            case 'o': keyword = Keyword::Object; break;
            case 'r': keyword = Keyword::Return; break;
            case 's': switch (peek(input))
            {
                case 't': keyword = peek(input, 3) == 'i' ? Keyword::String : Keyword::Struct; break;
                case 'w': keyword = Keyword::Switch; break;
                default: return false;
            }; break;
            case 't': keyword = Keyword::Talent; break;
            case 'v': keyword = peek(input) == 'e' ? Keyword::Vector : Keyword::Void; break;
            case 'w': keyword = Keyword::While; break;

            default: return false;
        }

        const std::string_view& keyword_sv = keywords[(int)keyword].first;

        if (std::strncmp(input.head(), keyword_sv.data(), keyword_sv.length()) == 0)
        {
            match->length = (int)keyword_sv.length();
            match->token.type = Token::Keyword;
            match->token.keyword = keyword;
            return true;
        }

        return false;
    }

    bool tokenize_identifier(const LexerInput& input, LexerMatch* match)
    {
        const char* head = input.head();
        if (!is_letter(*head) && *head != '_') return false;

        while (const char ch = *++head)
        {
            if (!is_letter(ch) && !is_digit(ch) && ch != '_') break;
        }

        match->length = (int)(head - input.head());
        match->token.type = Token::Identifier;
        match->token.identifier_data = { input.offset, match->length };
        return true;
    }

    bool tokenize_literal(const LexerInput& input, LexerMatch* match)
    {
        const char first_ch = read(input);

        const bool is_string = first_ch == '\"';
        const bool is_number = is_digit(first_ch);
        const bool is_decimal = first_ch == '.';
        const bool is_sign = first_ch == '+' || first_ch == '-';

        if (is_string)
        {
            // We will scan until we find a string close which is not escaped, or a newline.
            LexerInput temp_input = input;
            ++temp_input.offset;

            while (read(temp_input))
            {
                const char* head = temp_input.head();
                temp_input.offset += skip_until(head, '\"', '\n');

                if (read(temp_input) == '\n')
                {
                    return false; // New line inside string (non-escaped); this string is invalid.
                }
                else if (peek(temp_input, -1) == '\\')
                {
                    ++temp_input.offset; // Escaped quote inside string: step past it.
                }
                else
                {
                    break; // End of string.
                }
            }

            match->length = temp_input.offset - input.offset + 1;
            match->token.type = Token::Literal;
            match->token.literal = Literal::String;
            match->token.literal_data.str = { input.offset + 1, match->length - 2 };
            return true;
        }
        else if (is_number || is_decimal || is_sign)
        {
            const bool is_hex = first_ch == '0' && (peek(input) == 'x' || peek(input) == 'X');

            bool seen_number = is_number;
            bool seen_decimal = is_decimal;
            bool seen_exponent = false;
            bool seen_float_specifier = false;

            // If we're a number, we need to keep track of whether we've seen a decimal place,
            // and scan until we're no longer a number or a decimal place.
            int distance;
            for (distance = is_hex ? 2 : 1; const char ch = peek(input, distance); ++distance)
            {
                if (!is_hex && !seen_decimal && ch == '.')
                {
                    seen_decimal = true;
                }
                else if (!is_hex && !seen_exponent && ch == 'e')
                {
                    seen_exponent = true;
                }
                else if (!is_hex && !seen_float_specifier && ch == 'f')
                {
                    seen_float_specifier = true;
                }
                else if (is_digit(ch) || (is_hex && is_digit_hex(ch)))
                {
                    seen_number = true;
                }
                else
                {
                    // We need to check whether this non-digit is a punctuator or whitespace.
                    // If it is, we are a valid literal; if it isn't, this is an invalid token.
                    LexerInput temp_input = input;
                    temp_input.offset += distance;

                    if (!tokenize_punctuator(temp_input, match) && // note: match is written over later so it doesn't matter
                        !is_whitespace(read(temp_input)))
                    {
                        return false;
                    }

                    break;
                }
            }

            // It's possible that we haven't seen a number - this could be caused by an operator (. + -) false positive.

            if (!seen_number) return false;

            // Now we can proceed with parsing from the string representation.

            match->length = distance;
            match->token.type = Token::Literal;

            const char* head = input.head();
            char* parsed_head;

            // If we've seen a decimal, exponent, or floating specifier, we're a float.
            if (seen_decimal || seen_exponent || seen_float_specifier)
            {
                const double parsed = std::strtod(input.head(), &parsed_head);
                match->token.literal = Literal::Float;
                match->token.literal_data.flt = (float)parsed;
            }
            // Otherwise, we're an int.
            else
            {
                const int base = is_hex ? 16 : 10; // note: nwscript does not support octal with leading 0
                const long int parsed = std::strtol(input.head(), &parsed_head, base);
                match->token.literal = Literal::Int;
                match->token.literal_data.integer = (int)parsed;
            }

            int parsed_distance = (int)(parsed_head - head);
            if (seen_float_specifier) ++parsed_distance; // parsing skipped the float specifier, account for it here

            NWTREES_ASSERT_MSG(parsed_distance == distance,
                "The function that parses numbers disagrees with how long the number is.");

            return true;
        }

        return false;
    }

    bool tokenize_punctuator(const LexerInput& input, LexerMatch* match)
    {
        Punctuator punctuator;

        switch (read(input))
        {
            case '&': switch (peek(input))
            {
                default: punctuator = Punctuator::Amp; break;
                case '&': punctuator = Punctuator::AmpAmp; break;
                case '=': punctuator = Punctuator::AmpEquals; break;
            }; break;
            case '*': punctuator = peek(input) != '=' ? Punctuator::Asterisk : Punctuator::AsteriskEquals; break;
            case '^': punctuator = peek(input) != '=' ? Punctuator::Caret : Punctuator::CaretEquals; break;
            case ':': punctuator = peek(input) != ':' ? Punctuator::Colon : Punctuator::ColonColon; break;
            case ',': punctuator = Punctuator::Comma; break;
            case '.': punctuator = peek(input) != '.' || peek(input, 2) != '.' ? Punctuator::Dot : Punctuator::DotDotDot; break;
            case '=': punctuator = peek(input) != '=' ? Punctuator::Equal : Punctuator::EqualEqual; break;
            case '!': punctuator = peek(input) != '=' ? Punctuator::Exclamation : Punctuator::ExclamationEquals; break;
            case '>': switch (peek(input))
            {
                default: punctuator = Punctuator::Greater; break;
                case '=': punctuator = Punctuator::GreaterEquals; break;
                case '>': punctuator = peek(input, 2) != '=' ? Punctuator::GreaterGreater : Punctuator::GreaterGreaterEquals; break;
            }; break;
            case '{': punctuator = Punctuator::LeftCurlyBracket; break;
            case '(': punctuator = Punctuator::LeftParen; break;
            case '[': punctuator = Punctuator::LeftSquareBracket; break;
            case '<': switch (peek(input))
            {
                default: punctuator = Punctuator::Less; break;
                case '=': punctuator = Punctuator::LessEquals; break;
                case '<': punctuator = peek(input, 2) != '=' ? Punctuator::LessLess : Punctuator::LessLessEquals; break;
            }; break;
            case '-': switch (peek(input))
            {
                default: punctuator = Punctuator::Minus; break;
                case '=': punctuator = Punctuator::MinusEquals; break;
                case '-': punctuator = Punctuator::MinusMinus; break;
            }; break;
            case '%': punctuator = peek(input) != '=' ? Punctuator::Modulo : Punctuator::ModuloEquals; break;
            case '|': switch (peek(input))
            {
                default: punctuator = Punctuator::Pipe; break;
                case '=': punctuator = Punctuator::PipeEquals; break;
                case '|': punctuator = Punctuator::PipePipe; break;
            }; break;
            case '+': switch (peek(input))
            {
                default: punctuator = Punctuator::Plus; break;
                case '=': punctuator = Punctuator::PlusEquals; break;
                case '+': punctuator = Punctuator::PlusPlus; break;
            }; break;
            case '?': punctuator = Punctuator::Question; break;
            case '}': punctuator = Punctuator::RightCurlyBracket; break;
            case ')': punctuator = Punctuator::RightParen; break;
            case ']': punctuator = Punctuator::RightSquareBracket; break;
            case ';': punctuator = Punctuator::Semicolon; break;
            case '/': punctuator = peek(input) != '=' ? Punctuator::Slash : Punctuator::SlashEquals; break;
            case '~': punctuator = Punctuator::Tilde; break;

            default: return false;
        }

        match->length = (int)punctuators[(int)punctuator].first.length();
        match->token.type = Token::Punctuator;
        match->token.punctuator = punctuator;

        return true;
    }
}

LexerOutput nwtrees::lexer(const char* data, LexerOutput&& prev_output)
{
    LexerOutput output = std::move(prev_output);
    prepare_output(output);

    LexerInput input = { data, 0 };

    while (seek(input))
    {
        LexerMatch matches[Token::EnumCount];
        int match_count = 0;

        // -- Gather all matches.

        if (tokenize_keyword(input, matches + match_count))
        {
            ++match_count;
        }

        if (tokenize_identifier(input, matches + match_count))
        {
            ++match_count;
        }

        if (tokenize_literal(input, matches + match_count))
        {
            ++match_count;
        }

        if (tokenize_punctuator(input, matches + match_count))
        {
            ++match_count;
        }

        if (!match_count)
        {
            const DebugRange& range = find_debug_range(make_debug_ranges(data), input);

            char line_buff[128];
            const size_t len_to_copy = std::min(sizeof(line_buff) - 1, (size_t)(range.index_end - range.index_start));
            memcpy(line_buff, data + range.index_start, len_to_copy);
            line_buff[len_to_copy] = '\0';
            output.errors.emplace_back(Error::Unknown, std::vector<std::string>{"Unknown Token", line_buff});
            break;
        }

        // -- Sort by token length: longest length wins.
        // Note: the stable_sort ensures that if we need to choose between an equal-length token of different types,
        //       we select the token in the same order as listed above. This is important for keyword vs identifier.

        if (match_count > 1)
        {
            std::stable_sort(matches, matches + match_count, &cmp);
        }

        // -- Commit token.

        LexerMatch& selected_match = matches[0];
        output.tokens.push_back(std::move(selected_match.token));

        // -- Step stream forward, past the matched token length.

        input.offset += selected_match.length;
    }

    // -- For tokens that need name buffers, prepare the buffer and update the name entry.

    for (Token& token : output.tokens)
    {
        const bool is_identifier = token.type == Token::Identifier;
        const bool is_str_literal = token.type == Token::Literal && token.literal == Literal::String;

        if (is_identifier || is_str_literal)
        {
            NameBufferEntry* entry = is_identifier ? &token.identifier_data : &token.literal_data.str;
            const size_t new_idx = output.names.size();
            output.names.resize(new_idx + entry->len);
            std::memcpy(output.names.data() + new_idx, input.base + entry->idx, entry->len);
            entry->idx = (int)new_idx;
        }
    }

    // -- We will check for any string literals that are together and merge them into one token.
    // This is quite easy: because they are next to each other, their contents are guaranteed to be next
    // to each other in the buffer, so we just delete the ones at the end and increase the length of the first.

    return output;
}
