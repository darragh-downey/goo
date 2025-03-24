const std = @import("std");

/// Check if a character is a standard ASCII letter or underscore
pub fn isAsciiLetter(ch: u8) bool {
    return ('a' <= ch and ch <= 'z') or
        ('A' <= ch and ch <= 'Z') or
        ch == '_';
}

/// Check if a character is a letter
/// Note: This currently only handles ASCII characters
/// For full Unicode support, a proper Unicode library would be needed
pub fn isLetter(ch: u8) bool {
    return isAsciiLetter(ch);
    // Future enhancement: Add Unicode letter checks
}

/// Check if a character is a digit
/// Note: This currently only handles ASCII digits
pub fn isDigit(ch: u8) bool {
    return '0' <= ch and ch <= '9';
    // Future enhancement: Add Unicode digit checks
}

/// Check if a character is an alphanumeric character or underscore
pub fn isAlphanumeric(ch: u8) bool {
    return isLetter(ch) or isDigit(ch);
}

/// Check if a character is whitespace
pub fn isWhitespace(ch: u8) bool {
    // Handle common whitespace characters
    return ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r' or
        ch == 0x0B or ch == 0x0C or ch == 0x85 or ch == 0xA0;
    // Future enhancement: Add Unicode whitespace category checks
}

/// Check if a character is hexadecimal digit
pub fn isHexDigit(ch: u8) bool {
    return isDigit(ch) or
        ('a' <= ch and ch <= 'f') or
        ('A' <= ch and ch <= 'F');
}

/// Check if a character is an octal digit
pub fn isOctalDigit(ch: u8) bool {
    return '0' <= ch and ch <= '7';
}

/// Check if a character is a binary digit
pub fn isBinaryDigit(ch: u8) bool {
    return ch == '0' or ch == '1';
}

/// Get the numeric value of a hexadecimal digit
pub fn hexValue(ch: u8) u8 {
    if (isDigit(ch)) {
        return ch - '0';
    } else if ('a' <= ch and ch <= 'f') {
        return ch - 'a' + 10;
    } else if ('A' <= ch and ch <= 'F') {
        return ch - 'A' + 10;
    } else {
        return 0;
    }
}

/// Check if a character is a valid identifier character
/// Currently only handles ASCII, but extensible for Unicode
pub fn isIdentChar(ch: u8) bool {
    return isAlphanumeric(ch);
}

/// Convert a Unicode code point to UTF-8 bytes
pub fn unicodeToUtf8(codepoint: u21) ![4]u8 {
    var buffer: [4]u8 = undefined;
    _ = try std.unicode.utf8Encode(codepoint, &buffer);
    return buffer;
}

/// Get the length of a UTF-8 character based on first byte
pub fn utf8CharLen(first_byte: u8) u3 {
    if (first_byte & 0x80 == 0) return 1;
    if (first_byte & 0xE0 == 0xC0) return 2;
    if (first_byte & 0xF0 == 0xE0) return 3;
    if (first_byte & 0xF8 == 0xF0) return 4;
    return 1; // Invalid UTF-8 sequence, treat as a single byte
}
