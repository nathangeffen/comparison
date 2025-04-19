/**
 * Simple agent-based simulation of an infectious disease.
 */
#define _GNU_SOURCE
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <glib.h>

#include "abm.h"

#define NUM_RANDS 10000000

static uint64_t rngarr[NUM_RANDS]  = {0};

void read_randoms(void)
{
		size_t n = 0;
           char *line = NULL;
           size_t len = 0;

		   FILE *stream = fopen("../rand.txt", "r");

           if (stream == NULL) {
               perror("fopen");
               exit(EXIT_FAILURE);
           }

           while (getline(&line, &len, stream) != -1 && n < NUM_RANDS) {
				rngarr[n] = atoll(line);
				printf("%zu %lu\n", n, rngarr[n]);
				++n;
           }
           free(line);
		   fclose(stream);
}

void rng_init(rng_t *rng, uint64_t seed, bool from_file)
{
		if (from_file == false) {
				rng->seed = seed;
		}
		else {
				rng->seed = seed * 500000;
		}
}

const uint64_t m = 32768;

uint64_t rng_uint64_t(rng_t *rng)
{

    rng->seed = rng->seed * 1103515245 + 12345;
    return (rng->seed/65536) % m;
}

uint64_t rng_to(rng_t *rng, uint64_t max)
{
		assert(max > 0);
		uint64_t result = rng_uint64_t(rng) % max;
		return result;
}


double rng_double(rng_t *rng)
{
		double result = (double) rng_uint64_t(rng) / m;
		return result;
}

/**
 *   Possible agent states
 */
char state_char(state_t state) {
		switch (state) {
				case SUSCEPTIBLE:
						return 'S';
				case INFECTIOUS:
						return 'I';
				case RECOVERED:
						return 'R';
				case VACCINATED:
						return 'V';
				case DEAD:
						return 'D';
				default:
						return '_';
		}
}

agent_t new_agent(int identity, state_t state) {
		agent_t a;
		a.identity = identity;
		a.state = state;
		return a;
}

void shuffle(agent_t *agents, size_t n, rng_t *rng)
{
		for (size_t i = n - 1; i > 0; --i) {
				size_t j = rng_to(rng, i + 1);
				agent_t t = agents[j];
				agents[j] = agents[i];
				agents[i] = t;
		}
}

size_t count_if_state(const agent_t * restrict agents, size_t n, state_t state)
{
		size_t total = 0;
		for (const agent_t *a = agents; a < agents + n; a++)
				if (a->state == state)
						++total;
		return total;
}

size_t count_if_not_state(const agent_t * restrict agents, size_t n, state_t state)
{
		size_t total = count_if_state(agents, n, state);
		return n - total;
}

statistics_t get_stats(const agent_t * restrict agents, size_t n)
{
		statistics_t results;
		results.susceptible = count_if_state(agents, n, SUSCEPTIBLE);
		results.infectious = count_if_state(agents, n, INFECTIOUS);
		results.recovered = count_if_state(agents, n, RECOVERED);
		results.vaccinated = count_if_state(agents, n, VACCINATED);
		results.dead = count_if_state(agents, n, DEAD);
		return results;
}

/// Returns a new simulation
simulation_t new_simulation(size_t identity, const parameters_t *parameters) {
		simulation_t s;
		s.parameters = *parameters;
		s.identity = identity;
		rng_init(&s.rng, identity, parameters->random_file);
		size_t n = parameters->agents * 3 / 2;
		n = n < 10 ? 10 : n;
		s.agents = malloc(n * sizeof(*s.agents));
		if (s.agents == NULL) {
				fprintf(stderr, "Error, can't allocate agents: %s %d\n",
								__FILE__, __LINE__);
				exit(EXIT_FAILURE);
		}
		s.size = parameters->agents;
		s.capacity = n;
		for (size_t i = 0; i < s.size; i++)
				s.agents[i] = new_agent(i, SUSCEPTIBLE);
		shuffle(s.agents, s.size, &s.rng);
		for (size_t i = 0; i < parameters->infectious; i++)
				s.agents[i].state = INFECTIOUS;
		s.total_infections = parameters->infectious;
		s.infection_deaths = 0;
		return s;
}

/// Event to grow the number of agents.
void grow(simulation_t * restrict simulation)
{
		int num_agents = count_if_not_state(simulation->agents,
						simulation->size, DEAD);
		int new_agents = round(simulation->parameters.growth * num_agents);
		if (simulation->size + new_agents > simulation->capacity) {
				simulation->capacity *= 3 / 2;
				agent_t *t = realloc(simulation->agents, simulation->capacity);
				if (t == NULL) {
						fprintf(stderr, "Error, can't reallocate agents: %s %d\n", __FILE__, __LINE__);
						exit(EXIT_FAILURE);
				} else {
						simulation->agents = t;
				}
		}
		int id = simulation->size;
		for (int i = 0; i < new_agents; i++, id++)
				simulation->agents[id] = new_agent(id, SUSCEPTIBLE);
		simulation->size += new_agents;
}

/**
 * Intentionally time-consuming event to infect agents.  Agents
 * randomly encounter one another. If an infected agent
 * encounters a susceptible one, an infection takes place.
 */
void infect_method_one(simulation_t * restrict simulation)
{
		for (int i = 0; i < simulation->parameters.encounters; i++) {
				int ind1 = rng_to(&simulation->rng, simulation->size);
				int ind2 = rng_to(&simulation->rng, simulation->size);
				if (simulation->agents[ind1].state == SUSCEPTIBLE &&
								simulation->agents[ind2].state == INFECTIOUS) {
						simulation->agents[ind1].state = INFECTIOUS;
						++simulation->total_infections;
				} else if (simulation->agents[ind1].state == INFECTIOUS &&
								simulation->agents[ind2].state == SUSCEPTIBLE) {
						simulation->agents[ind2].state = INFECTIOUS;
						++simulation->total_infections;
				}
		}
}

void get_indices(const simulation_t * restrict simulation,
				size_t ** restrict indices, size_t * restrict n, state_t state, size_t max)
{
		*n = 0;
		*indices = malloc(sizeof(**indices) * max);
		if (*indices == NULL) {
				fprintf(stderr, "Error, can't allocate indices: %s %d\n",
								__FILE__, __LINE__);
				exit(EXIT_FAILURE);
		}
		for (size_t i = 0; i < simulation->size; i++) {
				if (i >= max) break;
				if (simulation->agents[i].state == state) {
						(*indices)[*n] = i;
						++(*n);
				}
		}
}

void infect_method_two(simulation_t * restrict simulation)
{
		size_t *indices;
		size_t n;

		get_indices(simulation, &indices, &n, SUSCEPTIBLE,
						simulation->parameters.encounters);
		shuffle(simulation->agents, simulation->size, &simulation->rng);

		for (size_t i = 0; i < n; i++) {
				if (simulation->agents[i].state == INFECTIOUS) {
						simulation->agents[indices[i]].state = INFECTIOUS;
						++simulation->total_infections;
				}
		}
		free(indices);
}

void recover(simulation_t * restrict simulation)
{
		for (agent_t *agent = simulation->agents;
						agent < simulation->agents + simulation->size;
						agent++) {
				if (agent->state == INFECTIOUS)
						if (rng_double(&simulation->rng) < simulation->parameters.recovery_prob)
								agent->state = RECOVERED;
		}
}

void vaccinate(simulation_t * restrict simulation)
{
		for (agent_t *agent = simulation->agents;
						agent < simulation->agents + simulation->size;
						agent++) {
				if (agent->state == SUSCEPTIBLE)
						if (rng_double(&simulation->rng) < simulation->parameters.vaccination_prob)
								agent->state = VACCINATED;
		}
}

void susceptible(simulation_t * restrict simulation)
{
		for (agent_t *agent = simulation->agents;
						agent < simulation->agents + simulation->size;
						agent++) {
				if (agent->state == VACCINATED || agent->state == RECOVERED)
						if (rng_double(&simulation->rng) < simulation->parameters.regression_prob)
								agent->state = SUSCEPTIBLE;
		}
}


/// Simple death event that differentiates between infected and susceptible
/// agents.
void die(simulation_t * restrict simulation)
{
		for (agent_t *agent = simulation->agents;
						agent < simulation->agents + simulation->size; agent++) {
				if (agent->state == SUSCEPTIBLE) {
						if (rng_double(&simulation->rng) < simulation->parameters.death_prob_susceptible) {
								agent->state = DEAD;
						}
				} else if (agent->state == INFECTIOUS) {
						if (rng_double(&simulation->rng) < simulation->parameters.death_prob_infectious) {
								agent->state = DEAD;
								simulation->infection_deaths++;
						}
				}
		}
}

static int cmp_agents(const void * restrict a, const void * restrict b)
{
		agent_t *x = (agent_t *) a;
		agent_t *y = (agent_t *) b;
		return (x->identity < y->identity) ? -1 : ( (x->identity == y->identity) ? 0 : 1);
}

void print_agents(const simulation_t * restrict simulation)
{
		qsort(simulation->agents, simulation->size, sizeof(agent_t), cmp_agents);
		FILE *f = fopen(simulation->parameters.agent_filename, "w");
		if (f == NULL) {
				fprintf(stderr, "Cannot open agent file\n");
				exit(EXIT_FAILURE);
		}
		const size_t n = simulation->size;
		for (const agent_t *a = simulation->agents; a < simulation->agents + n; ++a) {
				if (fprintf(f, "%zu,%c\n", a->identity, state_char(a->state)) < 0) {
						fprintf(stderr, "Error writing to agent file.\n");
						exit(EXIT_FAILURE);
				}
		}
		fclose(f);
}

void report_header(void)
{
		printf("#,iter,S,I,R,V,D,TI,TID\n");
}

/// Prints out the vital statistics.
void report(const simulation_t * restrict simulation, size_t iteration)
{
		statistics_t s = get_stats(simulation->agents, simulation->size);
		// We want to send one string to puts to prevent data races on stdout
		char output[250];
		snprintf(output, 250, "%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu",
						simulation->identity, iteration,
						s.susceptible, s.infectious, s.recovered, s.vaccinated,
						s.dead, simulation->total_infections, simulation->infection_deaths);
		puts(output);
		if (simulation->parameters.output_agents > 0) {
				if (iteration > 0 && iteration % simulation->parameters.output_agents == 0)
						print_agents(simulation);
		}
}

/** Simulation engine that repeatedly executes all the events
 *  for specified number of iterations.
 */
void simulate(simulation_t * restrict simulation) {
		if (simulation->identity == 0)
				report_header();
		report(simulation, 0);
		for (int i = 0; i < simulation->parameters.iterations; i++) {
				grow(simulation);
				if (simulation->parameters.infection_method == BOTH) {
						if (simulation->identity % 2 == 0) {
								infect_method_one(simulation);
						} else {
								infect_method_two(simulation);
						}
				} else if (simulation->parameters.infection_method == ONE) {
						infect_method_one(simulation);
				} else {
						infect_method_two(simulation);
				}
				recover(simulation);
				vaccinate(simulation);
				susceptible(simulation);
				die(simulation);
				if (i != 0 && i % 100 == 0)
						report(simulation, i);
		}
		report(simulation, simulation->parameters.iterations);
}

void set_default_parameters(parameters_t *p)
{
		p->simulations = 20;
		p->identity = 0;
		p->iterations = 365 * 4;
		p->agents = 10000;
		p->infectious = 10;
		p->encounters = 100;
		p->growth = 0.0001;
		p->death_prob_susceptible = 0.0001;
		p->death_prob_infectious = 0.001;
		p->recovery_prob = 0.01;
		p->vaccination_prob = 0.001;
		p->regression_prob = 0.0003;
		p->infection_method = BOTH;
		p->output_agents = 0;
		strcpy(p->agent_filename, "agents.csv");
		p->random_file = false;
}

