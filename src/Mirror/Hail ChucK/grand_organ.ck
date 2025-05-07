// Bright Mario64 slider-style organ chord

// Output volume
Gain master => dac;
0.4 => master.gain;

// Envelope with punchy attack & decay
ADSR env => master;
env.set(10::ms, 50::ms, 0.7, 1::second);
env.keyOn();

// Vibra-Leslie swirl
SinOsc vibrato => blackhole;
5.5 => vibrato.freq;

// How many chord notes?
4 => int notesN;  // C E G B for CMaj7 (feel free to try add9 etc.)

// Frequencies for C major7 chord (transpose as needed)
[261.63, 329.63, 392.00, 493.88] @=> float chordFreqs[];

// Organ harmonics per note (drawbars)
7 => int harmN;
[1,2,3,4,5,6,8] @=> int ratios[];
[1.0, 0.4, 0.2, 0.15, 0.1, 0.06, 0.05] @=> float amps[];

// Arrays of oscillators
SinOsc oscs[notesN][harmN];

// Clicks per note for percussive attack
Impulse clicks[notesN];
Gain clickgains[notesN];

// Build notes
for(0 => int ni; ni < notesN; ni++)
{
    // build harmonics per note
    for(0 => int hi; hi < harmN; hi++)
    {
        new SinOsc @=> oscs[ni][hi];
        oscs[ni][hi] => env;
        (chordFreqs[ni] * ratios[hi]) => oscs[ni][hi].freq;
        amps[hi] => oscs[ni][hi].gain;
    }
    // add a "click" to simulate attack percussion
    new Impulse @=> clicks[ni];
    new Gain @=> clickgains[ni];
    clicks[ni] => clickgains[ni] => env;
    0.12 => clickgains[ni].gain;
    1 => clicks[ni].next;    // trigger click
}

// Vibrato modulation update
spork ~ vibratoSweep();

// Play chord for 4s
4::second => now;
env.keyOff();
2::second => now;

// --- vibrato update ---
fun void vibratoSweep()
{
    while(true)
    {
        for(0 => int ni; ni < notesN; ni++)
        {
            for(0 => int hi; hi < harmN; hi++)
            {
                (chordFreqs[ni] * ratios[hi]) + vibrato.last()*6 => oscs[ni][hi].freq;
            }
        }
        10::ms => now;
    }
}
