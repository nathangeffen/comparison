#include <boost/asio.hpp>

#include "abm.hpp"

#include "CLI11.hpp"


/// Gets command line arguments and then runs the simulations in a parallel pool.
int main(int argc, char **argv) {
  Parameters parameters;
  int identity = 0;
  CLI::App app{"Agent based model in C++"};
  argv = app.ensure_utf8(argv);

  app.add_option("-s,--simulations", parameters.simulations,
		 "Number of simulations");
  app.add_option("--identity", identity,
		 "Id number of simulation (if running only one)");
  app.add_option("-i,--iterations", parameters.iterations,
		 "Number of iterations in a simulation");
  app.add_option("-a,--agents", parameters.agents,
		 "Number of initial agents");
  app.add_option("--infections", parameters.infections,
		 "Number of initial agents who are infectious");
  app.add_option("-e,--encounters", parameters.encounters,
		 "Number of encounters between agents in the infection methods");
  app.add_option("-g,--growth", parameters.growth,
		"Growth rate of the agent population per iteration");
  app.add_option("--death_prob_susceptible", parameters.death_prob_susceptible,
		 "Death prob of a susceptible agent population per iteration");
  app.add_option("--death_prob_infectious", parameters.death_prob_infectious,
		 "Death prob of an infectious agent population per iteration");
  app.add_option("-r,--recovery_prob", parameters.recovery_prob,
		 "Prob of infectious agent moving to recovery state per iteration");
  app.add_option("-v,--vaccination_prob", parameters.vaccination_prob,
		 "Prob of susceptible agent moving to vaccinated state per iteratio");
  app.add_option("--regression_prob", parameters.regression_prob,
		 "Prob of recovered or vaccinated agent becoming susceptible per iteration");
  app.add_option("--infection_method", parameters.infection_method,
		 "Infection method to use (0 = BOTH, 1 = ONE, 2 = TWO)");
  app.add_option("--output_agents", parameters.output_agents,
		 "Iteration frequency to write out agents (0 = never)");
  app.add_option("--agent_filename", parameters.agent_filename,
		 "Agent output file name");

  CLI11_PARSE(app, argc, argv);

  if (parameters.simulations <= 1) {
	Simulation simulation(identity, parameters);
	simulation.simulate();
  } else {
    boost::asio::thread_pool pool(std::thread::hardware_concurrency());
    for (int i = 0; i < parameters.simulations; i++) {
      boost::asio::post(pool, [i, &parameters]() {
	Simulation simulation(i, parameters);
	simulation.simulate();
      });
    }
    pool.join();
  }
  return 0;
}
