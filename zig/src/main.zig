// Copyright 2024 Nathan Geffen

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! This program implements a simple agent based model for the purpose of
//! comparing programming languages.

const clap = @import("./zig-clap/clap.zig");
const abm = @import("./abm.zig");
const std = @import("std");
const ArrayList = std.ArrayList;
const expect = std.testing.expect;
const eql = std.mem.eql;
const test_allocator = std.testing.allocator;
const Allocator = std.mem.Allocator;
//const RndGen = std.rand.DefaultPrng;

/// Runs one simulation. Called within the thread pool so has to be thread
/// safe. May not return an error apparently.
fn one_simulation(identity: usize, parameters: abm.Parameters) void {
    var seed: u64 = undefined;
    std.posix.getrandom(std.mem.asBytes(&seed)) catch {
        std.debug.print("Simulation {d} failed to get a random seed.\n", .{identity});
        return;
    };
    abm.rnd = abm.RndGen.init(seed);
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    const gpa = general_purpose_allocator.allocator();

    var simulation = abm.Simulation.init(gpa, identity, parameters) catch {
        std.debug.print("Simulation {d} failed to initialize.\n", .{identity});
        return;
    };
    defer simulation.deinit();
    simulation.simulate() catch {
        std.debug.print("Simulation {d} failed to run.\n", .{identity});
    };
}

/// Manages command line arguments.
fn process_command_line() !abm.Parameters {
    var parameters = abm.Parameters{};
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();

    const params = comptime clap.parseParamsComptime(
        \\-s, --simulations <usize>   Number of simulations.
        \\    --identity    <usize>   Id number of simulation
        \\-a, --agents      <usize>   Number of initial agents
        \\-i, --iterations <usize> Number of iterations per simulation
        \\-e, --encounters <usize> Number of encounters for infections
        \\-g, --growth     <f64>   Growth rate of agents per iteration
        \\--death_risk_susceptible <f64> Death rate of susceptible agents
        \\--death_risk_infectious <f64> Death rate of infectious agents
        \\-r, --recovery_risk <f64> Risk infectious to recover
        \\-v, --vaccination_risk <f64> Risk susceptible to vaccinated
        \\--regression_risk <f64> Risk vaccinated/recovered to susceptible
        \\--infection_method <u8> Infection method to use (0=both, 1=one, 2=two)
        \\--output_agents <usize> Iteration frequency to write agents
        \\--agent_filename <str> Agent output file name
        \\-h, --help                  Display help and exit
        \\
    );
    var diag = clap.Diagnostic{};
    var res = clap.parse(clap.Help, &params, clap.parsers.default, .{
        .diagnostic = &diag,
        .allocator = gpa.allocator(),
    }) catch |err| {
        diag.report(std.io.getStdErr().writer(), err) catch {};
        return err;
    };
    defer res.deinit();

    if (res.args.help != 0)
        try clap.help(std.io.getStdErr().writer(), clap.Help, &params, .{});
    if (res.args.simulations) |s|
        parameters.simulations = s;
    if (res.args.identity) |i|
        parameters.identity = i;
    if (res.args.agents) |a|
        parameters.agents = a;
    if (res.args.iterations) |i|
        parameters.iterations = i;
    if (res.args.encounters) |e|
        parameters.encounters = e;
    if (res.args.growth) |g|
        parameters.growth = g;
    if (res.args.death_risk_susceptible) |d|
        parameters.death_risk_susceptible = d;
    if (res.args.death_risk_infectious) |d|
        parameters.death_risk_infectious = d;
    if (res.args.recovery_risk) |r|
        parameters.recovery_risk = r;
    if (res.args.output_agents) |o|
        parameters.output_agents = o;
    if (res.args.infection_method) |i|
        parameters.infection_method = i;
    const allocator = std.heap.page_allocator;
    if (res.args.agent_filename) |f| {
        parameters.agent_filename = try std.fmt.allocPrint(allocator, "{s}", .{f});
    } else {
        parameters.agent_filename = try std.fmt.allocPrint(allocator, "agents.csv", .{});
    }
    return parameters;
}

/// Processes parameters, sets up thread pool and invokes the execution of the
/// simulations.
pub fn main() !void {
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    const gpa = general_purpose_allocator.allocator();
    defer _ = general_purpose_allocator.deinit();
    const parameters = process_command_line() catch return;

    const WaitGroup = std.Thread.WaitGroup;
    var wait_group: WaitGroup = undefined;
    wait_group.reset();
    const threads = try std.Thread.getCpuCount();
    const Pool = std.Thread.Pool;
    var thread_pool: Pool = undefined;

    try thread_pool.init(Pool.Options{
        .allocator = gpa,
        .n_jobs = @intCast(threads),
    });

    defer thread_pool.deinit();

    for (0..parameters.simulations) |i| {
        try thread_pool.spawn(one_simulation, .{ i, parameters });
    }
    thread_pool.waitAndWork(&wait_group);
}
