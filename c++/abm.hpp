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

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

struct Rng {
		uint64_t seed;
		const uint64_t a = 22695477;
		const uint64_t c = 1;
		const uint64_t m = 32768;

		Rng(uint64_t s) {
				seed = s;
		}

		uint64_t uint() {
				seed = seed * 1103515245 + 12345;
				return (seed/65536) % m;
		}

		uint64_t to(uint64_t max) {
				uint64_t result = uint() % max;
				return result;
		}

		double real() {
				double result = (double) uint() / m;
				return result;
		}
};


/// Possible agent states
enum State {
		SUSCEPTIBLE = 0,
		INFECTIOUS,
		RECOVERED,
		VACCINATED,
		DEAD
};

std::ostream& operator<<(std::ostream&, const State&);

/// Determines which infection event to call in the simulation
enum InfectionMethod {
		BOTH = 0,
		ONE = 1,
		TWO = 2
};

/// Structure to hold simulation's parameters.
struct Parameters {
		size_t simulations = 20;
		size_t iterations = 365 * 4;
		size_t agents = 10000;
		size_t infections = 10;
		size_t encounters = 100;
		double growth = 0.0001;
		double death_prob_susceptible = 0.0001;
		double death_prob_infectious = 0.001;
		double recovery_prob = 0.01;
		double vaccination_prob = 0.001;
		double regression_prob = 0.0003;
		InfectionMethod infection_method = InfectionMethod::BOTH;
		int output_agents = 0;
		std::string agent_filename = "agents.csv";
};

/// Holds an agent who has two attributes, a unique identity and a state.
struct Agent {
		Agent(int identity_, State state_) : identity(identity_), state(state_) {};
		int identity;
		State state;
};

void shuffle(std::vector<Agent>  &agents, Rng& rng);

/// This is used to represent a snapshot of stats for a simulation.
struct Statistics {
		size_t susceptible;
		size_t infectious;
		size_t recovered;
		size_t vaccinated;
		size_t dead;

		Statistics(const std::vector<Agent> &agents) {
				susceptible = std::count_if(agents.begin(), agents.end(),
								[](const Agent &a) {
								return a.state == State::SUSCEPTIBLE;
								});
				infectious = std::count_if(agents.begin(), agents.end(),
								[](const Agent &a) {
								return a.state == State::INFECTIOUS;
								});
				recovered = std::count_if(agents.begin(), agents.end(),
								[](const Agent &a) {
								return a.state == State::RECOVERED;
								});
				vaccinated = std::count_if(agents.begin(), agents.end(),
								[](const Agent &a) {
								return a.state == State::VACCINATED;
								});
				dead = std::count_if(agents.begin(), agents.end(),
								[](const Agent &a) {
								return a.state == State::DEAD;
								});
		}
};

/// This is the data structure for the simulation engine.
struct Simulation {
		size_t identity; // Unique id of this simulation
		std::vector<Agent> agents; // Holds the simulation's agents
		Parameters parameters;
		size_t total_infections;
		size_t infection_deaths = 0;
		Rng rng;

		/// Initializes the simulation with a unique identity, initial number of
		/// agents and initial number of infections.
		Simulation(size_t identity, const Parameters& parameters) :
				identity (identity), parameters(parameters), rng(identity) {
						for (size_t i = 0; i < parameters.agents; i++) {
								Agent agent(i, State::SUSCEPTIBLE);
								agents.push_back(agent);
						}
						shuffle(agents, rng);
						for (size_t i = 0; i < parameters.infections; i++) {
								agents[i].state = State::INFECTIOUS;
						}
						total_infections = parameters.infections;
				}


		/// Event to grow the number of agents.
		void grow() {
				size_t num_agents = std::count_if(agents.begin(), agents.end(),
								[](const Agent &a) {
								return a.state != State::DEAD;
								});
				size_t new_agents = std::round(parameters.growth * num_agents);
				size_t size = agents.size();
				for (size_t i = size; i < size + new_agents; i++)
						agents.push_back(Agent(i, State::SUSCEPTIBLE));
		}

		/// Intentionally time-consuming event to infect agents.  Agents
		/// randomly encounter one another. If an infectious agent
		/// encounters a susceptible one, an infection takes place.
		void infect_method_one() {
				for (size_t i = 0; i < parameters.encounters; i++) {
						int ind1 = rng.to(agents.size());
						int ind2 = rng.to(agents.size());
						if (agents[ind1].state == State::SUSCEPTIBLE &&
										agents[ind2].state == State::INFECTIOUS) {
								agents[ind1].state = State::INFECTIOUS;
								++total_infections;
						} else if (agents[ind1].state == State::INFECTIOUS &&
										agents[ind2].state == State::SUSCEPTIBLE) {
								agents[ind2].state = State::INFECTIOUS;
								++total_infections;
						}
				}
		}

		/// Returns a vector of indices into an agents vector
		/// where each entry corresponds to an agent with the given state.
		/// Stops if max number of entries reached.
		std::vector<size_t> get_indices(State state, size_t max) {
				std::vector<size_t> indices;
				for (size_t i = 0; i < agents.size(); i++) {
						if (i >= max) break;
						if (agents[i].state == state)
								indices.push_back(i);
				}
				return indices;
		}

		/// Simulation event that infects agents (2nd of 2 methods implemented)
		void infect_method_two() {
				std::vector<size_t> indices = get_indices(State::SUSCEPTIBLE,
								parameters.encounters);
				shuffle(agents, rng);
				for (size_t i = 0; i < indices.size(); i++) {
						if (agents[i].state == State::INFECTIOUS) {
								agents[indices[i]].state = State::INFECTIOUS;
								++total_infections;
						}
				}
		}

		/// Simulation event that moves agents from infectious to recovered state
		void recover() {
				for (auto &agent: agents) {
						if (agent.state == State::INFECTIOUS) {
								if (rng.real() < parameters.recovery_prob)
										agent.state = State::RECOVERED;
						}
				}
		}

		/// Simulation event that moves agents from susceptible to vaccinated state
		void vaccinate() {
				for (auto &agent: agents) {
						if (agent.state == State::SUSCEPTIBLE) {
								if (rng.real() < parameters.vaccination_prob)
										agent.state = State::VACCINATED;
						}
				}
		}

		/// Simulation event that moves vaccinated and susceptible agents back to
		/// the susceptible state
		void susceptible() {
				for (auto &agent: agents) {
						if (agent.state == State::VACCINATED ||
										agent.state == State::RECOVERED) {
								if (rng.real() < parameters.regression_prob)
										agent.state = State::SUSCEPTIBLE;
						}
				}
		}

		/// Simple death event that differentiates between infectious and susceptible
		/// agents.
		void die() {
				for (auto & agent: agents) {
						if (agent.state == State::SUSCEPTIBLE) {
								if (rng.real() < parameters.death_prob_susceptible) {
										agent.state = State::DEAD;
								}
						} else if (agent.state == State::INFECTIOUS) {
								if (rng.real() < parameters.death_prob_infectious) {
										agent.state = State::DEAD;
										++infection_deaths;
								}
						}
				}
		}

		/// Creates the csv header for the report event
		void report_header() {
				std::cout << "#,iter,S,I,R,V,D,TI,TID\n";
		}

		/// Outputs the agents to a file
		void print_agents() {
				sort(agents.begin(), agents.end(), [](const Agent &a, const Agent &b) {
								return a.identity < b.identity;
								});
				std::ofstream file(parameters.agent_filename);

				file << "id,state\n";
				for (auto &agent: agents)
						file << agent.identity << "," << agent.state << "\n";
				file.close();
		}

		/// Prints out the vital statistics.
		void report(int iteration) {
				Statistics stats = Statistics(agents);
				// We want to send one string to cout to prevent data races on stdout
				std::stringstream ss;
				ss << identity << "," << iteration << ","
						<< stats.susceptible << "," << stats.infectious << ","
						<< stats.recovered << "," << stats.vaccinated << ","
						<< stats.dead << "," << total_infections << ","
						<< infection_deaths << "\n";
				std::cout << ss.str();
				if (parameters.output_agents > 0) {
						if (iteration > 0 && iteration % parameters.output_agents == 0) {
								print_agents();
						}
				}
		}

		/// Simulation engine that repeatedly executes all the events
		/// for specified number of iterations.
		void simulate() {
				if (identity == 0)
						report_header();
				report(0);
				for (size_t i = 0; i < parameters.iterations; i++) {
						grow();
						switch(parameters.infection_method) {
								case BOTH:
										if (identity % 2 == 0)
												infect_method_one();
										else
												infect_method_two();
										break;
								case ONE: infect_method_one(); break;
								case TWO: infect_method_two(); break;
						}
						recover();
						vaccinate();
						susceptible();
						die();
						if (i != 0 && i % 100 == 0) {
								report(i);
						}
				}
				report(parameters.iterations);
		}
};


