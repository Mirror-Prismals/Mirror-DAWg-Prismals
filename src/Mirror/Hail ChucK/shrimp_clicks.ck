// shrimp_clicks.ck
// Simulates snapping shrimp clicks in a coral reef

10 => int numShrimp;

// Function to make a single shrimp snap (no arguments)
fun void shrimpSnap() {
    // Create a short burst of white noise for the snap
    Noise n => Gain g => dac;
    0.2 => g.gain;

    // Envelope for click shape
    Step s => g;
    1.0 => s.next;
    0.001 :: second => now;
    0.0 => s.next;
    0.003 :: second => now;

    // Disconnect after snap
    n =< g =< dac;
    s =< g;
}

// Each shrimp snaps at random intervals
fun void shrimpLoop() {
    while (true) {
        shrimpSnap();
        Math.random2f(0.2, 1.5) :: second => now;
    }
}

// Spork multiple shrimp
for (0 => int i; i < numShrimp; i++) {
    spork ~ shrimpLoop();
}

// Let it run for 60 seconds
60 :: second => now;
