/**
 * Simple agent-based simulation of an infectious disease.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

#include <glib.h>

_Thread_local gsl_rng *rng;

void rng_alloc(int seed) {
        int s = seed * (gsl_rng_default_seed + 1);
        const gsl_rng_type *T;
        T = gsl_rng_default;
        rng = gsl_rng_alloc(T);
        gsl_rng_set(rng, s);
}

/**
 *   Possible agent states
 */
enum state {
        SUSCEPTIBLE = 0,
        INFECTIOUS,
        RECOVERED,
        VACCINATED,
        DEAD,
};

typedef enum state state_t;

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
        }
}

/// Determines which infection event to call in the simulation
enum infection_method {
        BOTH = 0,
        ONE = 1,
        TWO = 2,
};

typedef enum infection_method infection_method_t;

/// Structure to hold simulation's parameters.
struct parameters {
        int simulations;
        int iterations;
        size_t agents;
        size_t infections;
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
};

typedef struct parameters parameters_t;

/// Holds an agent who has two attributes, a unique identity and a state.
struct agent {
        int identity;
        state_t state;
};

typedef struct agent agent_t;

agent_t new_agent(int identity, state_t state) {
        agent_t a;
        a.identity = identity;
        a.state = state;
        return a;
}

/// This is used to represent a snapshot of stats for a simulation.
struct statistics {
        size_t susceptible;
        size_t infectious;
        size_t recovered;
        size_t vaccinated;
        size_t dead;
};

typedef struct statistics statistics_t;

size_t count_if_state(const agent_t *agents, size_t n, state_t state)
{
        size_t total = 0;
        for (const agent_t *a = agents; a < agents + n; a++)
                if (a->state == state)
                        ++total;
        return total;
}

size_t count_if_not_state(const agent_t *agents, size_t n, state_t state)
{
        size_t total = count_if_state(agents, n, state);
        return n - total;
}

statistics_t get_stats(const agent_t *agents, size_t n)
{
        statistics_t results;
        results.susceptible = count_if_state(agents, n, SUSCEPTIBLE);
        results.infectious = count_if_state(agents, n, INFECTIOUS);
        results.recovered = count_if_state(agents, n, RECOVERED);
        results.vaccinated = count_if_state(agents, n, VACCINATED);
        results.dead = count_if_state(agents, n, DEAD);
        return results;
}

/// Simple simulation engine with a few events.
struct simulation {
        int identity;    // Unique id of this simulation
        agent_t *agents; // Array holding the simulation's agents
        int size;
        int capacity;
};

typedef struct simulation simulation_t;

/// Returns a new simulation
simulation_t new_simulation(int identity, const parameters_t *parameters) {
        simulation_t s;
        s.identity = identity;
        size_t n = parameters->agents * 3 / 2;
        n = n < 10 ? 10 : n;
        s.agents = malloc(n * sizeof(*s.agents));
        if (s.agents == NULL) {
                fprintf(stderr, "Error, can't allocate agents: %s %d\n",
                                __FILE__, __LINE__);
                exit(EXIT_FAILURE);
        }
        s.size = n_agents;
        s.capacity = n;
        for (int i = 0; i < s.size; i++)
                s.agents[i] = new_agent(i, SUSCEPTIBLE);
        for (int i = 0; i < n_infections; i++)
                s.agents[i].state = INFECTIOUS;
        gsl_ran_shuffle(rng, s.agents, s.size, sizeof(agent_t));
        return s;
}

/// Event to grow the number of agents.
void grow(simulation_t *simulation, double growth_per_day) {
        int num_agents = count_if_not_state(simulation->agents,
                        simulation->size, DEAD);
        int new_agents = round(growth_per_day * num_agents);
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
void infect(simulation_t *simulation, int events) {
        for (int i = 0; i < events; i++) {
                int ind1 = gsl_rng_uniform_int(rng, simulation->size);
                int ind2 = gsl_rng_uniform_int(rng, simulation->size);
                if (simulation->agents[ind1].state == SUSCEPTIBLE &&
                                simulation->agents[ind2].state == INFECTIOUS) {
                        simulation->agents[ind1].state = INFECTIOUS;
                } else if (simulation->agents[ind1].state == INFECTIOUS &&
                                simulation->agents[ind2].state == SUSCEPTIBLE) {
                        simulation->agents[ind2].state = INFECTIOUS;
                }
        }
}

/// Simple death event that differentiates between infected and susceptible
/// agents.
void die(simulation_t *simulation, double death_rate_susceptible,
                double death_rate_infected) {
        for (agent_t *agent = simulation->agents;
                        agent < simulation->agents + simulation->size; agent++) {
                if (agent->state == SUSCEPTIBLE) {
                        if (gsl_rng_uniform(rng) < death_rate_susceptible) {
                                agent->state = DEAD;
                        }
                } else if (agent->state == INFECTIOUS) {
                        if (gsl_rng_uniform(rng) < death_rate_infected) {
                                agent->state = DEAD;
                        }
                }
        }
}

/// Prints out the vital statistics.
void report(const simulation_t *simulation, int iteration) {
        int num_susceptible = count_agents_by_state(simulation, SUSCEPTIBLE);
        int num_infections = count_agents_by_state(simulation, INFECTIOUS);
        int num_deaths = count_agents_by_state(simulation, DEAD);
        // We want to send one string to puts to prevent data races on stdout
        char output[250];
        snprintf(output, 250,
                        "Simulation: %d. Iteration: %d. Susceptible: %d. Infections: %d. "
                        "Deaths: %d\n",
                        simulation->identity, iteration, num_susceptible, num_infections,
                        num_deaths);
        puts(output);
}

/** Simulation engine that repeatedly executes all the events
 *  for specified number of iterations.
 */
void simulate(simulation_t *simulation, int iterations, double growth_per_day,
                int events, double death_rate_susceptible,
                double death_rate_infected) {
        for (int i = 0; i < iterations; i++) {
                grow(simulation, growth_per_day);
                infect(simulation, events);
                die(simulation, death_rate_susceptible, death_rate_infected);
                if (i % 100 == 0)
                        report(simulation, i);
        }
}

/**
 * For processing command line arguments
 */
static struct option long_options[] = {
        {"simulations", required_argument, 0, 0},
        {"iterations", required_argument, 0, 0},
        {"infections", required_argument, 0, 0},
        {"agents", required_argument, 0, 0},
        {"growth", required_argument, 0, 0},
        {"events", required_argument, 0, 0},
        {"death_rate_susceptible", required_argument, 0, 0},
        {"death_rate_infected", required_argument, 0, 0},
        {0, 0, 0, 0}};

/**
 * Structure to hold simulation's parameters.
 */
struct parameters {
        int simulations;
        int iterations;
        int infections;
        int agents;
        int events;
        double growth;
        double death_rate_susceptible;
        double death_rate_infected;
};

typedef struct parameters parameters_t;

/**
 * Prints help screen if -h command line options specified.
 */
void print_help(const char *prog) {
        struct option *it = long_options;

        fprintf(stderr, "%s [options]\n", prog);
        while (it->name != 0) {
                fprintf(stderr, "%s  <value>\n", it->name);
                ++it;
        }
}

/**
 * Process command line arguments and return parameters.
 */
parameters_t process_arguments(int argc, char **argv) {
        parameters_t result;
        result.simulations = 10;
        result.iterations = 365 * 4;
        result.infections = 10;
        result.agents = 10000;
        result.events = 20;
        result.growth = 0.0001;
        result.death_rate_susceptible = 0.0001;
        result.death_rate_infected = 0.001;
        while (1) {
                int option_index = 0;

                int c = getopt_long(argc, argv, "h", long_options, &option_index);

                if (c == -1)
                        break;

                if (c == 0) {
                        const char *s = long_options[option_index].name;
                        if (strcmp(s, "simulations") == 0) {
                                result.simulations = atoi(optarg);
                        } else if (strcmp(s, "iterations") == 0) {
                                result.iterations = atoi(optarg);
                        } else if (strcmp(s, "infections") == 0) {
                                result.infections = atoi(optarg);
                        } else if (strcmp(s, "agents") == 0) {
                                result.agents = atoi(optarg);
                        } else if (strcmp(s, "events") == 0) {
                                result.events = atoi(optarg);
                        } else if (strcmp(s, "growth") == 0) {
                                result.growth = atof(optarg);
                        } else if (strcmp(s, "death_rate_susceptible") == 0) {
                                result.death_rate_susceptible = atof(optarg);
                        } else if (strcmp(s, "death_rate_infected") == 0) {
                                result.death_rate_infected = atof(optarg);
                        } else {
                                fprintf(stderr, "Unknown options: %s\n", s);
                                exit(EXIT_FAILURE);
                        }
                }
                if (c == 'h') {
                        print_help(argv[0]);
                        exit(EXIT_SUCCESS);
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
void run_one_simulation(gpointer data, gpointer user_data) {
        int *i = (int *)data;
        rng_alloc(*i + 3);
        parameters_t *p = (parameters_t *)user_data;

        simulation_t simulation = new_simulation(*i, p->agents, p->infections);
        simulate(&simulation, p->iterations, p->growth, p->events,
                        p->death_rate_susceptible, p->death_rate_infected);
        report(&simulation, p->iterations);
        free(simulation.agents);
        gsl_rng_free(rng);
}

/**
 * Gets command line arguments and then runs the simulations in a
 * parallel pool.
 */
int main(int argc, char **argv) {
        parameters_t parameters = process_arguments(argc, argv);

        GThreadPool *pool;
        GError *error = NULL;

        pool = g_thread_pool_new((GFunc)run_one_simulation, (gpointer)&parameters,
                        g_get_num_processors(), true, &error);
        if (error != NULL) {
                fprintf(stderr, "Problem initializing thread pool: %s %d\n", __FILE__,
                                __LINE__);
                exit(EXIT_FAILURE);
        }
        int *arr = malloc(sizeof(int) * parameters.simulations);
        if (arr == NULL) {
                fprintf(stderr, "Memory allocation error: %s %d\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
        }
        for (int i = 0; i < parameters.simulations; ++i) {
                *(arr + i) = i;
                g_thread_pool_push(pool, arr + i, &error);
        }
        if (error != NULL) {
                fprintf(stderr, "Problem executing thread pool: %s %d\n", __FILE__,
                                __LINE__);
                exit(EXIT_FAILURE);
        }
        g_thread_pool_free(pool, false, true);
        free(arr);
        return 0;
}
