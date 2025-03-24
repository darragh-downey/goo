const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // LSP Server
    const lsp_server = b.addExecutable(.{
        .name = "goo-lsp",
        .root_source_file = b.path("lsp/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(lsp_server);

    // Add any other tools here
    // ...

    // Run command
    const run_cmd = b.addRunArtifact(lsp_server);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the LSP server");
    run_step.dependOn(&run_cmd.step);
}
