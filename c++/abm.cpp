/**
 * Simple agent-based simulation of an infectious disease.
 */


#include <algorithm>
#include <iostream>
#include <sstream>
#include <random>
#include <vector>

#include <boost/asio.hpp>

#include <getopt.h>

std::random_device rd;
thread_local std::default_random_engine rng(rd());

/**
 * For processing command line arguments
 */
static struct option long_options[] = {
  {"simulations", required_argument, 0, 0},
  {"iterations", required_argument, 0, 0},
  {"infections", required_argument, 0, 0},
  {"agents", required_argument, 0,  0 },
  {"growth", required_argument, 0, 0},
  {"events", required_argument, 0,  0},
  {"death_rate_susceptible", required_argument, 0,  0},
  {"death_rate_infected", required_argument, 0,  0},
  {0,         0,                 0,  0 }
};

/**
 *   Possible agent states
 */
enum State {
  SUSCEPTIBLE = 0,
  INFECTED,
  DEAD
};

/**
 * Holds an agent who has two attributes, a unique identity and a state.
 */
struct Agent {
  Agent(int identity_, State state_) : identity(identity_), state(state_) {};
  int identity;
  State state;
};

/**
 * Prints help screen if -h command line options specified.
 */
void print_help(const char *prog) {
  struct option *it = long_options;

  std::cerr << prog << " [options]\n";
  while(it->name != 0) {
    std::cerr << it->name << " <value> \n";
    ++it;
  }
}

/**
 * Simple simulation engine with a few events.
 */
struct Simulation {

  /** Initializes the simulation with a unique identity, initial number of
   *  agents and initial number of infections.
   */
  Simulation(int identity_, int num_agents, int num_infections) {
    identity = identity_;
    for (int i = 0; i < num_agents; i++) {
      Agent agent(i, State::SUSCEPTIBLE);
      agents.push_back(agent);
    }
    for (int i = 0; i < num_infections; i++) {
      agents[i].state = State::INFECTED;
    }
    std::shuffle(agents.begin(), agents.end(), rng);
  }

  /**
   * Event to grow the number of agents.
   */
  void grow(double growth_per_day) {
    int num_agents = std::count_if(agents.begin(), agents.end(),
				   [](const Agent &a) {
				     return a.state != State::DEAD;
				   });
    int new_agents = round(growth_per_day * num_agents);
    int id = agents.size();
    for (int i = 0; i < new_agents; i++) {
      Agent agent(id, State::SUSCEPTIBLE);
      agents.push_back(agent);
      ++id;
    }
  }

  /**
   * Intentionally time-consuming event to infect agents.  Agents
   * randomly encounter one another. If an infected agent
   * encounters a susceptible one, an infection takes place.
   */
  void infect(int events) {
    std::uniform_int_distribution<> dist(0, agents.size() - 1);
    for (int i = 0; i < events; i++) {
      int ind1 = dist(rng);
      int ind2 = dist(rng);
      if (agents[ind1].state == State::SUSCEPTIBLE &&
	  agents[ind2].state == State::INFECTED) {
	agents[ind1].state = State::INFECTED;
      } else if (agents[ind1].state == State::INFECTED &&
		 agents[ind2].state == State::SUSCEPTIBLE) {
	agents[ind2].state = State::INFECTED;
      }
    }
  }

  /**
   * Simple death event that differentiates between infected and susceptible
   * agents.
   */
  void die(double death_rate_susceptible, double death_rate_infected) {
    std::uniform_real_distribution<> dist(0.0, 1.0);
    for (auto & agent: agents) {
      if (agent.state == State::SUSCEPTIBLE) {
	if (dist(rng) < death_rate_susceptible) {
	  agent.state = State::DEAD;
	}
      } else if (agent.state == State::INFECTED) {
	if (dist(rng) < death_rate_infected) {
	  agent.state = State::DEAD;
	}
      }
    }
  }

  /** Prints out the vital statistics.
   */
  void report(int iteration) {
    int num_susceptible = std::count_if(agents.begin(), agents.end(),
					[](const Agent &a) {
					  return a.state == State::SUSCEPTIBLE;
					});
    int num_infections = std::count_if(agents.begin(), agents.end(),
				       [](const Agent &a) {
					 return a.state == State::INFECTED;
				       });
    int num_deaths = std::count_if(agents.begin(), agents.end(),
				   [](const Agent &a) {
				     return a.state == State::DEAD;
				   });
    // We want to send one string to cout to prevent data races on stdout
    std::stringstream ss;
    ss << "Simulation: " << identity << ". "
       << "Iteration: " << iteration << ". "
       << "Susceptible: " << num_susceptible << ". "
       << "Infections: " << num_infections << ". "
       << "Deaths: " << num_deaths << "." << "\n";
    std::cout << ss.str();
  }

  /** Simulation engine that repeatedly executes all the events
   *  for specified number of iterations.
   */
  void simulate(int iterations, double growth_per_day, int events,
		double death_rate_susceptible, double death_rate_infected) {
    for (int i = 0; i < iterations; i++) {
      grow(growth_per_day);
      infect(events);
      die(death_rate_susceptible, death_rate_infected);
      if (i % 100 == 0) {
	report(i);
      }
    }
  }

  int identity; // Unique id of this simulation
  std::vector<Agent> agents; // Holds the simulation's agents
};


/**
 * Structure to hold simulation's parameters.
 */
struct Parameters {
  int simulations = 10;
  int sim_num = 0;
  int iterations = 365*4;
  int infections = 10;
  int agents = 10000;
  int events = 20;
  double growth = 0.0001;
  double death_rate_susceptible = 0.0001;
  double death_rate_infected = 0.001;
};

/**
 * Process command line arguments and return parameters.
 */
Parameters process_arguments(int argc, char **argv) {
  Parameters result;
  while (1) {
    int option_index = 0;

    int c = getopt_long(argc, argv, "h", long_options, &option_index);

    if (c == -1)
      break;

    if (c == 0) {
      const char *s = long_options[option_index].name;
      if (strcmp(s, "simulations") == 0) {
	result.simulations = std::stoi(optarg);
      } else if (strcmp(s, "iterations") == 0) {
	result.iterations = std::stoi(optarg);
      } else if (strcmp(s, "infections") == 0) {
	result.infections = std::stoi(optarg);
      } else if (strcmp(s, "agents") == 0) {
	result.agents = std::stoi(optarg);
      } else if (strcmp(s, "events") == 0) {
	result.events = std::stoi(optarg);
      } else if (strcmp(s, "growth") == 0) {
	result.growth = std::stod(optarg);
      } else if (strcmp(s, "death_rate_susceptible") == 0) {
	result.death_rate_susceptible = std::stod(optarg);
      } else if (strcmp(s, "death_rate_infected") == 0) {
	result.death_rate_infected = std::stod(optarg);
      } else {
	std::cerr << "Unknown options: " << s << "\n";
      }
    }
    if (c == 'h') {
      print_help(argv[0]);
    }
  }

  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }
  return result;
}

/**
 * Runs one simulation. Used by Boost pool.
 */
void run_one_simulation(const Parameters& p) {
  Simulation simulation(p.sim_num, p.agents, p.infections);
  simulation.simulate(p.iterations, p.growth, p.events,
		      p.death_rate_susceptible, p.death_rate_infected);
  simulation.report(p.iterations);
}

/**
 * Gets command line arguments and then runs the simulations in a
 * parallel pool.
 */
int main(int argc, char **argv) {
  Parameters original = process_arguments(argc, argv);
  std::vector<Parameters> parameters(original.simulations);
  boost::asio::thread_pool pool(std::thread::hardware_concurrency());
  for (int i = 0; i < original.simulations; i++) {
    parameters[i] = original;
    parameters[i].sim_num = i;
    boost::asio::post(pool, [&parameters, i]() {
      run_one_simulation(parameters[i]);
    });
  }
  pool.join();

  return 0;
}
