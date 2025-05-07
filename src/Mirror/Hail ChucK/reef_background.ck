// reef_background.ck
// Simulates the underwater hiss, subtle crackles, and ambience of a healthy reef

// --- Underwater Hiss ---
Noise hiss => LPF lpf => Gain gHiss => dac;
8000 => lpf.freq; // Lowpass for underwater feel
0.05 => gHiss.gain;

// --- Crackles (random, subtle pops) ---
fun void crackleLoop() {
    while (true) {
        // Random interval between crackles
        Math.random2f(0.1, 0.7) :: second => now;

        // Crackle: short, sharp filtered noise burst
        Noise n => BPF bpf => Gain g => dac;
        4000 + Math.random2f(-1500, 1500) => bpf.freq;
        20 => bpf.Q;
        0.05 => g.gain;

        Step s => g;
        1.0 => s.next;
        0.002 :: second => now;
        0.0 => s.next;
        0.01 :: second => now;

        // Disconnect
        n =< bpf =< g =< dac;
        s =< g;
    }
}

// --- Subtle Ambience (very low sine for underwater "pressure") ---
SinOsc sub => Gain gSub => dac;
Math.random2f(18, 30) => sub.freq;
0.01 => gSub.gain;

// Start crackle loop (can spork multiple for density)
spork ~ crackleLoop();

// Run for 60 seconds
60 :: second => now;
