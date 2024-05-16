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
#[command(version, about, long_about = None)]
struct Parameters {
    #[arg(short, long, default_value_t = 10)]
    pub simulations: i32,
    #[arg(long, default_value_t = 0)]
    pub identity: i32,
    #[arg(short, long, default_value_t = 10000)]
    pub agents: i32,
    #[arg(short, long, default_value_t = 365 * 4)]
    pub iterations: i32,
    #[arg(long, default_value_t = 10)]
    pub infections: i32,
    #[arg(short, long, default_value_t = 100)]
    pub encounters: i32,
    #[arg(short, long, default_value_t = 0.0001)]
    pub growth: f64,
    #[arg(long, default_value_t = 0.0001)]
    pub death_risk_susceptible: f64,
    #[arg(long, default_value_t = 0.001)]
    pub death_risk_infectious: f64,
    #[arg(short, long, default_value_t = 0.01)]
    pub recovery_risk: f64,
    #[arg(short, long, default_value_t = 0.001)]
    pub vaccination_risk: f64,
    #[arg(long, default_value_t = 0.0003)]
    pub regression_risk: f64,
    #[arg(long, default_value_t = 0)]
    pub output_agents: i32,
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
        death_risk_susceptible: parameters.death_risk_susceptible,
        death_risk_infectious: parameters.death_risk_infectious,
        recovery_risk: parameters.recovery_risk,
        vaccination_risk: parameters.vaccination_risk,
        regression_risk: parameters.regression_risk,
        output_agents: parameters.output_agents,
        agent_filename: parameters.agent_filename.clone()
    };
    let mut s = abm::Simulation::new(parameters.identity, abm_parameters);
    s.simulate();
}

/// Processes parameters, sets up thread pool and invokes the execution of the
/// simulations.
fn main() {
    let parameters =  Parameters::parse();
    if parameters.simulations == 0 {
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

