// "Computer Keys" inspired digital piano in ChucK

// Function to play a digital piano note
fun void playDigitalPiano(float freq, float dur) {
    // Main oscillator (sine for body)
    SinOsc main => Gain g => dac;
    // Add metallic overtone
    SawOsc overtone => g;
    overtone.gain(0.2);
    overtone.freq(freq * 2.01); // slightly detuned for digital flavor

    main.freq(freq);
    main.gain(0.1);

    // Envelope
    ADSR env => g;
    env.set(0.001, dur * 0.3, 0.0, dur * 0.7); // fast attack, short decay, no sustain, quick release
    env.keyOn();

    // Bitcrusher effect (simulate with sample-and-hold)
    Step bitcrush => g;
    bitcrush.next(0.0); // not used, just for effect chain

    // Play note
    dur::second => now;
    env.keyOff();
    0.9::second => now;

    // Disconnect
    main =< dac;
    overtone =< dac;
    env =< g;
    bitcrush =< g;
}

// Play a C major chord
[261.63, 329.63, 392.00] @=> float notes[];
for (0 => int i; i < notes.size(); i++) {
    spork ~ playDigitalPiano(notes[i], 0.5);
    0.05::second => now;
}

// Wait for all notes to finish
10::second => now;
