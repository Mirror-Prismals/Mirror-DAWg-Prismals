Gain master => dac;
0.4 => master.gain;

// chord progression root frequencies
[
    // I chord: C major root pos
    [261.63, 329.63, 392.00, 523.25], // C4 E4 G4 C5
    
    // IV6/3 chord (F major first inversion: A>C>F)
    [220.00, 261.63, 349.23, 440.0]   // A3 C4 F4 A4
]
@=> float chordFreqs[][];

6 => int harmN;
[1,2,3,4,5,6] @=> int ratios[];
[1.0, 0.7, 0.5, 0.25, 0.15, 0.1] @=> float amps[];

SinOsc vibrato => blackhole;
6 => vibrato.freq;

spork ~ playProgression();

while(true) 1::second => now;

fun void playProgression() {
    while(true)
    {
        // I chord
        spork ~ playChord(chordFreqs[0]);
        2::second => now;
        // IV6/3 chord (first inversion)
        spork ~ playChord(chordFreqs[1]);
        2::second => now;
    }
}

// function to build/play a chord
fun void playChord(float freqs[])
{
    ADSR env => Gain vol => dac;
    env.set(5::ms, 30::ms, 0.8, 500::ms);
    env.keyOn();
    0.35 => vol.gain;
    
    int notesN; freqs.cap() => notesN;
    
    Noise n => BPF nfilt => Gain nGain => env;
    3000 => nfilt.freq;
    5 => nfilt.Q;
    0.08 => nGain.gain;
    
    float bendRatio; 0.97 => bendRatio;
    SawOsc oscs[notesN][harmN];
    
    for(0 => int ni; ni < notesN; ni++)
    {
        Noise n2 => Gain g2 => env;
        0.02 => g2.gain; 1 => n2.gain;
        for(0 => int hi; hi < harmN; hi++)
        {
            new SawOsc @=> oscs[ni][hi];
            oscs[ni][hi] => env;
            amps[hi] => oscs[ni][hi].gain;
            (freqs[ni]*bendRatio*ratios[hi]) => oscs[ni][hi].freq;
        }
    }
    spork ~ chordGlideUp(freqs, oscs, bendRatio);
    spork ~ vibratoSweep(freqs, oscs);
    1.6::second => now;
    env.keyOff();
    0.4::second => now;
}

fun void chordGlideUp(float freqs[], SawOsc oscs[][], float bendRatio)
{
    int notesN; freqs.cap() => notesN;
    100 => int steps;
    for(0 => int count; count<=steps; count++)
    {
        for(0 => int ni; ni<notesN; ni++)
        {
            for(0 => int hi; hi<harmN; hi++)
            {
                (freqs[ni]*bendRatio*ratios[hi]) + ((freqs[ni]*ratios[hi])-(freqs[ni]*bendRatio*ratios[hi]))*(count/100.0) 
                => oscs[ni][hi].freq;
            }
        }
        1::ms => now;
    }
}

fun void vibratoSweep(float freqs[], SawOsc oscs[][])
{
    int notesN; freqs.cap() => notesN;
    while(true)
    {
        for(0 => int ni; ni<notesN; ni++)
        {
            for(0 => int hi; hi<harmN; hi++)
            {
                (freqs[ni]*ratios[hi]) + vibrato.last()*10 => oscs[ni][hi].freq;
            }
        }
        10::ms => now;
    }
}
