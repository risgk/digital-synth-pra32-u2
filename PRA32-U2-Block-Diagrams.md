# Digital Synth PRA32-U2 Block Diagrams v3.16.0

- 2026-10-16 ISGK Instruments
- <https://github.com/risgk/digital-synth-pra32-u2>

The diagrams of the Filter, the FX (Chorus FX, Delay FX, and Reverb), and the Output Compressor, and the graphs of the Wave Tables.
For the whole synthesizer, see [Synthesizer Block Diagram](./README.md#synthesizer-block-diagram) in README.md.
Solid arrows carry audio, dashed arrows carry control.


## Filter

A zero-delay feedback state variable filter. The band pass integrator leaks, more and more as
the band pass bends into a soft clip curve (ceiling 4.0), and that is what holds the resonance
down; at small levels the leak is next to nothing, so the pass band passes nearly clean. The
loop equation, leak included, is solved every sample by the one-step simplified Newton method
(the chord method, with the slope held at the linear part) from the linear solution,
which needs no division and lands close to the root. The leak's rate is fixed in time, so the sound
does not change with the sampling rate. The low pass state stays linear, with a clamp at 16 as
a guard that ordinary use never reaches. The curve is biased by 1/8, so that the leak adds even
harmonics as the band pass is driven (at a high Resonance or near the cutoff), with its
small-signal gain made up for, so that the resonance is kept; a DC blocker on the output
removes the DC that comes with it. The output clip comes last. Above the Filter
Resonance 122, k turns negative and the loop oscillates.

```mermaid
flowchart LR
    IN([Input]) --> SUM((Σ))
    SUM -->|HP| I1["Integrator 1<br/>BP, state s1"]
    I1 -->|BP| I2["Integrator 2<br/>LP, state s2"]
    SUM -->|HP| MS["Filter Mode<br/>LP / BP / BR / HP"]
    I1 -->|BP| MS
    I2 -->|LP| MS
    MS --> DC["DC blocker<br/>7.5 Hz"]
    DC --> OC["Output clip<br/>linear up to 0.75"]
    OC --> OUT([Output])
    SC["Nonlinear leak<br/>biased 1/8, ceiling 4.0, one-step simplified Newton"] -.- I1
    L2["s2 is linear<br/>guard clamp at 16"] -.- I2
    I1 -->|"−k·BP"| SUM
    I2 -->|"−LP"| SUM
    classDef nl fill:#FAECE7,stroke:#D85A30,color:#712B13
    class SC,OC nl
```

The same structure redrawn as an op-amp integrator filter. The asymmetric diode pair stands for
the biased nonlinear leak, the coupling capacitor for the DC blocker, and the output clipper for the
output clip. It is an interpretation, not a reproduction of an actual circuit.

```mermaid
flowchart LR
    IN([Input]) --> A1["Summing amp Σ"]
    A1 -->|HP| A2["Integrator ∫<br/>C1"]
    D["Diode pair<br/>asymmetric"] -.-|across C1| A2
    A2 -->|BP| A3["Integrator ∫<br/>C2, linear"]
    A1 -->|HP| MS["Filter Mode<br/>LP / BP / BR / HP"]
    A2 -->|BP| MS
    A3 -->|LP| MS
    MS --> CC["Coupling capacitor"]
    CC --> LIM["Output clipper"]
    LIM --> OUT([Output])
    A2 -->|"R/k (resonance)"| A1
    A3 -->|R| A1
    classDef nl fill:#FAECE7,stroke:#D85A30,color:#712B13
    class D,LIM nl
```


## Chorus FX

A stereo chorus, used as a send effect: the input is sent at the Chorus Level, and the
chorused signal is added to the input. Each side goes through a gentle low pass filter into a
short delay line of its own. A triangle LFO swings the two delay times in opposite directions
about 5.33 ms (by the Chorus Depth, at the Chorus Rate), so the left and right delayed signals
bend in pitch against each other, and a mono source comes out spread across the stereo field.

```mermaid
flowchart LR
    INL([Input L]) --> SL["× Chorus Level"]
    SL --> FL["Low pass<br/>1-pole"]
    FL --> DL["Delay line L<br/>5.33 ms − LFO"]
    DL --> ML((Σ))
    INL --> ML
    ML --> OUTL([Output L])
    INR([Input R]) --> SR["× Chorus Level"]
    SR --> FR["Low pass<br/>1-pole"]
    FR --> DR["Delay line R<br/>5.33 ms + LFO"]
    DR --> MR((Σ))
    INR --> MR
    MR --> OUTR([Output R])
    LFO["Triangle LFO<br/>Chorus Rate, Chorus Depth"] -.-> DL & DR
```

The delay lines are read between two samples (linear interpolation), and the delay times are
interpolated over the samples between the control updates, so that the pitch bends smoothly.


## Delay FX

A stereo delay with feedback (Delay Mode: Stereo Delay or Ping Pong Delay), used as a send
effect: the input is sent at the Delay Level, and the delayed signal is added to the input.
The send joins the delayed signal, and the sum goes through the Delay Feedback gain and a gentle
low pass filter into the delay line, so the first echo comes at the Delay Level times the Delay
Feedback, and each echo after it at the Delay Feedback times the one before.

### Stereo Delay

Each side has a delay line and a loop of its own. The diagram is one side; the other is the same.

```mermaid
flowchart LR
    IN([Input L / R]) --> S["× Delay Level"]
    S --> SUM((Σ))
    SUM --> FB["× Delay Feedback<br/>0 to 0.5"]
    FB --> LPF["Low pass<br/>1-pole"]
    LPF --> DL["Delay line<br/>Delay Time, up to 340 ms"]
    DL --> SUM
    DL --> M((Σ))
    IN --> M
    M --> OUT([Output L / R])
```

### Ping Pong Delay

The two sides are sent together, as mono, into the left delay line, which feeds the right one,
which feeds the left one again, so the echoes go back and forth between the left and the right.

```mermaid
flowchart LR
    INL([Input L]) --> SL["× Delay Level"]
    INR([Input R]) --> SR["× Delay Level"]
    SL --> H["(L + R) × 0.5"]
    SR --> H
    H --> SUML((Σ))
    SUML --> FBL["× Delay Feedback"]
    FBL --> LPFL["Low pass<br/>1-pole"]
    LPFL --> DLL["Delay line L<br/>Delay Time"]
    DLL --> FBR["× Delay Feedback"]
    FBR --> LPFR["Low pass<br/>1-pole"]
    LPFR --> DLR["Delay line R<br/>Delay Time"]
    DLR --> SUML
    DLL --> ML((Σ))
    INL --> ML
    ML --> OUTL([Output L])
    DLR --> MR((Σ))
    INR --> MR
    MR --> OUTR([Output R])
```

The delay lines are read between two samples (linear interpolation). When the Delay Time is
changed, the read position follows it at a limited speed, which bends the pitch of the echoes
(within +/-25%), as on a tape delay.


## Reverb

A feedback delay network (FDN) reverb (Delay Mode: Reverb), which reuses the delay lines of the
Delay FX (no extra RAM). It is a send effect as the Delay FX is, and the Delay Level, the Delay
Time, and the Delay Feedback work as they do for the Delay FX: the same values give about the
same length and loudness of the tail.

```mermaid
flowchart LR
    IN([Input L / R]) --> S["× Delay Level<br/>× Delay Feedback"]
    S --> AP["Input allpass × 2<br/>each side"]
    AP --> SUM((Σ))
    SUM -->|"L to line 0, R to line 1"| LN["4 delay lines<br/>Delay Time / 3, and<br/>219, 187, 157 / 256 of it"]
    LN --> H["4 × 4 Hadamard"]
    H --> G["× Decay<br/>each line"]
    G --> LPF["Low pass<br/>1-pole, each line"]
    LPF --> LA["Loop allpass<br/>each line"]
    LA --> SUM
    LN --> OM["Output mix<br/>L: (line 0 + line 3) × 17/16<br/>R: (line 1 − line 2) × 17/16"]
    OM --> M((Σ))
    IN --> M
    M --> OUT([Output L / R])
```

- It runs at half the sampling rate (24 kHz), every other sample, to halve its processing time:
  the input is the average of two samples, and the output is interpolated between the last two
  results; the lengths above are in 24 kHz samples
- Input diffusion: 2 Schroeder allpasses per side (gain 0.75; 113 and 83 samples for the left, 109
  and 89 for the right), which smear the input so that the tail starts dense; the right side is
  slightly detuned from the left, for decorrelation
- The 4 lines are mixed by an orthonormal Hadamard matrix (additions and subtractions only)
- Size: the line 0 is 1/3 of the Delay Time (the first reflection is a triplet of the delay),
  and the lines 1-3 are 219/256, 187/256, and 157/256 of it; when the Delay Time is changed,
  the lengths follow it slowly (within +/-3% of the pitch), read between two samples
- Decay: each line's gain is set so that the tail falls by the Delay Feedback / 256 over each
  1.5 x Delay Time, so that it sounds about as long as the Delay's echoes
- Damping: a light 1-pole low pass filter in each feedback path, so that the high frequencies
  decay about as fast as the Delay's echoes do
- Loop diffusion: a short allpass (gain 0.5; 73, 89, 107, and 127 samples) in each feedback path, lossless, which multiplies
  the echo density on every pass; the input is added after it, so that the first reflection stays
  in place
- Switching to or from the Reverb, which uses the delay lines differently, fades the wet signal
  out, and clears the delay lines before fading it in again


## Output Compressor

A feed-forward peak compressor for the final output, which also works as the limiter, followed by
the soft clip; always on. The input of FX Routing Dry is not compressed (or limited), but is delayed,
lowered by the output gain, and clipped with the output.

```mermaid
flowchart LR
    IN([Input L / R<br/>from the Delay FX<br/>and FX Routing Comp]) --> DLY["Delay<br/>1 ms (look-ahead)"]
    DLY --> VCA["× Gain"]
    VCA --> SUM((Σ))
    DRY([Input L / R<br/>FX Routing Dry]) --> DD["Delay<br/>1 ms"]
    DD --> OG["× 0.75<br/>-2.5 dB"]
    OG --> SUM
    SUM --> SC["Soft clip<br/>linear up to 0.75, ceiling 1.0"]
    SC --> OUT([Audio Out L / R])
    IN --> PK["Peak<br/>max of |L| and |R|,<br/>held over 1 ms"]
    PK -.-> ENV["Envelope<br/>attack 1 ms, release 100 ms"]
    ENV -.-> GT["Gain table<br/>2:1 above -6 dB (soft knee -9 to -3 dB),<br/>ceiling at 1.0, × 0.75"]
    GT -.-> VCA
    classDef nl fill:#FAECE7,stroke:#D85A30,color:#712B13
    class SC nl
```

- The peak of both sides is held over the look-ahead time, so that the gain is lowered before the
  peak comes out, and the left and the right are lowered together (the stereo image is kept)
- The gain is looked up in a table, by the envelope in a pseudo-logarithmic scale; it holds the
  soft-knee compression (2:1 above -6 dB at its input, with a soft knee from -9 dB to -3 dB), the
  ceiling (the peaks are limited to 1.0 at its input, reached at +6 dB), and the output gain
  (x 0.75, -2.5 dB)
- The gain is recalculated once per 4 samples, and interpolated over the samples in between


## Wave Table Graphs

![Wave Table Graphs](./pra32-u2-wave-table-graphs.png)
