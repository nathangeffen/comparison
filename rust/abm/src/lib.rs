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

//! This crate implements a simple agent-based model simulation engine
//! for the purpose of comparing programming environments.

#![crate_name = "abm"]
use std::fmt;
use std::fs::File;
use std::io::Write;
// use rand::prelude::*;
use round::round;

/// Represents the possible states an agent can be in.
#[derive(Debug, PartialEq)]
pub enum State {
    SUSCEPTIBLE,
    INFECTIOUS,
    RECOVERED,
    VACCINATED,
    DEAD,
}

/// Displays the first letter of a state in upper case
impl fmt::Display for State {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            State::SUSCEPTIBLE => write!(f, "S"),
            State::INFECTIOUS => write!(f, "I"),
            State::RECOVERED => write!(f, "R"),
            State::VACCINATED => write!(f, "V"),
            State::DEAD => write!(f, "D"),
        }
    }
}

/// Determines which infection event to call in the simulation
#[derive(Debug, Clone)]
pub enum InfectionMethod {
    BOTH,
    ONE,
    TWO,
}

/// Displays the infection method
impl fmt::Display for InfectionMethod {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            InfectionMethod::BOTH => write!(f, "Both"),
            InfectionMethod::ONE => write!(f, "One"),
            InfectionMethod::TWO => write!(f, "Two"),
        }
    }
}

/// These are the parameters for a simulation.
#[derive(Debug, Clone)]
pub struct Parameters {
    pub agents: usize,
    pub iterations: i32,
    pub infections: usize,
    pub encounters: usize,
    pub growth: f64,
    pub death_prob_susceptible: f64,
    pub death_prob_infectious: f64,
    pub recovery_prob: f64,
    pub vaccination_prob: f64,
    pub regression_prob: f64,
    pub infection_method: InfectionMethod,
    pub output_agents: i32,
    pub agent_filename: String,
}

impl Default for Parameters {
    fn default() -> Self {
        Parameters {
            agents: 100,
            iterations: 365 * 4,
            infections: 20,
            encounters: 10,
            growth: 0.1,
            death_prob_susceptible: 0.001,
            death_prob_infectious: 0.01,
            recovery_prob: 0.1,
            vaccination_prob: 0.01,
            regression_prob: 0.001,
            infection_method: InfectionMethod::BOTH,
            output_agents: 0,
            agent_filename: String::from(""),
        }
    }
}

/// This typically represents a single person in a simulation.
#[derive(Debug)]
pub struct Agent {
    /// Unique identity of the agent
    identity: usize,
    /// State the agent is in
    state: State,
}

/// This is used to represent a snapshot of stats for a simulation.
#[derive(Debug)]
pub struct Statistics {
    pub susceptible: usize,
    pub infectious: usize,
    pub recovered: usize,
    pub vaccinated: usize,
    pub dead: usize,
    pub total_infections: usize,
    pub infection_deaths: usize,
}

#[derive(Debug)]
struct Rng {
    seed: u64,
    m: u64,
}

impl Rng {
    pub fn new(seed: u64) -> Rng {
        return Rng { seed, m: 32768 };
    }

    pub fn uint(&mut self) -> u64 {
        self.seed = self.seed.wrapping_mul(1103515245).wrapping_add(12345);
        return (self.seed / 65536) % self.m;
    }

    pub fn to(&mut self, max: u64) -> u64 {
        let result = self.uint() % max;
        return result;
    }

    pub fn real(&mut self) -> f64 {
        let result: f64 = (self.uint() as f64) / (self.m as f64);
        return result;
    }

    pub fn shuffle(&mut self, agents: &mut [Agent]) {
        for i in (1..agents.len()).rev() {
            let j: usize = self.to((i + 1) as u64) as usize;
            agents.swap(i, j);
        }
    }
}

/// This is the data structure for the simulation engine.
#[derive(Debug)]
pub struct Simulation {
    /// Unique identifier of the simulation.
    identity: usize,
    /// Vector of agents, typically each one representing a person
    agents: Vec<Agent>,
    /// Parameters of the simulation
    parameters: Parameters,
    /// Tracks total number of times infections occur including the initial infections.
    total_infections: usize,
    /// Tracks the number of deaths of infected agents
    infection_deaths: usize,
    /// Random number generator
    rng: Rng,
}

impl Simulation {
    /// ```rust
    /// use crate::abm;
    ///
    /// let mut simulation = abm::Simulation::new(1, Default::default());
    /// simulation.simulate();
    /// let statistics = simulation.statistics();
    /// assert_ne!(statistics.total_infections, 20);
    /// assert_ne!(statistics.infection_deaths, 0);
    /// ```
    /// Creates a new simulation
    pub fn new(identity: usize, parameters: &Parameters) -> Simulation {
        let mut agents = Vec::new();
        for i in 0..parameters.agents {
            let agent = Agent {
                identity: i,
                state: State::SUSCEPTIBLE,
            };
            agents.push(agent);
        }

        let mut rng = Rng::new(identity as u64);
        rng.shuffle(&mut agents);
        for agent in &mut agents[0..parameters.infections] {
            agent.state = State::INFECTIOUS;
        }
        let infections = parameters.infections;
        let s = Simulation {
            identity,
            agents: Vec::from(agents),
            parameters: parameters.clone(),
            total_infections: infections,
            infection_deaths: 0,
            rng,
        };
        return s;
    }

    /// Counts number of agents with given state
    fn count_if_state(&self, state: State) -> usize {
        let mut total = 0;
        for agent in &self.agents {
            if agent.state == state {
                total += 1;
            }
        }
        return total;
    }

    /// Counts number of agents not in the given state
    fn count_if_not_state(&self, state: State) -> usize {
        let n = self.count_if_state(state);
        return self.agents.len() - n;
    }

    /// Simulation event that grows the agent population
    pub fn grow(&mut self) {
        let num_agents = self.count_if_not_state(State::DEAD);
        let new_agents = round(self.parameters.growth * num_agents as f64, 0);
        let n = self.agents.len() + new_agents as usize;
        for identity in self.agents.len()..n {
            let agent = Agent {
                identity,
                state: State::SUSCEPTIBLE,
            };
            self.agents.push(agent);
        }
    }

    /// Simulation event that infects agents (1st of 2 methods implemented)
    pub fn infect_method_one(&mut self) {
        for _ in 0..self.parameters.encounters {
            let ind1 = self.rng.to(self.agents.len() as u64) as usize;
            let ind2 = self.rng.to(self.agents.len() as u64) as usize;
            if self.agents[ind1].state == State::SUSCEPTIBLE
                && self.agents[ind2].state == State::INFECTIOUS
            {
                self.agents[ind1].state = State::INFECTIOUS;
                self.total_infections += 1;
            } else if self.agents[ind2].state == State::SUSCEPTIBLE
                && self.agents[ind1].state == State::INFECTIOUS
            {
                self.agents[ind2].state = State::INFECTIOUS;
                self.total_infections += 1;
            }
        }
    }

    /// Returns a vector of indices into an agents vector
    /// where each entry corresponds to an agent with the given state.
    /// Stops if max number of entries reached.
    fn get_indices(agents: &Vec<Agent>, state: State, max: usize) -> Vec<usize> {
        let mut indices = Vec::with_capacity(max);
        for i in 0..agents.len() {
            if i >= max {
                break;
            }
            if agents[i].state == state {
                indices.push(i);
            }
        }
        return indices;
    }

    /// Simulation event that infects agents (2nd of 2 methods implemented)
    pub fn infect_method_two(&mut self) {
        let indices_susceptible =
            Simulation::get_indices(&self.agents, State::SUSCEPTIBLE, self.parameters.encounters);
        self.rng.shuffle(&mut self.agents);
        for i in 0..indices_susceptible.len() {
            if self.agents[i].state == State::INFECTIOUS {
                self.agents[indices_susceptible[i]].state = State::INFECTIOUS;
                self.total_infections += 1;
            }
        }
    }

    /// Simulation event that moves agents from infectious to recovered state
    pub fn recover(&mut self) {
        for agent in &mut self.agents {
            if agent.state == State::INFECTIOUS {
                let r: f64 = self.rng.real();
                if r < self.parameters.recovery_prob {
                    agent.state = State::RECOVERED;
                }
            }
        }
    }

    /// Simulation event that moves agents from susceptible to vaccinated state
    pub fn vaccinate(&mut self) {
        for agent in &mut self.agents {
            if agent.state == State::SUSCEPTIBLE {
                let r: f64 = self.rng.real();
                if r < self.parameters.vaccination_prob {
                    agent.state = State::VACCINATED;
                }
            }
        }
    }

    /// Simulation event that moves vaccinated and susceptible agents back to
    /// the susceptible state
    pub fn susceptible(&mut self) {
        for i in 0..self.agents.len() {
            if self.agents[i].state == State::VACCINATED || self.agents[i].state == State::RECOVERED
            {
                let r: f64 = self.rng.real();
                if r < self.parameters.regression_prob {
                    self.agents[i].state = State::SUSCEPTIBLE;
                }
            }
        }
    }

    /// Simulation event that kills agents
    pub fn die(&mut self) {
        for agent in &mut self.agents {
            if agent.state == State::SUSCEPTIBLE {
                let r: f64 = self.rng.real();
                if r < self.parameters.death_prob_susceptible {
                    agent.state = State::DEAD
                }
            } else if agent.state == State::INFECTIOUS {
                let r: f64 = self.rng.real();
                if r < self.parameters.death_prob_infectious {
                    agent.state = State::DEAD;
                    self.infection_deaths += 1;
                }
            }
        }
    }

    /// Creates the csv header for the report event
    fn report_header(&self) {
        println!("#,iter,S,I,R,V,D,TI,TID");
    }

    /// Tallies the vital statistics for reporting
    pub fn statistics(&self) -> Statistics {
        Statistics {
            susceptible: self.count_if_state(State::SUSCEPTIBLE),
            infectious: self.count_if_state(State::INFECTIOUS),
            recovered: self.count_if_state(State::RECOVERED),
            vaccinated: self.count_if_state(State::VACCINATED),
            dead: self.count_if_state(State::DEAD),
            total_infections: self.total_infections,
            infection_deaths: self.infection_deaths,
        }
    }

    /// Simulation event that reports a snapshot of the main stats
    pub fn report(&mut self, iteration: i32) {
        let s = self.statistics();
        let sim_num = self.identity;
        println!(
            "{sim_num},{iteration},{},{},{},{},{},{},{}",
            s.susceptible,
            s.infectious,
            s.recovered,
            s.vaccinated,
            s.dead,
            s.total_infections,
            s.infection_deaths
        );
        if self.parameters.output_agents > 0 {
            if iteration > 0 && iteration % self.parameters.output_agents == 0 {
                self.print_agents();
            }
        }
    }

    /// Outputs the agents to a file
    pub fn print_agents(&mut self) {
        self.agents.sort_by(|a, b| a.identity.cmp(&b.identity));
        let s = self.parameters.agent_filename.clone();
        let mut file = File::create(s).unwrap();
        writeln!(&mut file, "id,state").unwrap();
        for agent in &self.agents {
            writeln!(&mut file, "{},{}", agent.identity, agent.state).unwrap();
        }
    }

    /// The engine of the simulation
    pub fn simulate(&mut self) {
        if self.identity == 0 {
            self.report_header();
        }
        self.report(0);
        for i in 0..self.parameters.iterations {
            self.grow();
            match self.parameters.infection_method {
                InfectionMethod::BOTH => {
                    // If the identity is even use infection method 1 else 2.
                    if self.identity % 2 == 0 {
                        self.infect_method_one();
                    } else {
                        self.infect_method_two();
                    }
                }
                InfectionMethod::ONE => self.infect_method_one(),
                InfectionMethod::TWO => self.infect_method_two(),
            }
            self.recover();
            self.vaccinate();
            self.susceptible();
            self.die();
            if i != 0 && i % 100 == 0 {
                self.report(i);
            }
        }
        self.report(self.parameters.iterations);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn confirm_setup() {
        let p: Parameters = Default::default();
        let s = Simulation::new(5, &p);
        assert_eq!(s.agents.len(), 100);
        let mut total = 0;
        for i in 0..p.agents {
            if s.agents[i].state == State::INFECTIOUS {
                total += 1
            }
        }
        assert_eq!(total, 20);
    }
}
