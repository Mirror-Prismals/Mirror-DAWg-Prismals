// OS "Note" Notification
SinOsc s => ADSR env => dac;
env.set(10::ms, 300::ms, 0.5, 200::ms);

[784.0, 659.3, 523.3] @=> float notes[];

for(0 => int i; i < notes.size(); i++) {
    notes[i] => s.freq;
    1 => env.keyOn;
    if (i == notes.size()-1) 500::ms => now;
    else 100::ms => now;
    1 => env.keyOff;
    20::ms => now;
}
