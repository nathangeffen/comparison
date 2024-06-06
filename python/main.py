#!/bin/python
"""
Simple agent-based simulation of an infectious disease.
"""

# Copyright 2024 Nathan Geffen

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http:#www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This program implements a simple agent based model for the purpose of
# comparing programming languages.

from multiprocessing import Pool
import argparse
from abm import Parameters, Simulation, InfectionMethod

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='abm',
        description='Simple agent based model')

    parser.add_argument('--simulations', type=int, default=20,
                        help='number of simulations')
    parser.add_argument('--iterations', type=int, default=365*4,
                        help='number of iterations')
    parser.add_argument('--agents', type=int, default=10000,
                        help='number of agents')
    parser.add_argument('--infections', type=int, default=10,
                        help='initial infections')
    parser.add_argument('--identity', type=int, default=0,
                        help='initial infections')
    parser.add_argument('--encounters', type=int, default=100,
                        help='number of potential infections per '
                        'iteration to simulate')
    parser.add_argument('--growth', type=float, default=0.0001,
                        help='population growth per iteration')
    parser.add_argument('--death_prob_susceptible', type=float, default=0.0001,
                        help='death prob for susceptible agents per iteration')
    parser.add_argument('--death_prob_infectious', type=float, default=0.001,
                        help='death prob for infectious agents per iteration')
    parser.add_argument('--recovery_prob', type=float, default=0.01,
                        help='Prob of infectious agent moving '
                        'to recovery state per iteration')
    parser.add_argument('--vaccination_prob', type=float, default=0.001,
                        help='Prob of susceptible agent moving '
                        'to vaccinated state per iteration')
    parser.add_argument('--regression_prob', type=float, default=0.0003,
                        help='Prob of recovered or vaccinated agent moving '
                        'to susceptible state per iteration')
    parser.add_argument('--infection_method', type=int, default=0,
                        help='Infection method to use '
                        '(0 = BOTH, 1 = ONE, 2 = TWO)')
    parser.add_argument('--output_agents', type=int, default=0,
                        help='Iteration frequency to write out agents '
                        '(0 = never)')
    parser.add_argument('--agent_filename', type=str, default="agents.csv",
                        help='Agent output file name')

    args = parser.parse_args()
    parameters = Parameters()
    parameters.simulations = args.simulations
    parameters.iterations = args.iterations
    parameters.agents = args.agents
    parameters.infections = args.infections
    parameters.identity = args.identity
    parameters.encounters = args.encounters
    parameters.growth = args.growth
    parameters.death_prob_susceptible = args.death_prob_susceptible
    parameters.death_prob_infectious = args.death_prob_infectious
    parameters.recovery_prob = args.recovery_prob
    parameters.vaccination_prob = args.vaccination_prob
    parameters.regression_prob = args.regression_prob

    if args.infection_method == 0:
        parameters.infection_method = InfectionMethod.BOTH
    elif args.infection_method == 1:
        parameters.infection_method = InfectionMethod.ONE
    else:
        parameters.infection_method = InfectionMethod.TWO

    parameters.output_agents = args.output_agents
    parameters.agent_filename = args.agent_filename

    def run_simulation(sim_num):
        """ Runs a single simulation. Invoked from the multiprocessing pool.
        """
        simulation = Simulation(sim_num, parameters)
        simulation.simulate()

    with Pool() as p:
        p.map(run_simulation, range(args.simulations))
