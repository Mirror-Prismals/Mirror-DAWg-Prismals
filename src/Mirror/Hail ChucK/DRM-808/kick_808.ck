SinOsc s => ADSR env => Gain g => dac;

// Kick parameters
60 => float baseFreq;       // Final pitch (~Bass)
100 => float sweepAmt;      // How much sweep drops
50::ms => dur sweepTime;    // How fast pitch drops (short!)
0.5::second => dur decayTime; // amplitude decay length (~500ms)

// Set initial sine oscillator amp
1 => s.gain;
0.9 => g.gain;    // output gain

// Envelope: fast attack, no sustain, smooth decay
env.set(2::ms, 20::ms, 0, decayTime);

// Trigger envelope
env.keyOn();

// Pitch drops exponentially during sweep
now => time t0;
while(now - t0 < sweepTime) {
    (baseFreq + sweepAmt * Math.exp(-5 * (now - t0)/sweepTime)) => s.freq;
    1::ms => now;
}
baseFreq => s.freq;

// Let sine ring until amp fades
decayTime => now;

// Envelope release
env.keyOff();

// Fade out tail
300::ms => now;
