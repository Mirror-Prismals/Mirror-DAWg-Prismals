// fish_whoops.ck
// Simulates fish "whoop" calls: short, gliding tones with random pitch and timing

// Number of fish
5 => int numFish;

// Function to make a single whoop
fun void fishWhoop() {
    SinOsc s => Gain g => dac;
    0.1 => g.gain;

    // Random start and end frequencies (200–800 Hz)
    Math.random2f(200, 400) => float startFreq;
    Math.random2f(500, 800) => float endFreq;

    // Random duration (0.2–0.6 sec)
    Math.random2f(0.2, 0.6) => float whoopDur;

    // Glide from start to end frequency
    0 => float t;
    0.01 => float dt;
    while (t < whoopDur) {
        startFreq + (endFreq - startFreq) * (t / whoopDur) => s.freq;
        dt :: second => now;
        t + dt => t;
    }

    // Fade out quickly
    0.05 => g.gain;
    0.05 :: second => now;

    // Disconnect
    s =< g =< dac;
}

// Each fish calls at random intervals
fun void fishLoop() {
    while (true) {
        fishWhoop();
        Math.random2f(1.5, 4.0) :: second => now;
    }
}

// Spork multiple fish
for (0 => int i; i < numFish; i++) {
    spork ~ fishLoop();
}

// Let it run for 60 seconds
60 :: second => now;
