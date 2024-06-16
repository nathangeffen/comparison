#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "abm.h"

parameters_t process_arguments(int argc, char **argv)
{
		parameters_t result; // Global parameters
		set_default_parameters(&result);
		GOptionEntry entries[] =
		{
				{ "simulations", 's', 0, G_OPTION_ARG_INT, &result.simulations,
						"Number of simulations", "N" },
				{ "identity", 0, 0, G_OPTION_ARG_INT, &result.identity,
						"Id number of simulation (if running only one) ", "N" },
				{ "iterations", 0, 0, G_OPTION_ARG_INT, &result.iterations,
						"Number of iterations in a simulation", "N" },
				{ "agents", 'a', 0, G_OPTION_ARG_INT, &result.agents,
						"Number of initial agents", "N" },
				{ "infections", 0, 0, G_OPTION_ARG_INT, &result.infectious,
						"Number of initial agents who are infectious"},
				{ "encounters", 'e', 0, G_OPTION_ARG_INT, &result.encounters,
						"Number of encounters between agents in the infection methods"},
				{ "growth", 'g', 0, G_OPTION_ARG_DOUBLE, &result.growth,
						"Growth rate of the agent population per iteration"},
				{ "death_prob_susceptible", 0, 0, G_OPTION_ARG_DOUBLE, &result.death_prob_susceptible,
						"Death prob of a susceptible agent population per iteration"},
				{ "death_prob_infectious", 0, 0, G_OPTION_ARG_DOUBLE, &result.death_prob_infectious,
						"Death prob of an infectious agent population per iteration"},
				{ "recover", 'r', 0, G_OPTION_ARG_DOUBLE, &result.recovery_prob,
						"Prob of infectious agent moving to recovery state per iteration"},
				{ "vaccination_prob", 'v', 0, G_OPTION_ARG_DOUBLE, &result.vaccination_prob,
						"Prob of susceptible agent moving to vaccinated state per iteration"},
				{ "regression_prob", 0, 0, G_OPTION_ARG_DOUBLE, &result.regression_prob,
						"Prob of recovered or vaccinated agent becoming susceptible per iteration"},
				{ "infection_method", 0, 0, G_OPTION_ARG_INT, &result.infection_method,
						"Infection method to use (0 = BOTH, 1 = ONE, 2 = TWO)"},
				{ "output_agents", 0, 0, G_OPTION_ARG_INT, &result.output_agents,
						"Iteration frequency to write out agents (0 = never)"},
				{ "agent_filename", 0, 0, G_OPTION_ARG_FILENAME, &result.agent_filename,
						"Agent output file name"},
				G_OPTION_ENTRY_NULL
		};

		GError *error = NULL;
		GOptionContext *context;

		context = g_option_context_new("Agent based model for testing programming environments");
		g_option_context_add_main_entries(context, entries, NULL);

		if (!g_option_context_parse (context, &argc, &argv, &error))
		{
				fprintf(stderr, "Option parsing failed: %s\n", error->message);
				exit(EXIT_FAILURE);
		}
		g_option_context_free(context);

		return result;
}

/**
 * Runs one simulation.
 */
void run_one_simulation(gpointer data, gpointer user_data) {
		size_t *i = (size_t *) data;
		parameters_t *p = (parameters_t *)user_data;
		simulation_t simulation = new_simulation((size_t) *i, p);
		simulate(&simulation);
		free(simulation.agents);
}

/**
 * Gets command line arguments and then runs the simulations in a
 * parallel pool.
 */
int main(int argc, char **argv)
{
		GThreadPool *pool;
		GError *error = NULL;

		parameters_t parameters = process_arguments(argc, argv);

		size_t *arr = malloc(sizeof(size_t) * parameters.simulations);
		if (arr == NULL) {
				fprintf(stderr, "Out of memory: %s %d", __FILE__,
								__LINE__);
				exit(EXIT_FAILURE);
		}
		g_thread_pool_set_max_unused_threads(0);
		pool = g_thread_pool_new((GFunc)run_one_simulation, (gpointer) &parameters,
						g_get_num_processors(), true, &error);
		if (error != NULL) {
				fprintf(stderr, "Problem initializing thread pool: %s %d\n", __FILE__,
								__LINE__);
				exit(EXIT_FAILURE);
		}
		for (size_t i = 0; i < parameters.simulations; ++i) {
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
		g_thread_pool_stop_unused_threads();

		return 0;
}

