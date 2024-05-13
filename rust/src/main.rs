use abm;
use num_cpus;
use clap::Parser;
use threadpool::ThreadPool;

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

