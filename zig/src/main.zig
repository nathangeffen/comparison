const std = @import("std");
const ArrayList = std.ArrayList;
const expect = std.testing.expect;
const eql = std.mem.eql;
const test_allocator = std.testing.allocator;

const State = enum {
    SUSCEPTIBLE,
    INFECTIOUS,
    RECOVERED,
    VACCINATED,
    DEAD,
    pub fn format(
        self: State,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;

        const res = switch (self) {
            State.SUSCEPTIBLE => "S",
            State.INFECTIOUS => "I",
            State.RECOVERED => "R",
            State.VACCINATED => "V",
            State.DEAD => "D",
        };
        try writer.print("{s}", .{res});
    }
};

test "State fmt" {
    const state: State = State.SUSCEPTIBLE;
    const str = try std.fmt.allocPrint(
        test_allocator,
        "{s}",
        .{state},
    );
    defer test_allocator.free(str);

    try expect(eql(
        u8,
        str,
        "S",
    ));
}

const Parameters = struct {
    agents: i32 = 10000,
    iterations: i32 = 10,
    infections: i32 = 10,
    encounters: i32 = 100,
    growth: f64 = 0.0001,
    death_risk_susceptible: f64 = 0.0001,
    death_risk_infectious: f64 = 0.001,
    recovery_risk: f64 = 0.01,
    vaccination_risk: f64 = 0.001,
    regression_risk: f64 = 0.0003,
    output_agents: i32 = 0,
    agent_filename: []const u8 = "agents.csv",
};

test "Parameters" {
    const p = Parameters{
        .agents = 100,
        .agent_filename = "Bla bla",
    };
    try expect(eql(
        u8,
        p.agent_filename,
        "Bla bla",
    ));
    try expect(p.agents == 100);
    try expect(p.regression_risk == 0.0003);
}

const Agent = struct {
    identity: i32,
    state: State,
}

const Statistics = struct {
    susceptible: i32 = 0,
    infectious: i32 = 0,
    recovered: i32 = 0,
    vaccinated: i32 = 0,
    dead: i32 = 0,
    total_infections: i32 = 0,
    infection_deaths: i32 = 0,
}

const Simulation = struct {
    identity: i32,
    agents: ArrayList,
    parameters: Parameters,
    total_infections: i32,
    infection_deaths: i32,

    pub fn new(identity: i32,
               parameters: Parameters) !Simulation {

    }
}

pub fn main() !void {
    // Prints to stderr (it's a shortcut based on `std.io.getStdErr()`)
    const s: State = State.SUSCEPTIBLE;
    std.debug.print("All your {} are belong to us.\n", .{s});

    // stdout is for the actual output of your application, for example if you
    // are implementing gzip, then only the compressed bytes should be sent to
    // stdout, not any debugging messages.
    const stdout_file = std.io.getStdOut().writer();
    var bw = std.io.bufferedWriter(stdout_file);
    const stdout = bw.writer();

    try stdout.print("Run `zig build test` to run the tests.\n", .{});

    try bw.flush(); // don't forget to flush!
}

test "simple test" {
    var list = std.ArrayList(i32).init(std.testing.allocator);
    defer list.deinit(); // try commenting this out and see if zig detects the memory leak!
    try list.append(42);
    try std.testing.expectEqual(@as(i32, 42), list.pop());
}
