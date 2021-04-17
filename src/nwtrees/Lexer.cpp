#include <nwtrees/Assert.hpp>
#include <nwtrees/Lexer.hpp>

#include <algorithm>
#include <ctype.h>
#include <span>
#include <string_view>
#include <unordered_map>

using namespace nwtrees;

namespace
{
    struct LexerInput
    {
        const char* base;
        const char* head() const { return base + offset; }
        int offset;
    };

    struct LexerMatch
    {
        Token token;
        int length;
    };

    char seek(LexerInput& input);
    char read(const LexerInput& input);
    char peek(const LexerInput& input, int count = 1);

    int skip_until(const char* tail, const char term);
    int skip_until(const char* tail, const std::span<const char>& matches);

    std::vector<DebugRange> make_debug_ranges(const LexerInput& input, LexerOutput& output);
    const DebugRange& find_debug_range(const std::vector<DebugRange>& ranges, const LexerInput& input);
    void add_debug_data(const LexerInput& input, Token& token);

    bool tokenize_keyword(const LexerInput& input, LexerMatch* match);
    bool tokenize_identifier(const LexerInput& input, LexerMatch* match);
    bool tokenize_literal(const LexerInput& input, LexerMatch* match);
    bool tokenize_punctuator(const LexerInput& input, LexerMatch* match);

    bool cmp(const LexerMatch& lhs, const LexerMatch& rhs) { return lhs.length > rhs.length; }
    bool is_whitespace(const char ch) { return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r' || ch == '\n'; }

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

    char read(const LexerInput& input)
    {
        return input.base[input.offset];
    }

    char peek(const LexerInput& input, int count)
    {
        return input.base[input.offset + count];
    }

    int skip_until(const char* tail, const char term)
    {
        return skip_until(tail, std::span<const char>(&term, 1));
    }

    int skip_until(const char* tail, const std::span<const char>& matches)
    {
        const char* head = tail;
        while (const char ch = *head)
        {
            if (std::find(std::begin(matches), std::end(matches), ch) != std::end(matches)) break;
            ++head;
        }
        return (int)(head - tail);
    }

    std::vector<DebugRange> make_debug_ranges(const LexerInput& input, LexerOutput& output)
    {
        const char* str = input.base;

        int line = 0;
        int line_idx_start = 0;

        std::vector<DebugRange> ranges;

        while (const char ch = *str++)
        {
            if (ch == '\n')
            {
                const int line_idx_next = (int)(str - input.base);
                ranges.push_back({ line, line_idx_start, line_idx_next - 1 });
                line = line + 1;
                line_idx_start = line_idx_next;
            }
        }

        ranges.push_back({ line, line_idx_start, (int)(str - input.base - 1) });
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
        const char* head = input.head();

        for (const auto& [str, kw] : keywords)
        {
            if (std::strncmp(head, str.data(), str.size()) != 0) continue;
            match->length = (int)str.size();
            match->token.type = Token::Keyword;
            match->token.keyword = kw;
            return true;
        }

        return false;
    }

    bool tokenize_identifier(const LexerInput& input, LexerMatch* match)
    {
        int distance;

        for (distance = 0; const char ch = peek(input, distance); ++distance)
        {
            if (!std::isalpha(ch) && (!std::isdigit(ch) || !distance) && ch != '_') break;
        }

        if (distance)
        {
            match->length = distance;
            match->token.type = Token::Identifier;
            match->token.identifier.str = { input.offset, match->length };
            return true;
        }

        return false;
    }

    bool tokenize_literal(const LexerInput& input, LexerMatch* match)
    {
        const char first_ch = read(input);

        const bool is_string = first_ch == '\"';
        const bool is_number = std::isdigit(first_ch);
        const bool is_decimal = first_ch == '.';
        const bool is_sign = first_ch == '+' || first_ch == '-';

        if (is_string)
        {
            // We will scan until we find a string close which is not escaped, or a newline.
            LexerInput temp_input = input;
            ++temp_input.offset;

            while (read(temp_input))
            {
                const char matches[] = { '\"', '\n' };
                temp_input.offset += skip_until(temp_input.head(), matches);

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
            match->token.literal.type = Literal::String;
            match->token.literal.str = { input.offset + 1, match->length - 2 };
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
                else if (std::isdigit(ch) || (is_hex && std::isxdigit(ch)))
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
                match->token.literal.type = Literal::Float;
                match->token.literal.flt = (float)parsed;
            }
            // Otherwise, we're an int.
            else
            {
                const int base = is_hex ? 16 : 10; // note: nwscript does not support octal with leading 0
                const long int parsed = std::strtol(input.head(), &parsed_head, base);
                match->token.literal.type = Literal::Int;
                match->token.literal.integer = (int)parsed;
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
        const char* head = input.head();

        // Unlike keywords, we are extremely likely to find multiple matches here: for example,
        // "+=" could match + or +=, and ">>=" could match ">" or ">>" or ">>=".
        // Thankfully, there is an effective way for us to get the upper bound of possible matches:
        // the length of the longest punctuator!

        static constexpr size_t longest_punctuator =
            std::max_element(std::begin(punctuators), std::end(punctuators),
            [] (const auto& lhs, const auto& rhs)
        {
            return lhs.first.size() < rhs.first.size();
        })->first.size();

        int match_count = 0;
        LexerMatch matches[longest_punctuator];

        for (const auto& [str, punc] : punctuators)
        {
            NWTREES_ASSERT(match_count <= longest_punctuator);

            if (std::strncmp(head, str.data(), str.size()) != 0) continue;
            matches[match_count].length = (int)str.size();
            matches[match_count].token.type = Token::Punctuator;
            matches[match_count].token.punctuator = punc;
            ++match_count;
        }

        if (!match_count) return false;

        // As with the calling function, we sort by token length and select the longest token.
        std::sort(matches, matches + match_count, &cmp);
        *match = std::move(matches[0]);

        return true;
    }
}

LexerOutput nwtrees::lexer(const char* data)
{
    LexerOutput output;
    LexerInput input = { data, 0 };

    output.debug_ranges = make_debug_ranges(input, output);

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

        const DebugRange& range = find_debug_range(output.debug_ranges, input);

        if (!match_count)
        {
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

        std::stable_sort(matches, matches + match_count, &cmp);
        LexerMatch& selected_match = matches[0];

        // -- Populate the debug data using the range for this line.
        // Note: We assume that every token will take, at most, one line.

        const int index_start = input.offset;
        const int index_end = input.offset + selected_match.length;

        selected_match.token.debug.line = range.line;
        selected_match.token.debug.column_start = index_start - range.index_start;
        selected_match.token.debug.column_end = index_end - range.index_start;

        // -- Step stream forward, past the matched token length.

        input.offset += selected_match.length;

        // -- Commit token.

        output.tokens.push_back(std::move(matches[0].token));
    }

    // -- For tokens that need name buffers, prepare the buffer and update the name entry.

    for (Token& token : output.tokens)
    {
        const bool is_identifier = token.type == Token::Identifier;
        const bool is_str_literal = token.type == Token::Literal && token.literal.type == Literal::String;

        if (is_identifier || is_str_literal)
        {
            NameBufferEntry* entry = is_identifier ? &token.identifier.str : &token.literal.str;
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