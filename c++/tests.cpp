#define BOOST_TEST_MODULE fit_tests
#include <boost/process.hpp>
#include <boost/test/included/unit_test.hpp>

#include "abm.hpp"

BOOST_AUTO_TEST_CASE(simulation_test_example) {
  Parameters parameters;
  Simulation simulation(4, parameters);
  BOOST_TEST(simulation.identity == 4);
  BOOST_TEST(simulation.agents.size() == 10000);
  simulation.simulate();
  BOOST_TEST(simulation.agents.size() > 10000);
}
