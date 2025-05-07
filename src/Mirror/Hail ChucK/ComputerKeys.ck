// "Computer Keys" sine-piano patch, Subwoofer Lullaby voice leading

fun void playChord(float notes[], float dur, float tail) {
    JCRev reverb => dac;
    reverb.mix(0.18);

    Gain chordMix => reverb;
    chordMix.gain(0.28);

    ADSR env => chordMix;
    env.set(0.001, 0.12, 0.0, tail);
    env.keyOn();

    for (0 => int i; i < notes.size(); i++) {
        spork ~ playNote(notes[i], dur, env);
    }

    dur::second => now;
    env.keyOff();
    tail::second => now;
    chordMix =< reverb;
    reverb =< dac;
}

fun void playNote(float freq, float dur, ADSR @ env) {
    SinOsc main => Gain g => env;
    main.freq(freq);
    main.gain(0.18);

    SinOsc bell => g;
    bell.freq(freq * 2.0);
    bell.gain(0.05);

    for (0 => int i; i < 20; i++) {
        (1.0 - (i/20.0)) * 8.0 + freq => main.freq;
        (1.0 - (i/20.0)) * 16.0 + freq * 2.0 => bell.freq;
        1::ms => now;
    }
    freq => main.freq;
    freq * 2.0 => bell.freq;

    dur::second => now;
    main =< env;
    bell =< env;
}

// Define note frequencies
// C5 = 523.25 Hz, E5 = 659.25 Hz, G5 = 783.99 Hz, B5 = 987.77 Hz
[523.25] @=> float c5[];
[659.25, 987.77] @=> float e5b5[];
[783.99] @=> float g5[];

// Chord sequence and timings
[ c5, e5b5, g5, e5b5 ] @=> float chords[][];
[ 0.4, 0.4, 0.4, 0.4 ] @=> float durs[];
[ 0.0, 1.2, 2.4, 3.6 ] @=> float times[];
20.0 => float tail;

// Spork each chord at the correct time, so their tails overlap
for (0 => int i; i < chords.size(); i++) {
    spork ~ delayedChord(chords[i], durs[i], tail, times[i]);
}

fun void delayedChord(float notes[], float dur, float tail, float delay) {
    delay::second => now;
    playChord(notes, dur, tail);
}

// Wait for all chords/reverbs to finish ringing
(tail + times[chords.size()-1] + 1)::second => now;
