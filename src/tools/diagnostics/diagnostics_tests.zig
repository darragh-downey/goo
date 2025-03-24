const std = @import("std");
const testing = std.testing;
const c = @cImport({
    @cInclude("diagnostics.h");
    @cInclude("error_catalog.h");
    @cInclude("source_highlight.h");
    @cInclude("diagnostics_module.h");
});

// Test fixtures
const test_source =
    \\fn test() {
    \\    let x = 5
    \\    return x;
    \\}
    \\
;

test "diagnostics creation and emission" {
    // Create a diagnostic context
    const ctx = c.goo_diag_context_new();
    defer c.goo_diag_context_free(ctx);
    
    try testing.expect(ctx != null);
    try testing.expectEqual(@as(c_int, 0), c.goo_diag_error_count(ctx));
    
    // Create and emit a diagnostic
    const diag = c.goo_diag_new(
        c.GOO_DIAG_ERROR,
        "test.goo",
        2, 12, 1,
        "missing semicolon after expression"
    );
    
    try testing.expect(diag != null);
    c.goo_diag_emit(ctx, diag);
    
    // Check error count
    try testing.expectEqual(@as(c_int, 1), c.goo_diag_error_count(ctx));
}

test "error catalog functionality" {
    // Initialize error catalog
    try testing.expect(c.goo_error_catalog_init());
    defer c.goo_error_catalog_cleanup();
    
    // Check catalog
    const count = c.goo_error_catalog_count();
    try testing.expect(count > 0);
    
    // Look up a known code
    const entry = c.goo_error_catalog_lookup("E0001");
    try testing.expect(entry != null);
    
    // Look up an unknown code
    const unknown_entry = c.goo_error_catalog_lookup("EXXXX");
    try testing.expect(unknown_entry == null);
    
    // Check category string
    const cat_str = c.goo_error_category_to_string(c.GOO_ERROR_CAT_SYNTAX);
    try testing.expectEqualStrings("Syntax", std.mem.span(cat_str));
}

test "source highlighting" {
    // Create a diagnostic
    const diag = c.goo_diag_new(
        c.GOO_DIAG_ERROR,
        "test.goo",
        2, 12, 1,
        "missing semicolon after expression"
    );
    defer c.goo_diag_free(diag);
    
    try testing.expect(diag != null);
    
    // Get default highlight options
    const options = c.goo_highlight_options_default();
    
    // Buffer for output
    var buffer: [2048]u8 = undefined;
    
    // Highlight the source
    const length = c.goo_highlight_source(
        diag,
        test_source,
        &options,
        &buffer,
        buffer.len
    );
    
    try testing.expect(length > 0);
}

test "diagnostics module" {
    // Create default config
    const config = c.goo_diagnostics_default_config();
    
    // Initialize module
    const ctx = c.goo_diagnostics_init(&config);
    defer c.goo_diagnostics_cleanup(ctx);
    
    try testing.expect(ctx != null);
    
    // Report an error
    c.goo_diagnostics_report_error(
        ctx,
        "test.goo",
        test_source,
        2, 12, 1,
        "missing semicolon after expression"
    );
    
    // Check error count
    try testing.expectEqual(@as(c_int, 1), c.goo_diag_error_count(ctx));
    
    // Check abort condition
    try testing.expect(c.goo_diagnostics_should_abort(ctx));
} 