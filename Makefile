all: rust zig

rust: FORCE
	cd rust; cargo build --release
	cd rust; hyperfine -w 2 "cargo run --release -- --simulations 1 --agents 1000000 --output_agents 1460 --encounters 10000 --infections 400"
	cd rust; hyperfine -w 2 "cargo run --release -- --simulations 100"
	cd rust; hyperfine -w 2 "cargo run --release -- --simulations 1 --agents 1000000 --output_agents 1460 --encounters 2000 --infections 400 --infection_method 2"

zig: FORCE
	cd zig;	zig build --release=fast
	cd zig; hyperfine -w 2 "zig build run --release=fast -- --simulations 1 --agents 1000000 --output_agents 1460 --encounters 10000 --infections 400"
	cd zig; hyperfine -w 2 "zig build run --release=fast -- --simulations 100"
	cd zig; hyperfine -w 2 "zig build run --release=fast -- --simulations 1 --agents 1000000 --output_agents 1460 --encounters 2000 --infections 400 --infection_method 2"

FORCE:
