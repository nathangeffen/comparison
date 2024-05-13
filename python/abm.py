#!/bin/python
"""
Simple agent-based simulation of an infectious disease.
"""

from multiprocessing import Pool
from enum import Enum
import random
import argparse


class State(Enum):
    """ Maintains the possible states of an agent.
    """
    SUSCEPTIBLE = 0
    INFECTED = 1
    DEAD = 2


class Agent:
    """ Holds an agent who has two attributes, a unique identity and a state.
    """
    def __init__(self, identity, state):
        self.identity = identity
        self.state = state


class Simulation:
    """ Simple simulation engine with a few events.
    """

    def __init__(self, identity, num_agents, num_infections):
        """Initializes the simulation with a unique identity, initial number of
        agents and initial number of infections.
        """
        self.identity = identity
        self.agents = [Agent(i, State.SUSCEPTIBLE) for i in range(num_agents)]
        for i in range(num_infections):
            self.agents[i].state = State.INFECTED
        random.shuffle(self.agents)

    def grow(self, growth_per_day):
        """ Event to grow the number of agents.
        """
        num_agents = len([agent for agent in self.agents if agent.state != State.DEAD])
        new_agents = growth_per_day * num_agents
        identity = len(self.agents)
        for _ in range(round(new_agents)):
            agent = Agent(identity, State.SUSCEPTIBLE)
            self.agents.append(agent)
            identity += 1


    def infect(self, events):
        """ Intentionally time-consuming event to infect agents.
        Agents randomly encounter one another. If an infected
        agent encounters a susceptible one, an
        infection takes place.
        """
        for _ in range(events):
            ind1 = random.randint(0, len(self.agents) - 1)
            ind2 = random.randint(0, len(self.agents) - 1)
            if self.agents[ind1].state == State.SUSCEPTIBLE and \
               self.agents[ind2].state == State.INFECTED:
                self.agents[ind1].state = State.INFECTED
            elif self.agents[ind2].state == State.SUSCEPTIBLE and \
                 self.agents[ind1].state == State.INFECTED:
                self.agents[ind2].state = State.INFECTED


    def die(self, death_rate_susceptible, death_rate_infected):
        """ Simple death event that differentiates between infected and
        susceptible agents.
        """
        for agent in self.agents:
            if agent.state == State.SUSCEPTIBLE:
                if random.random() < death_rate_susceptible:
                    agent.state = State.DEAD
            elif agent.state == State.INFECTED:
                if random.random() < death_rate_infected:
                    agent.state = State.DEAD

    def report(self, iteration):
        """ Prints out the vital statistics.
        """
        num_susceptible = len([agent for agent in self.agents if agent.state == State.SUSCEPTIBLE])
        num_infections = len([agent for agent in self.agents if agent.state == State.INFECTED])
        num_deaths = len([agent for agent in self.agents if agent.state == State.DEAD])
        s = f"Simulation: {self.identity}. " \
            f"Iteration: {iteration}. " \
            f"Susceptible: {num_susceptible}. " \
            f"Infections: {num_infections}. " \
            f"Deaths: {num_deaths}."
        print(s)

    def simulate(self, iterations, growth_per_day, events, death_rate_susceptible, death_rate_infected):
        """ Simulation engine that repeatedly executes all the events for
        specified number of iterations.
        """
        for i in range(iterations):
            self.grow(growth_per_day)
            self.infect(events)
            self.die(death_rate_susceptible, death_rate_infected)
            if i % 100 == 0:
                self.report(i)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='abm',
        description='Simple agent based model')

    parser.add_argument('--simulations', type=int, default=10,
                        help='number of simulations')
    parser.add_argument('--iterations', type=int, default=365*4,
                        help='number of iterations')
    parser.add_argument('--infections', type=int, default=10,
                        help='initial infections')
    parser.add_argument('--agents', type=int, default=10000,
                        help='number of agents')
    parser.add_argument('--events', type=int, default=20,
                        help='number of potential infections per iteration to simulate')
    parser.add_argument('--growth', type=float, default=0.0001,
                        help='population growth per iteration')
    parser.add_argument('--death_rate_susceptible', type=float, default=0.0001,
                        help='death rate for susceptible agents per iteration')
    parser.add_argument('--death_rate_infected', type=float, default=0.001,
                        help='death rate for infected agents per iteration')
    args = parser.parse_args()

    def run_simulation(num):
        """ Runs a single simulation. Invoked from the multiprocessing pool.
        """
        simulation = Simulation(num, args.agents, args.infections)
        simulation.simulate(args.iterations, args.growth, args.events,
                            args.death_rate_susceptible,
                            args.death_rate_infected)
        simulation.report(args.iterations)

    with Pool() as p:
        p.map(run_simulation, range(args.simulations))
