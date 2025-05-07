// ========== Effects / gain ==========

JCRev reverb => Gain master => dac;
0.8 => reverb.mix;
0.07 => master.gain;

// ========== Oscillators ==========
// 12 oscillators for a thick chord!
12 => int N;
SqrOsc oscs[N];
for (0 => int i; i < N; i++) {
    oscs[i] => reverb;
    0.02 => oscs[i].gain;
}

// ========== Helper function ==========
fun void playVoicing( float notes[] ) {
    for (0 => int i; i < N; i++) {
        if( i < notes.size() ) {
            notes[i] => oscs[i].freq;
        } else {
            // shut off unused osc
            0.0 => oscs[i].gain;
        }
    }
}

// ========== Define chords with explicit voicing ==========

[ 
// A major 9: A3 root, plenty of stacked octaves and extensions
220.0,    // A3 root
110.0,    // A2 bass octave
55.0,     // A1 sub
277.2,    // C#4 (maj3) 
329.6,    // E4 (5th)
415.3,    // G#4 (maj7)
493.9,    // B4 (9th)
440.0,    // A4
220.0,    // A3 (double root)
277.2,    // C#4 (double maj3)
659.2,    // E5 (upper 5th layer shimmer)
880.0     // A5 (upper root octave)
] @=> float Amaj9[];


[ 
// E major 9: similar voicing
164.8,    // E3
82.4,     // E2
41.2,     // E1
207.7,    // G#3 (maj3)
246.9,    // B3 (5)
311.1,    // D#4 (maj7)
370.0,    // F#4 (9)
329.6,    // E4 octave
659.2,    // E5 shimmer
415.3,    // G#4
246.9,    // B3 double
207.7     // G#3 double
] @=> float Emaj9[];


[ 
// F#m11: jazzy, stacked
185.0,    // F#3
92.5,     // F#2
46.25,    // F#1
220.0,    // A3 (min3)
277.2,    // C#4 (5)
329.6,    // E4 (b7)
369.9,    // F#4 (octave)
493.9,    // B4 (11th)
220.0,    // A3 double
554.4,    // C#5 shimmer
739.9,    // B5 upper
369.9     // F#4 double
] @=> float Fsm11[];


[ 
// Dmaj9
146.8,    // D3
73.4,     // D2
36.7,     // D1
183.3,    // F#3 (M3)
220.0,    // A3 (5)
277.2,    // C#4 (7)
329.6,    // E4 (9)
293.7,    // D4 octave
587.3,    // D5 shimmer
183.3,    // F#3 double
440.0,    // A4
293.7     // D4 double
] @=> float Dmaj9[];

// ========== Main loop ==========

6::second => dur bar;

while (true) {
    playVoicing(Amaj9);  bar => now;
    playVoicing(Emaj9);  bar => now;
    playVoicing(Fsm11);  bar => now;
    playVoicing(Dmaj9);  bar => now;
}
