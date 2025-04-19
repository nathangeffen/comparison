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

# This class implements a simple agent-based model simulation engine
# for the purpose of comparing programming environments.


from enum import Enum, auto
import random


class State(Enum):
    """ Maintains the possible states of an agent.
    """
    SUSCEPTIBLE = auto()
    INFECTIOUS = auto()
    RECOVERED = auto()
    VACCINATED = auto()
    DEAD = auto()

    def __str__(self):
        return self.name[0]


class InfectionMethod(Enum):
    """ Determines which infection event to call in the simulation
    """
    BOTH = 0
    ONE = 1
    TWO = 2


class Parameters:

    def __init__(self):
        self.simulations = 20
        self.agents = 10000
        self.infections = 10
        self.identity = 0
        self.iterations = 365 * 4
        self.encounters = 10
        self.growth = 0.0001
        self.death_prob_susceptible = 0.0001
        self.death_prob_infectious = 0.001
        self.recovery_prob = 0.01
        self.vaccination_prob = 0.001
        self.regression_prob = 0.0003
        self.infection_method = InfectionMethod.BOTH
        self.output_agents = 0
        self.agent_filename = "agents.csv"


class Agent:
    """ Holds an agent who has two attributes, a unique identity and a state.
    """
    def __init__(self, identity, state):
        self.identity = identity
        self.state = state


class Simulation:
    """ Simple simulation engine with a few events.
    """

    def __init__(self, sim_num, parameters):
        """Initializes the simulation with a unique identity, initial number of
        agents and initial number of infections.
        """
        self.identity = sim_num
        self.parameters = parameters
        self.agents = [Agent(i, State.SUSCEPTIBLE)
                       for i in range(parameters.agents)]
        random.shuffle(self.agents)
        for i in range(parameters.infections):
            self.agents[i].state = State.INFECTIOUS
        self.total_infections = parameters.infections
        self.infection_deaths = 0

    def grow(self):
        """ Event to grow the number of agents.
        """
        num_agents = len([agent for agent in self.agents
                          if agent.state != State.DEAD])
        new_agents = self.parameters.growth * num_agents
        identity = len(self.agents)
        for _ in range(round(new_agents)):
            agent = Agent(identity, State.SUSCEPTIBLE)
            self.agents.append(agent)
            identity += 1

    def infect_method_one(self):
        """ Simulation event that infects agents (1st of 2 methods implemented.
        """
        for _ in range(self.parameters.encounters):
            ind1 = random.randint(0, len(self.agents) - 1)
            ind2 = random.randint(0, len(self.agents) - 1)
            if (self.agents[ind1].state == State.SUSCEPTIBLE) and \
               (self.agents[ind2].state == State.INFECTIOUS):
                self.agents[ind1].state = State.INFECTIOUS
                self.total_infections += 1
            elif (self.agents[ind2].state == State.SUSCEPTIBLE) and \
                 (self.agents[ind1].state == State.INFECTIOUS):
                self.agents[ind2].state = State.INFECTIOUS

    def get_indices(self, state, max):
        indices = []
        for i in range(len(self.agents)):
            if i >= max:
                break
            if self.agents[i].state == state:
                indices.append(i)
        return indices

    def infect_method_two(self):
        indices = self.get_indices(State.SUSCEPTIBLE,
                                   self.parameters.encounters)
        random.shuffle(self.agents)
        for i in range(len(indices)):
            if self.agents[i].state == State.INFECTIOUS:
                self.agents[indices[i]].state = State.INFECTIOUS
                self.total_infections += 1

    def recover(self):
        """ Simulation event that moves agents from infectious to recovered
        state.
        """
        for agent in self.agents:
            if agent.state == State.INFECTIOUS:
                if random.random() < self.parameters.recovery_prob:
                    agent.state = State.RECOVERED

    def vaccinate(self):
        """ Simulation event that moves agents from susceptible to vaccinated
        state.
        """
        for agent in self.agents:
            if agent.state == State.SUSCEPTIBLE:
                if random.random() < self.parameters.vaccination_prob:
                    agent.state = State.VACCINATED

    def susceptible(self):
        """ Simulation event that moves vaccinated and susceptible agents back
        the susceptible state.
        """
        for agent in self.agents:
            if (agent.state == State.VACCINATED) or \
               (agent.state == State.RECOVERED):
                if random.random() < self.parameters.regression_prob:
                    agent.state = State.SUSCEPTIBLE

    def die(self):
        """ Simple death event that differentiates between infectious and
        susceptible agents.
        """
        for agent in self.agents:
            if agent.state == State.SUSCEPTIBLE:
                if random.random() < self.parameters.death_prob_susceptible:
                    agent.state = State.DEAD
            elif agent.state == State.INFECTIOUS:
                if random.random() < self.parameters.death_prob_infectious:
                    agent.state = State.DEAD
                    self.infection_deaths += 1

    def print_agents(self):
        """ Simulation event that outputs the agents to a file.
        """
        self.agents.sort(key=lambda agent: agent.identity)
        with open(self.parameters.agent_filename, 'w') as f:
            print("id,state", file=f)
            for agent in self.agents:
                print(f"{agent.identity},{agent.state}", file=f)

    def report_header(self):
        """ Creates the csv header for the report event.
        """
        print("#,iter,S,I,R,V,D,TI,TID")

    def report(self, iteration):
        """ Prints out the vital statistics.
        """
        susceptible = len([agent for agent in self.agents
                           if agent.state == State.SUSCEPTIBLE])
        infectious = len([agent for agent in self.agents
                          if agent.state == State.INFECTIOUS])
        recovered = len([agent for agent in self.agents
                         if agent.state == State.RECOVERED])
        vaccinated = len([agent for agent in self.agents
                          if agent.state == State.VACCINATED])
        dead = len([agent for agent in self.agents
                    if agent.state == State.DEAD])

        s = f"{self.identity}.{iteration}," \
            f"{susceptible},{infectious},{recovered},{vaccinated},{dead}," \
            f"{self.total_infections},{self.infection_deaths}"
        print(s)
        if self.parameters.output_agents > 0:
            if (iteration > 0) and \
               (iteration % self.parameters.output_agents == 0):
                self.print_agents()

    def simulate(self):
        """ Simulation engine that repeatedly executes all the encounters for
        specified number of iterations.
        """
        if self.identity == 0:
            self.report_header()
        for i in range(self.parameters.iterations):
            self.grow()
            if self.parameters.infection_method == InfectionMethod.BOTH:
                if self.identity % 2 == 0:
                    self.infect_method_one()
                else:
                    self.infect_method_two()
            elif self.parameters.infection_method == InfectionMethod.ONE:
                self.infect_method_one()
            else:
                self.infect_method_two()
            self.recover()
            self.vaccinate()
            self.susceptible()
            self.die()
            if i != 0 and i % 100 == 0:
                self.report(i)
        self.report(self.parameters.iterations)
