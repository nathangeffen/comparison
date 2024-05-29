all: rust zig

rust: FORCE
	cd rust; cargo run --release

zig: FORCE
	cd zig;	zig build run --release=fast

FORCE:
