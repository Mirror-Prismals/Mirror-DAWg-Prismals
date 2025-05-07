// fish_croaks.ck
// Simulates fish "croak" calls: short, rough, percussive bursts

4 => int numFish;

// Function to make a single croak (no arguments)
fun void fishCroak() {
    Noise n => BPF bpf => Gain g => dac;
    0.15 => g.gain;
    Math.random2f(80, 200) => bpf.freq; // Croak frequency (low)
    10 => bpf.Q;

    // Use Step for envelope
    Step s => g;
    1.0 => s.next; // Attack
    0.005 :: second => now;
    0.0 => s.next; // Release
    0.035 :: second => now;

    // Disconnect after croak
    n =< bpf =< g =< dac;
    s =< g;
}

// Each fish croaks at random intervals
fun void fishLoop() {
    while (true) {
        fishCroak();
        Math.random2f(1.0, 3.0) :: second => now;
    }
}

// Spork multiple fish
for (0 => int i; i < numFish; i++) {
    spork ~ fishLoop();
}

// Let it run for 60 seconds
60 :: second => now;
