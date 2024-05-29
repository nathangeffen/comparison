package abm

import (
	"fmt"
	"math"
	"math/rand"
)

// Agent states are stored as ints.
type State int


// Here are the possible agent states.
const (
	Susceptible State = 0
	Infected State = 1
	Dead State = 2
)


// Holds an agent who has two attributes, a unique identity and a state.
type Agent struct {
	identity int
	state State
}

// Returns the agent state
func(a *Agent) State() State {
    return a.state
}

// Creates a new agent with a unique identity number and an initial state
func NewAgent(identity int, state State) Agent {
	a := Agent{identity: identity, state: state}
	return a
}

// Each simulation has a unique identity number and a slice of agents.
type Simulation struct {
	identity int
	agents []Agent
}


// Creates a new simulation with a specified number of agents, with a
// specified number of them initially infected.
func NewSimulation(identity int, num_agents int, num_infections int) Simulation {
	s := Simulation{identity: identity}
	s.agents = make([]Agent, num_agents)
	for i := 0; i < num_infections; i++ {
		s.agents[i].identity = i
		s.agents[i].state = Infected
	}
	for i := num_infections; i < len(s.agents); i++ {
		s.agents[i].identity = i
		s.agents[i].state = Susceptible
	}
	rand.Shuffle(len(s.agents), func(i, j int) {
		s.agents[i], s.agents[j] = s.agents[j], s.agents[i]
	})
	return s
}


// Getter function for a simulation's agents.
func(s *Simulation) Agents() []Agent {
    return s.agents
}

// Counts the number of agents in a given state.
func count_state(agents[] Agent, state State) int {
	c := 0
	for _, agent := range(agents) {
		if agent.state == state {
			c += 1
		}
	}
	return c
}


// Counts the number of agents not in a given state.
func count_not_state(agents[] Agent, state State) int {
	c := count_state(agents, state)
	return len(agents) - c
}


// Grows the number of agents in the simulation.
func (s *Simulation) Grow(growth_per_day float64) {
	num_agents := count_not_state(s.agents, Dead)
	new_agents := int(math.Round(growth_per_day * float64(num_agents)))
	n := len(s.agents)
	for i := n; i < n + new_agents; i++ {
		a := NewAgent(i, Susceptible)
		s.agents = append(s.agents, a)
	}
}

// Intentionally time consuming method to infect agents in the simulation.
func (s *Simulation) Infect(events int) {
	for i := 0; i < events; i++ {
		ind1 := rand.Intn(len(s.agents))
		ind2 := rand.Intn(len(s.agents))
		if s.agents[ind1].state == Susceptible &&
			s.agents[ind2].state == Infected {
			s.agents[ind1].state = Infected
		} else if s.agents[ind2].state == Susceptible &&
			s.agents[ind1].state == Infected {
			s.agents[ind2].state = Infected
		}
	}
}

// Kills agents in the simulation, with death rates for susceptible
// and infected agents differentiated.
func (s *Simulation) Die(death_rate_susceptible float64,
	death_rate_infected float64) {
	for i := 0; i < len(s.agents); i++ {
		if s.agents[i].state == Susceptible {
			if rand.Float64() < death_rate_susceptible {
				s.agents[i].state = Dead
			}
		} else if s.agents[i].state == Infected {
			if rand.Float64() < death_rate_infected {
				s.agents[i].state = Dead
			}
		}
	}
}

// Writes simulation statistics to standard output.
func (s *Simulation) Report(iteration int) {
	num_susceptible := count_state(s.agents, Susceptible)
	num_infections := count_state(s.agents, Infected)
	num_deaths := count_state(s.agents, Dead)
	fmt.Println(
		"Simulation:", s.identity,
		"Iteration:", iteration,
		"Susceptible", num_susceptible,
		"Infections:", num_infections,
		"Deaths:", num_deaths)
}

// Simulation engine that repeatedly executes the events the specified
// number of iterations.
func (s *Simulation) Simulate(iterations int,
	growth_per_day float64,
	events int,
	death_rate_susceptible float64,
	death_rate_infected float64) {
	for i := range(iterations) {
		s.Grow(growth_per_day)
		s.Infect(events)
		s.Die(death_rate_susceptible, death_rate_infected)
		if i % 100 == 0 {
			s.Report(i)
		}
	}
}
