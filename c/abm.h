#ifndef __ABM_H__
#define __ABM_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>

struct rng {
		bool from_file;
		uint64_t seed;
};

typedef struct rng rng_t;

enum state {
		SUSCEPTIBLE = 0,
		INFECTIOUS,
		RECOVERED,
		VACCINATED,
		DEAD,
};

typedef enum state state_t;

/// Holds an agent who has two attributes, a unique identity and a state.
struct agent {
		size_t identity;
		state_t state;
};

typedef struct agent agent_t;

/// This is used to represent a snapshot of stats for a simulation.
struct statistics {
		size_t susceptible;
		size_t infectious;
		size_t recovered;
		size_t vaccinated;
		size_t dead;
};

typedef struct statistics statistics_t;

/// Determines which infection event to call in the simulation
enum infection_method {
		BOTH = 0,
		ONE = 1,
		TWO = 2,
};

typedef enum infection_method infection_method_t;

/// Structure to hold simulation's parameters.
struct parameters {
		size_t  simulations;
		size_t identity;
		size_t iterations;
		size_t agents;
		size_t infectious;
		size_t encounters;
		double growth;
		double death_prob_susceptible;
		double death_prob_infectious;
		double recovery_prob;
		double vaccination_prob;
		double regression_prob;
		infection_method_t infection_method;
		int output_agents;
		char agent_filename[255];
		bool random_file;
};

typedef struct parameters parameters_t;

/// Simple simulation engine with a few events.
struct simulation {
		size_t identity;    // Unique id of this simulation
		agent_t *agents; // Array holding the simulation's agents
		size_t size;
		size_t capacity;
		size_t total_infections;
		size_t infection_deaths;
		parameters_t parameters;
		rng_t rng;
};

typedef struct simulation simulation_t;

void read_randoms(void);
void rng_init(rng_t *rng, uint64_t seed, bool from_file);
uint64_t rng_uint64_t(rng_t *rng);
uint64_t rng_to(rng_t *rng, uint64_t max);
double rng_double(rng_t *rng);
char state_char(state_t state);
agent_t new_agent(int identity, state_t state);
void shuffle(agent_t *agents, size_t n, rng_t *rng);

size_t count_if_state(const agent_t *agents, size_t n, state_t state);
size_t count_if_not_state(const agent_t *agents, size_t n, state_t state);
statistics_t get_stats(const agent_t *agents, size_t n);
simulation_t new_simulation(size_t identity, const parameters_t *parameters);
void grow(simulation_t *simulation);
void infect_method_one(simulation_t *simulation);
void get_indices(const simulation_t *simulation, size_t **indices, size_t *n, state_t state, size_t max);
void get_indices2(const simulation_t *simulation, size_t *n, state_t state, size_t max);
void infect_method_two(simulation_t *simulation);
void recover(simulation_t *simulation);
void vaccinate(simulation_t *simulation);
void susceptible(simulation_t *simulation);
void die(simulation_t *simulation);
void print_agents(const simulation_t *simulation);
void report_header(void);
void report(const simulation_t *simulation, size_t iteration);
void simulate(simulation_t *simulation);
void set_default_parameters(parameters_t *p);

#endif
