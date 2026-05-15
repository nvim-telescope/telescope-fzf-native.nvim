const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseFast });

    const fzf_mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });

    fzf_mod.addCSourceFile(.{
        .file = b.path("src/fzf.c"),
        .language = .c,
        .flags = &.{
            "-std=gnu99",
            "-Wall",
            "-fpic",
        },
    });

    const fzf_lib = b.addLibrary(.{
        .name = "libfzf",
        .root_module = fzf_mod,
        .linkage = .dynamic,
        .use_llvm = true,
        .use_lld = true,
    });

    const install = b.addInstallArtifact(fzf_lib, .{
        .dest_dir = .{ .override = .{ .custom = "../build" } },
        .pdb_dir = .disabled,
        .implib_dir = .disabled,
    });
    b.default_step.dependOn(&install.step);
}
