const std = @import("std"); const Union = union(enum) { a: i32, b: f32 }; pub fn main() void { const x = Union{ .a = 42 }; std.debug.print("{any}\n", .{x}); }
