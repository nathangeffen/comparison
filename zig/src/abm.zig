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

//! This package implements a simple agent-based model simulation engine
//! for the purpose of comparing programming environments.

const std = @import("std");
const ArrayList = std.ArrayList;
const expect = std.testing.expect;
const eql = std.mem.eql;
const test_allocator = std.testing.allocator;
const Allocator = std.mem.Allocator;
pub const RndGen = std.rand.DefaultPrng;
pub threadlocal var rnd: RndGen = undefined;
const assert = std.debug.assert;

/// Represents the possible states an agent can be in.
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

/// These are the parameters for a simulation.
pub const Parameters = struct {
    simulations: usize = 20,
    identity: usize = 0,
    agents: usize = 10_000,
    iterations: usize = 365 * 4,
    infections: usize = 10,
    encounters: usize = 100,
    growth: f64 = 0.0001,
    death_risk_susceptible: f64 = 0.0001,
    death_risk_infectious: f64 = 0.001,
    recovery_risk: f64 = 0.01,
    vaccination_risk: f64 = 0.001,
    regression_risk: f64 = 0.0003,
    infection_method: u8 = 0,
    output_agents: usize = 0,
    agent_filename: []u8 = undefined,
};

/// This typically represents a single person in a simulation.
const Agent = struct {
    identity: usize,
    state: State,
};

/// This is used to represent a snapshot of stats for a simulation.
const Statistics = struct {
    susceptible: usize = 0,
    infectious: usize = 0,
    recovered: usize = 0,
    vaccinated: usize = 0,
    dead: usize = 0,
    total_infections: usize = 0,
    infection_deaths: usize = 0,
};

/// This is the data structure for the simulation engine.
pub const Simulation = struct {
    /// Unique identifier of the simulation.
    identity: usize,
    /// Array of agents, typically each one representing a person
    agents: ArrayList(Agent),
    /// Parameters of the simulation
    parameters: Parameters,
    /// Tracks total number of times infections occur including the initial infections.
    total_infections: usize,
    /// Tracks the number of deaths of infected agents
    infection_deaths: usize,
    allocator: Allocator,

    /// Creates a new simulation
    pub fn init(allocator: Allocator, identity: usize, parameters: Parameters) !Simulation {
        var simulation = Simulation{
            .identity = identity,
            .agents = ArrayList(Agent).init(allocator),
            .parameters = parameters,
            .total_infections = 0,
            .infection_deaths = 0,
            .allocator = allocator,
        };
        for (0..@intCast(parameters.agents)) |i| {
            try simulation.agents.append(Agent{
                .identity = @intCast(i),
                .state = State.SUSCEPTIBLE,
            });
        }
        std.Random.shuffle(rnd.random(), Agent, simulation.agents.items);
        for (0..@intCast(parameters.infections)) |i| {
            simulation.agents.items[i].state = State.INFECTIOUS;
        }
        return simulation;
    }

    /// Counts number of agents with given state
    fn count_if_state(self: Simulation, state: State) usize {
        var total: usize = 0;
        for (self.agents.items) |value| {
            if (value.state == state) {
                total += 1;
            }
        }
        return total;
    }

    /// Counts number of agents not in the given state
    fn count_if_not_state(self: Simulation, state: State) usize {
        const n = count_if_state(self, state);
        return self.agents.items.len - n;
    }

    /// Simulation event that grows the agent population
    pub fn grow(self: *Simulation) !void {
        const num_agents: usize = self.count_if_not_state(State.DEAD);
        const f: f64 = @floatFromInt(num_agents);
        const new_agents: usize = @intFromFloat(std.math.round(self.parameters.growth * f));
        const n: usize = self.agents.items.len + new_agents;
        for (self.agents.items.len..n) |identity| {
            try self.*.agents.append(Agent{
                .identity = @intCast(identity),
                .state = State.SUSCEPTIBLE,
            });
        }
    }

    /// Simulation event that infects agents (1st of 2 methods implemented)
    pub fn infect_method_one(self: *Simulation) void {
        for (0..@intCast(self.parameters.encounters)) |_| {
            const ind1 = rnd.random().uintLessThan(usize, self.agents.items.len);
            const ind2 = rnd.random().uintLessThan(usize, self.agents.items.len);
            if (self.agents.items[ind1].state == State.SUSCEPTIBLE and
                self.agents.items[ind2].state == State.INFECTIOUS)
            {
                self.*.agents.items[ind1].state = State.INFECTIOUS;
                self.*.total_infections += 1;
            } else if (self.agents.items[ind2].state == State.SUSCEPTIBLE and
                self.agents.items[ind1].state == State.INFECTIOUS)
            {
                self.*.agents.items[ind2].state = State.INFECTIOUS;
                self.total_infections += 1;
            }
        }
    }

    /// Returns a vector of indices into an agents vector
    /// where each entry corresponds to an agent with the given state.
    /// Stops if max number of entries reached.
    fn get_indices(self: *Simulation, state: State, max: usize) !ArrayList(usize) {
        assert(max < self.agents.items.len);
        var indices = ArrayList(usize).init(self.*.allocator);
        _ = try indices.ensureTotalCapacityPrecise(max);
        var c: usize = 0;
        for (0..self.agents.items.len) |i| {
            if (i >= max) break;
            if (self.agents.items[i].state == state) {
                try indices.append(i);
                c += 1;
            }
        }
        return indices;
    }

    /// Simulation event that infects agents (2nd of 2 methods implemented)
    pub fn infect_method_two(self: *Simulation) !void {
        const indices =
            try self.get_indices(State.SUSCEPTIBLE, @intCast(self.parameters.encounters));
        defer indices.deinit();
        std.Random.shuffle(rnd.random(), Agent, self.*.agents.items);
        for (0..indices.items.len) |i| {
            if (self.agents.items[i].state == State.INFECTIOUS) {
                self.*.agents.items[indices.items[i]].state =
                    State.INFECTIOUS;
                self.total_infections += 1;
            }
        }
    }

    /// Simulation event that moves agents from infectious to recovered state
    pub fn recover(self: *Simulation) void {
        for (0..self.agents.items.len) |i| {
            if (self.agents.items[i].state == State.INFECTIOUS) {
                const r = rnd.random().float(f64);
                if (r < self.parameters.recovery_risk) {
                    self.*.agents.items[i].state = State.RECOVERED;
                }
            }
        }
    }

    /// Simulation event that moves agents from susceptible to vaccinated state
    pub fn vaccinate(self: *Simulation) void {
        for (0..self.agents.items.len) |i| {
            if (self.agents.items[i].state == State.SUSCEPTIBLE) {
                const r = rnd.random().float(f64);
                if (r < self.parameters.vaccination_risk) {
                    self.*.agents.items[i].state = State.VACCINATED;
                }
            }
        }
    }

    /// Simulation event that moves vaccinated and susceptible agents back to
    /// the susceptible state
    pub fn susceptible(self: *Simulation) void {
        for (0..self.agents.items.len) |i| {
            if (self.agents.items[i].state == State.VACCINATED or
                self.agents.items[i].state == State.RECOVERED)
            {
                const r = rnd.random().float(f64);
                if (r < self.parameters.regression_risk) {
                    self.agents.items[i].state = State.SUSCEPTIBLE;
                }
            }
        }
    }

    /// Simulation event that kills agents
    pub fn die(self: *Simulation) void {
        for (0..self.agents.items.len) |i| {
            if (self.agents.items[i].state == State.SUSCEPTIBLE) {
                const r = rnd.random().float(f64);
                if (r < self.parameters.death_risk_susceptible) {
                    self.*.agents.items[i].state = State.DEAD;
                }
            } else if (self.agents.items[i].state == State.INFECTIOUS) {
                const r = rnd.random().float(f64);
                if (r < self.parameters.death_risk_infectious) {
                    self.*.agents.items[i].state = State.DEAD;
                    self.infection_deaths += 1;
                }
            }
        }
    }

    /// Creates the csv header for the report event
    fn report_header() !void {
        const stdout = std.io.getStdOut().writer();
        try stdout.print("#,iter,S,I,R,V,D,TI,TID\n", .{});
    }

    /// Tallies the vital statistics for reporting
    pub fn statistics(self: Simulation) Statistics {
        return Statistics{
            .susceptible = self.count_if_state(State.SUSCEPTIBLE),
            .infectious = self.count_if_state(State.INFECTIOUS),
            .recovered = self.count_if_state(State.RECOVERED),
            .vaccinated = self.count_if_state(State.VACCINATED),
            .dead = self.count_if_state(State.DEAD),
            .total_infections = self.total_infections,
            .infection_deaths = self.infection_deaths,
        };
    }

    /// Simulation event that reports a snapshot of the main stats
    pub fn report(self: *Simulation, iteration: usize) !void {
        const s = self.statistics();
        const sim_num = self.identity;
        // We do this to prevent data races on stdout
        var str: []const u8 = undefined;
        defer self.allocator.free(str);
        str = try std.fmt.allocPrint(self.allocator, "{d},{d},{d},{d},{d},{d},{d},{d},{d}\n", .{
            sim_num,
            iteration,
            s.susceptible,
            s.infectious,
            s.recovered,
            s.vaccinated,
            s.dead,
            s.total_infections,
            s.infection_deaths,
        });

        const stdout = std.io.getStdOut().writer();
        try stdout.print("{s}", .{str});
        if (self.parameters.output_agents > 0) {
            if (iteration > 0 and
                iteration % self.parameters.output_agents == 0)
            {
                try self.print_agents();
            }
        }
    }

    /// Compares two agents' identity numbers for the purposes of sorting
    fn cmp_agent(_: void, a: Agent, b: Agent) bool {
        return a.identity < b.identity;
    }

    /// Simulation event that outputs the agents to a file
    pub fn print_agents(self: *Simulation) !void {
        const file = try std.fs.cwd().createFile(
            self.parameters.agent_filename,
            .{},
        );
        defer file.close();
        const out = file.writer();

        std.mem.sort(Agent, self.*.agents.items, {}, Simulation.cmp_agent);
        try out.print("id,state\n", .{});
        for (0..self.agents.items.len) |i| {
            try out.print("{},{}\n", .{
                self.agents.items[i].identity,
                self.agents.items[i].state,
            });
        }
    }

    /// The engine of the simulation
    pub fn simulate(self: *Simulation) !void {
        if (self.identity == 0) {
            try report_header();
        }
        try self.report(0);
        for (0..self.parameters.iterations) |i| {
            try self.grow();
            if (self.parameters.infection_method == 1) {
                self.infect_method_one();
            } else if (self.parameters.infection_method == 2) {
                try self.infect_method_two();
            } else {
                // If the identity is even use infection method 1 else 2.
                if (self.identity % 2 == 0) {
                    self.infect_method_one();
                } else {
                    try self.infect_method_two();
                }
            }
            self.recover();
            self.vaccinate();
            self.susceptible();
            self.die();
            if (i != 0 and i % 100 == 0) {
                try self.report(i);
            }
        }
        try self.report(self.parameters.iterations);
    }

    pub fn deinit(self: Simulation) void {
        self.agents.deinit();
    }
};

test "parameters" {
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

test "detect leaks" {
    rnd = RndGen.init(23);
    const p = Parameters{};
    const simulation = try Simulation.init(std.testing.allocator, 1, p);
    defer simulation.deinit();
    try std.testing.expect(simulation.agents.items.len == 10_000);
}

test "modify simulation" {
    rnd = RndGen.init(23);
    const p = Parameters{};
    var simulation = try Simulation.init(std.testing.allocator, 5, p);
    defer simulation.deinit();
    const len_a = simulation.agents.items.len;
    try simulation.grow();
    const len_b = simulation.agents.items.len;
    try std.testing.expect(len_b > len_a);
}

test "infect" {
    rnd = RndGen.init(23);
    var p = Parameters{};
    p.infections = 500;
    p.encounters = 300;
    var simulation = try Simulation.init(std.testing.allocator, 5, p);
    defer simulation.deinit();
    const infectious_before = simulation.count_if_state(State.INFECTIOUS);
    simulation.infect();
    const infectious_after = simulation.count_if_state(State.INFECTIOUS);
    try std.testing.expect(infectious_after > infectious_before);
}

test "recover" {
    rnd = RndGen.init(23);
    var p = Parameters{};
    p.infections = 5000;
    var simulation = try Simulation.init(std.testing.allocator, 5, p);
    defer simulation.deinit();
    const recovered_before = simulation.count_if_state(State.RECOVERED);
    try std.testing.expect(recovered_before == 0);
    simulation.recover();
    const recovered_after = simulation.count_if_state(State.RECOVERED);
    try std.testing.expect(recovered_after > 0);
}

test "vaccinate" {
    rnd = RndGen.init(23);
    var p = Parameters{};
    p.infections = 5000;
    var simulation = try Simulation.init(std.testing.allocator, 5, p);
    defer simulation.deinit();
    const vaccinated_before = simulation.count_if_state(State.VACCINATED);
    try std.testing.expect(vaccinated_before == 0);
    simulation.vaccinate();
    const vaccinated_after = simulation.count_if_state(State.VACCINATED);
    try std.testing.expect(vaccinated_after > 0);
}

test "susceptible" {
    rnd = RndGen.init(23);
    var p = Parameters{};
    p.infections = 500;
    p.regression_risk = 0.1;
    var simulation = try Simulation.init(std.testing.allocator, 5, p);
    defer simulation.deinit();
    simulation.recover();
    simulation.vaccinate();
    const susceptible_before = simulation.count_if_state(State.SUSCEPTIBLE);
    simulation.susceptible();
    const susceptible_after = simulation.count_if_state(State.SUSCEPTIBLE);
    try std.testing.expect(susceptible_after > susceptible_before);
}

test "die" {
    rnd = RndGen.init(23);
    var p = Parameters{};
    p.death_risk_susceptible = 0.01;
    p.death_risk_infectious = 0.1;
    p.infections = 5000;
    var simulation = try Simulation.init(std.testing.allocator, 5, p);
    defer simulation.deinit();
    const deaths_before = simulation.count_if_state(State.DEAD);
    const infectious_deaths_before = simulation.infection_deaths;
    try std.testing.expect(deaths_before == 0);
    try std.testing.expect(infectious_deaths_before == 0);
    simulation.die();
    const deaths_after = simulation.count_if_state(State.DEAD);
    const infection_deaths_after = simulation.infection_deaths;
    try std.testing.expect(deaths_after > 0);
    try std.testing.expect(infection_deaths_after > 0);
    try std.testing.expect(infection_deaths_after < deaths_after);
}
