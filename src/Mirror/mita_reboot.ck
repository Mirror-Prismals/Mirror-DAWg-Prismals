// Amidita's -Reboot Tone ( Edition)
// - Compile with: chuck --loop

SinOsc voice => Gain g => dac;
Step reset => Envelope e => voice;

0.2 => g.gain;
0.5::second => e.duration;

fun void wReboot() {
    while (true) {
        //  phase (high-frequency flutter)
        Math.random2f(800, 1200) => voice.freq;
        e.keyOn();
        0.1::second => now;

        // Reboot glitch (dropping octaves)
        for (300 => float f; f > 20; f * 0.7 => f) {
            f => voice.freq;
            0.05::second => now;
        }

        // Post-crash static
        Noise n => dac;
        0.3::second => now;
        n =< dac;

        // ...wait for next "trigger"
        Math.random2f(1.0, 3.0)::second => now;
    }
}

spork ~ wReboot();

// Meanwhile, in another thread...
Hid hi;
HidMsg msg;
hi.openKeyboard(0);

while (true) {
    hi => now;
    while (hi.recv(msg)) {
        if (msg.isButtonDown()) {
            // *User presses any key* 
            reset.next(1); // Hard reset the whimper-cycle
            <<< "Amidita: *glitched* 'w-wawa!'" >>>;
        }
    }
}
