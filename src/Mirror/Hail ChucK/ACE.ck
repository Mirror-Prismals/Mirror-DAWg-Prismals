// -- Setup synthesis --
SqrOsc lead => dac;
SqrOsc bass => dac;
SinOsc vib => blackhole;
0.3 => lead.gain;
0.2 => bass.gain;
5 => vib.freq;

// -- Roots --
440 => float A;
A * Math.pow(2, -4/12.0) => float Fsharp;
A * Math.pow(2, -3/12.0) => float G;
A * Math.pow(2, -1/12.0) => float Ab;
A * Math.pow(2, -7/12.0) * 2 => float D;  // octave up
A * Math.pow(2, -5/12.0) => float B;

// -- Fixed 2D array [28 chords][max 4 notes] --
29 => int ChordCount;
4 => int MaxNotes;
float chords[ChordCount][MaxNotes];
int beats[ChordCount];

// -- Helper to fill chord row --
fun void setChord(int idx, float arr[], int beat) {
    0 => int j;
    while (j < arr.cap() && j < MaxNotes) {
        arr[j] => chords[idx][j];
        j++;
    }
    // fill leftover with 0
    while (j < MaxNotes) {
        0.0 => chords[idx][j];
        j++;
    }
    beat => beats[idx];
}

// -- define your chords & beats --
0 => int i;

// Major chord
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5, D*1.75 ], 8);
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5, D*1.75 ], 8);
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ Ab, Ab*1.25, Ab*1.5 ], 4);
setChord(i++, [ G, G*1.25, G*1.5 ], 4);
setChord(i++, [ Fsharp, Fsharp*1.1892, Fsharp*1.5 ], 8);
setChord(i++, [ G, G*1.25, G*1.5 ], 8);
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5, D*1.75 ], 8);
setChord(i++, [ Fsharp, Fsharp*1.1892, Fsharp*1.5 ], 8);
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5, D*1.75 ], 8);
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5, D*1.75 ], 8);
setChord(i++, [ A, A*1.25, A*1.5, A*1.875 ], 8); // Maj7
setChord(i++, [ D, D*1.25, D*1.5 ], 8);
setChord(i++, [ B, B*1.1892, B*1.5 ], 8);
setChord(i++, [ Fsharp, Fsharp*1.1892, Fsharp*1.5 ], 8);
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5, D*1.75 ], 8); // dom7
setChord(i++, [ B, B*1.1892, B*1.5 ], 8);
setChord(i++, [ Fsharp, Fsharp*1.1892, Fsharp*1.5 ], 8);
setChord(i++, [ A, A*1.25, A*1.5 ], 8);
setChord(i++, [ D, D*1.25, D*1.5 ], 8);
setChord(i++, [ B, B*1.1892, B*1.5 ], 8);
setChord(i++, [ Fsharp, Fsharp*1.1892, Fsharp*1.5 ], 8);

// -- Main loop --
180::ms => dur unit;

while (true) {
    for (0 => int ci; ci < ChordCount; ci++) {
        chords[ci][0] / 2 => bass.freq; // root as bass one octave down
        for (0 => int r; r < beats[ci]; r++) {
            for (0 => int ni; ni < MaxNotes; ni++) {
                if (chords[ci][ni] > 0) {
                    chords[ci][ni] + vib.last()*6 => lead.freq;
                    unit => now;
                }
            }
        }
    }
}
