const std = @import("std");
const token = @import("token.zig");
const char_utils = @import("character_utils.zig");
const keywords = @import("keywords.zig");

const TokenType = token.TokenType;
const Token = token.Token;
const TokenValue = token.TokenValue;

/// Result of reading an identifier
pub const IdentifierResult = struct {
    token: Token,
    end_pos: usize,
};

/// Read an identifier from the input stream
pub fn readIdentifier(source: []const u8, start_pos: usize, start_line: usize, start_column: usize) IdentifierResult {
    var current_pos = start_pos;

    // Read until we hit a non-identifier character
    while (current_pos < source.len and char_utils.isIdentChar(source[current_pos])) {
        current_pos += 1;
    }

    const identifier = source[start_pos..current_pos];
    const token_type = keywords.lookupIdentifier(identifier);

    // CRITICAL FIX: We must NOT use the original source string directly
    // Instead, return a token that indicates the slice is from the source
    // The lexer will handle making a copy of the string if needed

    return .{
        .token = Token.init(token_type, start_line, start_column, TokenValue.string(identifier)),
        .end_pos = current_pos,
    };
}

/// Result of reading a number
pub const NumberResult = struct {
    token: Token,
    end_pos: usize,
    error_message: ?[]const u8 = null,
};

/// Read a number from the input stream
pub fn readNumber(source: []const u8, start_pos: usize, start_line: usize, start_column: usize) !NumberResult {
    var current_pos = start_pos;
    var is_float = false;
    var is_hex = false;
    var is_octal = false;
    var is_binary = false;
    var error_message: ?[]const u8 = null;
    var max_digit_count: usize = std.math.maxInt(usize);

    // Check for hex, octal, or binary prefix
    if (current_pos + 1 < source.len and source[current_pos] == '0') {
        if (source[current_pos + 1] == 'x' or source[current_pos + 1] == 'X') {
            is_hex = true;
            current_pos += 2; // Skip "0x"
            max_digit_count = 16; // Max 16 hex digits for 64-bit int
        } else if (source[current_pos + 1] == 'o' or source[current_pos + 1] == 'O') {
            is_octal = true;
            current_pos += 2; // Skip "0o"
            max_digit_count = 22; // Max 22 octal digits for 64-bit int
        } else if (source[current_pos + 1] == 'b' or source[current_pos + 1] == 'B') {
            is_binary = true;
            current_pos += 2; // Skip "0b"
            max_digit_count = 64; // Max 64 binary digits for 64-bit int
        }
    }

    const digit_start_pos = current_pos;
    var digit_count: usize = 0;

    // Read digits based on the number type
    if (is_hex) {
        if (current_pos >= source.len or !char_utils.isHexDigit(source[current_pos])) {
            error_message = "Hexadecimal literal requires at least one hex digit";
        }
        while (current_pos < source.len and char_utils.isHexDigit(source[current_pos]) and digit_count < max_digit_count) {
            current_pos += 1;
            digit_count += 1;
        }
        // Check for overflow
        if (current_pos < source.len and char_utils.isHexDigit(source[current_pos])) {
            error_message = "Hexadecimal literal too large";
            // Continue reading digits but we'll report as error
            while (current_pos < source.len and char_utils.isHexDigit(source[current_pos])) {
                current_pos += 1;
            }
        }
    } else if (is_octal) {
        if (current_pos >= source.len or !char_utils.isOctalDigit(source[current_pos])) {
            error_message = "Octal literal requires at least one octal digit";
        }
        while (current_pos < source.len and char_utils.isOctalDigit(source[current_pos]) and digit_count < max_digit_count) {
            current_pos += 1;
            digit_count += 1;
        }
        // Check for overflow
        if (current_pos < source.len and char_utils.isOctalDigit(source[current_pos])) {
            error_message = "Octal literal too large";
            // Continue reading digits but we'll report as error
            while (current_pos < source.len and char_utils.isOctalDigit(source[current_pos])) {
                current_pos += 1;
            }
        }
    } else if (is_binary) {
        if (current_pos >= source.len or !char_utils.isBinaryDigit(source[current_pos])) {
            error_message = "Binary literal requires at least one binary digit";
        }
        while (current_pos < source.len and char_utils.isBinaryDigit(source[current_pos]) and digit_count < max_digit_count) {
            current_pos += 1;
            digit_count += 1;
        }
        // Check for overflow
        if (current_pos < source.len and char_utils.isBinaryDigit(source[current_pos])) {
            error_message = "Binary literal too large";
            // Continue reading digits but we'll report as error
            while (current_pos < source.len and char_utils.isBinaryDigit(source[current_pos])) {
                current_pos += 1;
            }
        }
    } else {
        // Decimal number
        while (current_pos < source.len and char_utils.isDigit(source[current_pos]) and digit_count < 19) { // Max 19 decimal digits for 64-bit
            current_pos += 1;
            digit_count += 1;
        }

        // Check for overflow of decimal integer
        var possible_overflow = false;
        if (current_pos < source.len and char_utils.isDigit(source[current_pos])) {
            possible_overflow = true;
            while (current_pos < source.len and char_utils.isDigit(source[current_pos])) {
                current_pos += 1;
            }
        }

        // Check for decimal point
        if (current_pos < source.len and source[current_pos] == '.') {
            is_float = true;
            current_pos += 1;

            // Reset digit count for fractional part
            digit_count = 0;

            // Read fraction digits
            while (current_pos < source.len and char_utils.isDigit(source[current_pos])) {
                current_pos += 1;
                digit_count += 1;
            }

            // Integer overflow is not an issue if it's a float
            possible_overflow = false;
        }

        // Check for exponent
        if (current_pos < source.len and (source[current_pos] == 'e' or source[current_pos] == 'E')) {
            is_float = true;
            current_pos += 1;

            // Optional sign
            if (current_pos < source.len and (source[current_pos] == '+' or source[current_pos] == '-')) {
                current_pos += 1;
            }

            // Must have at least one digit
            if (current_pos >= source.len or !char_utils.isDigit(source[current_pos])) {
                error_message = "Exponent requires at least one digit";
            }

            // Read exponent digits
            digit_count = 0;
            while (current_pos < source.len and char_utils.isDigit(source[current_pos])) {
                current_pos += 1;
                digit_count += 1;
            }

            // Integer overflow is not an issue if it's a float
            possible_overflow = false;
        }

        if (possible_overflow) {
            error_message = "Decimal integer literal too large";
        }
    }

    const number_text = source[digit_start_pos..current_pos];
    var result_token: Token = undefined;

    if (is_float) {
        // Parse as float
        const float_val = std.fmt.parseFloat(f64, number_text) catch {
            error_message = "Invalid floating-point number";
            // Return a default token with error value
            return NumberResult{
                .token = Token.init(.ERROR, start_line, start_column, TokenValue.string(number_text)),
                .end_pos = current_pos,
                .error_message = error_message,
            };
        };
        result_token = Token.init(.FLOAT_LITERAL, start_line, start_column, TokenValue.float(float_val));
    } else {
        // Parse as integer
        if (error_message != null) {
            // We already detected an error (like overflow), return error token
            return NumberResult{
                .token = Token.init(.ERROR, start_line, start_column, TokenValue.string(number_text)),
                .end_pos = current_pos,
                .error_message = error_message,
            };
        }

        const int_val = if (is_hex)
            std.fmt.parseInt(i64, number_text[2..], 16) catch {
                error_message = "Invalid hexadecimal number";
                // Return a default token with error value
                return NumberResult{
                    .token = Token.init(.ERROR, start_line, start_column, TokenValue.string(number_text)),
                    .end_pos = current_pos,
                    .error_message = error_message,
                };
            }
        else if (is_octal)
            std.fmt.parseInt(i64, number_text[2..], 8) catch {
                error_message = "Invalid octal number";
                // Return a default token with error value
                return NumberResult{
                    .token = Token.init(.ERROR, start_line, start_column, TokenValue.string(number_text)),
                    .end_pos = current_pos,
                    .error_message = error_message,
                };
            }
        else if (is_binary)
            std.fmt.parseInt(i64, number_text[2..], 2) catch {
                error_message = "Invalid binary number";
                // Return a default token with error value
                return NumberResult{
                    .token = Token.init(.ERROR, start_line, start_column, TokenValue.string(number_text)),
                    .end_pos = current_pos,
                    .error_message = error_message,
                };
            }
        else
            std.fmt.parseInt(i64, number_text, 10) catch {
                error_message = "Invalid decimal number";
                // Return a default token with error value
                return NumberResult{
                    .token = Token.init(.ERROR, start_line, start_column, TokenValue.string(number_text)),
                    .end_pos = current_pos,
                    .error_message = error_message,
                };
            };

        result_token = Token.init(.INT_LITERAL, start_line, start_column, TokenValue.integer(int_val));
    }

    return NumberResult{
        .token = result_token,
        .end_pos = current_pos,
        .error_message = error_message,
    };
}

/// Result of reading a string
pub const StringResult = struct {
    token: Token,
    end_pos: usize,
    new_line: usize,
    new_column: usize,
    error_message: ?[]const u8 = null,
};

/// Read a string literal from the input stream
pub fn readString(source: []const u8, start_pos: usize, start_line: usize, start_column: usize, allocator: std.mem.Allocator) !StringResult {
    var buffer = std.ArrayList(u8).init(allocator);
    errdefer buffer.deinit(); // Always ensure buffer is freed in error cases

    var current_pos = start_pos + 1; // Skip opening quote
    var current_line = start_line;
    var current_column = start_column + 1;
    var error_message: ?[]const u8 = null;
    var found_closing_quote = false;

    while (current_pos < source.len) {
        const ch = source[current_pos];

        if (ch == '"') {
            // Found closing quote
            found_closing_quote = true;
            current_pos += 1;
            current_column += 1;
            break;
        } else if (ch == '\\' and current_pos + 1 < source.len) {
            // Handle escape sequences
            current_pos += 1;
            current_column += 1;
            const escaped_char = source[current_pos];

            switch (escaped_char) {
                'n' => try buffer.append('\n'),
                'r' => try buffer.append('\r'),
                't' => try buffer.append('\t'),
                '\\' => try buffer.append('\\'),
                '"' => try buffer.append('"'),
                '\'' => try buffer.append('\''),
                '0' => try buffer.append(0),
                'a' => try buffer.append('\x07'), // Bell
                'b' => try buffer.append('\x08'), // Backspace
                'v' => try buffer.append('\x0B'), // Vertical tab
                'f' => try buffer.append('\x0C'), // Form feed
                'e' => try buffer.append('\x1B'), // Escape
                'u' => {
                    // Handle \u1234 Unicode escape sequences (4 hex digits)
                    if (current_pos + 4 >= source.len) {
                        error_message = "Incomplete Unicode escape sequence \\u";
                        break;
                    }

                    const unicode_pos = current_pos + 1;
                    const unicode_end = unicode_pos + 4;
                    if (unicode_end > source.len) {
                        error_message = "Incomplete Unicode escape sequence \\u";
                        break;
                    }

                    const hex_digits = source[unicode_pos..unicode_end];
                    const codepoint = std.fmt.parseInt(u21, hex_digits, 16) catch {
                        error_message = "Invalid Unicode escape sequence";
                        break;
                    };

                    // Check if this is a valid Unicode code point
                    if (codepoint > 0x10FFFF) {
                        error_message = "Unicode code point out of range";
                        break;
                    }

                    var utf8_buf: [4]u8 = undefined;
                    const len = std.unicode.utf8Encode(codepoint, &utf8_buf) catch {
                        error_message = "Invalid Unicode codepoint";
                        break;
                    };
                    try buffer.appendSlice(utf8_buf[0..len]);

                    current_pos = unicode_end - 1; // -1 because we'll increment at the end of the loop
                    current_column += 4; // For the 4 hex digits
                },
                'U' => {
                    // Handle \U00012345 Unicode escape sequences (8 hex digits)
                    if (current_pos + 8 >= source.len) {
                        error_message = "Incomplete Unicode escape sequence \\U";
                        break;
                    }

                    const unicode_pos = current_pos + 1;
                    const unicode_end = unicode_pos + 8;
                    if (unicode_end > source.len) {
                        error_message = "Incomplete Unicode escape sequence \\U";
                        break;
                    }

                    const hex_digits = source[unicode_pos..unicode_end];
                    const codepoint = std.fmt.parseInt(u21, hex_digits, 16) catch {
                        error_message = "Invalid Unicode escape sequence";
                        break;
                    };

                    // Check if this is a valid Unicode code point
                    if (codepoint > 0x10FFFF) {
                        error_message = "Unicode code point out of range";
                        break;
                    }

                    var utf8_buf: [4]u8 = undefined;
                    const len = std.unicode.utf8Encode(codepoint, &utf8_buf) catch {
                        error_message = "Invalid Unicode codepoint";
                        break;
                    };
                    try buffer.appendSlice(utf8_buf[0..len]);

                    current_pos = unicode_end - 1; // -1 because we'll increment at the end of the loop
                    current_column += 8; // For the 8 hex digits
                },
                'x' => {
                    // Hexadecimal escape sequence like \x7F (2 hex digits)
                    if (current_pos + 2 >= source.len) {
                        error_message = "Incomplete hexadecimal escape sequence \\x";
                        break;
                    }

                    const hex_pos = current_pos + 1;
                    const hex_end = hex_pos + 2;
                    if (hex_end > source.len) {
                        error_message = "Incomplete hexadecimal escape sequence \\x";
                        break;
                    }

                    const hex_digits = source[hex_pos..hex_end];
                    const byte_val = std.fmt.parseInt(u8, hex_digits, 16) catch {
                        error_message = "Invalid hexadecimal escape sequence";
                        break;
                    };

                    try buffer.append(byte_val);
                    current_pos = hex_end - 1; // -1 because we'll increment at the end of the loop
                    current_column += 2; // For the 2 hex digits
                },
                else => {
                    // Allow \<newline> to continue a string to the next line without including the newline
                    if (escaped_char == '\n') {
                        current_line += 1;
                        current_column = 1;
                    } else {
                        error_message = "Invalid escape sequence";
                        try buffer.append(escaped_char); // Include the character anyway for better error recovery
                    }
                },
            }
        } else if (ch == '\n') {
            // Handle newlines in strings
            try buffer.append(ch);
            current_line += 1;
            current_column = 1;
        } else {
            try buffer.append(ch);
            current_column += 1;
        }

        current_pos += 1;
    }

    // Check if we exited the loop without finding a closing quote
    if (!found_closing_quote) {
        error_message = "Unterminated string literal";
    }

    // Create a copy of the string content - this will be owned by the token
    const string_content = try buffer.toOwnedSlice();

    // Create the token based on whether we have an error
    var result_token: Token = undefined;
    if (error_message == null) {
        result_token = Token.init(TokenType.STRING_LITERAL, start_line, start_column, TokenValue.string(string_content));
    } else {
        result_token = Token.init(TokenType.ERROR, start_line, start_column, TokenValue.string(string_content));
    }

    // Return result with appropriate metadata
    return StringResult{
        .token = result_token,
        .end_pos = current_pos,
        .new_line = current_line,
        .new_column = current_column,
        .error_message = error_message,
    };
}
