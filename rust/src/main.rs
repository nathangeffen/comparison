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

use abm;
use num_cpus;
use clap::Parser;
use threadpool::ThreadPool;

/// This struct handles the command line arguments.
#[derive(Parser, Debug, Clone)]
#[command(version, about, long_about = None, rename_all = "snake_case")]
struct Parameters {
    /// Number of simulations
    #[arg(short, long, default_value_t = 20)]
    pub simulations: usize,

    /// Id number of simulation (if running only one)
    #[arg(long, default_value_t = 0)]
    pub identity: usize,

    /// Number of iterations in a simulation
    #[arg(short, long, default_value_t = 365 * 4)]
    pub iterations: i32,

    /// Number of initial agents
    #[arg(short, long, default_value_t = 10_000)]
    pub agents: usize,

    /// Number of initial agents who are infectious
    #[arg(long, default_value_t = 10)]
    pub infections: usize,

    /// Number of encounters between agents in the infection methods
    #[arg(short, long, default_value_t = 100)]
    pub encounters: usize,

    /// Growth rate of the agent population per iteration
    #[arg(short, long, default_value_t = 0.0001)]
    pub growth: f64,

    /// Death rate of the susceptible agent population per iteration
    #[arg(long, default_value_t = 0.0001)]
    pub death_prob_susceptible: f64,

    /// Death rate of the infectious agent population per iteration
    #[arg(long, default_value_t = 0.001)]
    pub death_prob_infectious: f64,

    /// Prob of infectious agent moving to recovery state per iteration
    #[arg(short, long, default_value_t = 0.01)]
    pub recovery_prob: f64,

    /// Prob of susceptible agent moving to vaccinated state per iteration
    #[arg(short, long, default_value_t = 0.001)]
    pub vaccination_prob: f64,

    /// Prob of recovered or vaccinated agent becoming susceptible per iteration
    #[arg(long, default_value_t = 0.0003)]
    pub regression_prob: f64,

    /// Infection method to use (0 = BOTH, 1 = ONE, 2 = TWO)
    #[arg(long, default_value_t = 0)]
    pub infection_method: u8,

    /// Iteration frequency to write out agents (0 = never)
    #[arg(long, default_value_t = 0)]
    pub output_agents: i32,

    /// Agent output file name
    #[arg(long, default_value_t = String::from("agents.csv"))]
    pub agent_filename: String
}

/// Runs one simulation. Called within the thread pool so has to be thread
/// safe.
fn one_simulation(parameters: Parameters) {
    let abm_parameters = abm::Parameters {
        agents: parameters.agents,
        iterations: parameters.iterations,
        infections: parameters.infections,
        encounters: parameters.encounters,
        growth: parameters.growth,
        death_prob_susceptible: parameters.death_prob_susceptible,
        death_prob_infectious: parameters.death_prob_infectious,
        recovery_prob: parameters.recovery_prob,
        vaccination_prob: parameters.vaccination_prob,
        regression_prob: parameters.regression_prob,
	infection_method: match parameters.infection_method {
	    1 => abm::InfectionMethod::ONE,
	    2 => abm::InfectionMethod::TWO,
	    _ => abm::InfectionMethod::BOTH,
	},
        output_agents: parameters.output_agents,
        agent_filename: parameters.agent_filename.clone()
    };
    let mut s = abm::Simulation::new(parameters.identity, &abm_parameters);
    s.simulate();
}

/// Processes parameters, sets up thread pool and invokes the execution of the
/// simulations.
 fn main() {
    let parameters =  Parameters::parse();
    if parameters.simulations <= 1 {
         one_simulation(parameters);
    } else {
        let threads = num_cpus::get();
        let pool = ThreadPool::new(threads);
        for i in 0..parameters.simulations {
            let mut p = parameters.clone();
            p.identity = i;
            pool.execute(move|| {one_simulation(p)});
        }
        pool.join();
    }
}
