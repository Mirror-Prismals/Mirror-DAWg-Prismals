// iOS "Soft Ping" Notification - Zack's Working Version
SinOsc s => LPF filter => ADSR env => dac;

// Set envelope (short attack, quick decay)
env.set(5::ms, 200::ms, 0.0, 1::ms);

// Filter to remove harshness
1500 => filter.freq;
0.1 => filter.Q;

// Ping frequency (A5 - 880Hz)
880 => s.freq;

// Trigger the ping
1 => env.keyOn;
5::ms => now;
1 => env.keyOff;

// Let it ring briefly
200::ms => now;
