// Max pattern length
32 => int MAXLEN;

// Globals for builder
int pattern[MAXLEN];
int counts[MAXLEN];
int remainders[MAXLEN];
int idx;

// Recursive builder (must be top-level in ChucK)
fun void build(int lvl) {
    if (lvl == -1) {
        0 => pattern[idx];
        idx++;
    } else if (lvl == -2) {
        1 => pattern[idx];
        idx++;
    } else {
        for (0 => int j; j < counts[lvl]; j++) {
            build(lvl - 1);
        }
        if (remainders[lvl] != 0) {
            build(lvl - 2);
        }
    }
}

// Bjorklund/Euclidean generator for ChucK
fun void bjorklund(int steps, int pulses) {
    // Reset globals
    for (0 => int i; i < MAXLEN; i++) {
        0 => pattern[i];
        0 => counts[i];
        0 => remainders[i];
    }
    0 => idx;

    int divisor;
    int level;
    steps - pulses => divisor;
    pulses => remainders[0];
    0 => level;

    // Build counts and remainders arrays
    while (true) {
        divisor / remainders[level] => counts[level];
        divisor % remainders[level] => remainders[level + 1];
        remainders[level] => divisor;
        level++;
        if (remainders[level] <= 1) { break; }
    }
    divisor => counts[level];

    // Build the pattern
    build(level);
}

// --- PARAMETERS ---
31 => int steps;
16 => int pulses;

// --- GENERATE PATTERN ---
bjorklund(steps, pulses);

// --- PRINT PATTERN ---
<<< "Euclidean Pattern:" >>>;
for (0 => int i; i < steps; i++) {
    if (pattern[i] == 1) {
        <<< "x" >>>;
    } else {
        <<< "-" >>>;
    }
}

// --- PLAY PATTERN ---
SinOsc osc => dac;
440 => osc.freq;
0.05::second => dur pulseLength;
0.25::second => dur stepLength;

for (0 => int i; i < steps; i++) {
    if (pattern[i] == 1) {
        0.2 => osc.gain;
        pulseLength => now;
        0 => osc.gain;
        (stepLength - pulseLength) => now;
    } else {
        0 => osc.gain;
        stepLength => now;
    }
}
