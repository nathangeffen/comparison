package main

import (
	"flag"
	"sync"
	"nathangeffen/abm"
)

// The simulation's parameters are kept here.
type parameters struct {
	simulations int
	iterations int
	infections int
	agents int
	events int
	growth float64
	death_rate_susceptible float64
	death_rate_infected float64
}


// Process the command line arguments and return values set in
// parameters struct.
func processFlags() parameters {
	var p parameters
	flag.IntVar(&p.simulations, "simulations", 10,
		"number of simulations")
	flag.IntVar(&p.iterations, "iterations", 365 * 4,
		"number of iterations")
	flag.IntVar(&p.infections, "infections", 10,
		"initial infections")
	flag.IntVar(&p.agents, "agents", 10000,
		"number of agents")
	flag.IntVar(&p.events, "events", 20,
		"number of potential infections per iteration to simulate")
	flag.Float64Var(&p.growth,  "growth", 0.0001,
		"population growth per iteration")
	flag.Float64Var(&p.death_rate_susceptible, "death_rate_susceptible",
		0.0001, "death rate for susceptible agents per iteration")
	flag.Float64Var(&p.death_rate_infected, "death_rate_infected",
		0.001, "death rate for infected agents per iteration")
	flag.Parse()
	return p
}

// Gets the command line arguments and then executes in parallel the
// specified number of simulations.  Note that it uses a standard
// library WaitGroup to manage the parallel processing. See Go by
// Example to see the WaitGroup pattern. Note that unlike the Python
// and C++ versions, we provide absolutely nothing about the number of
// cores. WaitGroup presumably works that all out.
func main() {
	p := processFlags()
	var wg sync.WaitGroup
	for i := 0; i < p.simulations; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			func(sim_num int, p *parameters) {
				s := abm.NewSimulation(sim_num, p.agents, p.infections)
				s.Simulate(p.iterations, p.growth, p.events,
					p.death_rate_susceptible, p.death_rate_infected)
				s.Report(p.iterations)
			}(i, &p)
		}()
	}
	wg.Wait()
}
