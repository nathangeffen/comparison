// Copyright 2024 Nathan Geffen

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! This program implements a simple agent based model for the purpose of
//! comparing programming languages.

#include "abm.hpp"


void shuffle(std::vector<Agent> &agents, Rng& rng)
{
		for (size_t i = agents.size() - 1; i > 0; --i) {
				size_t j = rng.to(i + 1);
				Agent t = agents[j];
				agents[j] = agents[i];
				agents[i] = t;
		}
}

/// Displays the first letter of a state in upper case
std::ostream& operator<<(std::ostream& stream,
			 const State& state) {
  switch(state) { // Assuming you define print for matrix
  case State::SUSCEPTIBLE: stream << 'S'; break;
  case State::INFECTIOUS: stream << 'I'; break;
  case State::RECOVERED: stream << 'R'; break;
  case State::VACCINATED: stream << 'V'; break;
  case State::DEAD: stream << 'D'; break;
  }
  return stream;
}

