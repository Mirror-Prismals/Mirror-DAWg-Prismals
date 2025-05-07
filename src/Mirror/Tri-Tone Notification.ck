// OS Tri-tone Notification - by ChucK & DeepSeek
// Connect to audio output
SinOsc s => ADSR env => dac;

// Set envelope parameters
env.set(5::ms, 100::ms, 0.0, 5::ms);

// First note (D5)
622.25 => s.freq;
1 => env.keyOn;
100::ms => now;
1 => env.keyOff;
5::ms => now;

// Second note (F5)
698.46 => s.freq;
1 => env.keyOn;
100::ms => now;
1 => env.keyOff;
5::ms => now;

// Third note (A5)
880.00 => s.freq;
1 => env.keyOn;
100::ms => now;
1 => env.keyOff;
