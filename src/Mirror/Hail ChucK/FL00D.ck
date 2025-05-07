// Simple drop generator
Noise n => BPF b => dac;
2000 => b.freq;
0.5 => b.Q;

while (true) {
    1 => n.gain;
    0.005 => float dur;
    dur::second => now;
    0 => n.gain;
    Std.rand2f(0.05, 0.3)::second => now;
}
