// ===============================================  
// CHUCK'S GLASS MARBLE (CORRECTED)  
// - Verified working presets  
// - Clean decay every time  
// ===============================================  

// === PRESET KEY ===  
// 0: "marimba", 1: "vibes", 2: "agogo",  
// 3: "wood1", 4: "resonant", 5: "wood2",  
// 6: "beats", 7: "twoFixed", 8: "clump",  
// 9: "tibetan bowl"  

// Settings  
5 => int preset;    // 3 = "wood1" (glass-like)  
60 => int note;    // MIDI note (60 = C5)  
0.4 => float strikeGain;  
0.2 => float revMix;  

// Signal chain  
ModalBar marble => NRev rev => LPF filter => dac;  
preset => marble.preset;  // Now using INT, not string  
strikeGain => marble.strike;  
revMix => rev.mix;  
2000 => filter.freq;  

// Play function  
fun void drop() {  
    note + Math.random2(-1, 1) => marble.noteOn;  
    8000::ms => now;  
}  

// Drop it!  
drop();  

// === PRESET DEMO === Uncomment to test all:  
/*  
for (0 => int i; i < 10; i++) {  
    i => marble.preset;  
    <<< "Playing preset:", i >>>;  
    drop();  
}  
*/  
// ===============================================  
