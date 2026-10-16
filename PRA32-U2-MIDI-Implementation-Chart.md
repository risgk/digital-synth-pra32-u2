```
  [Polyphonic Synthesizer]                                        Date: 2026-10-16                      
  Model: Digital Synth PRA32-U2   MIDI Implementation Chart       Version: 3.15.0                       
+-------------------------------+---------------+---------------+--------------------------------------+
| Function...                   | Transmitted   | Recognized    | Remarks                              |
+-------------------------------+---------------+---------------+--------------------------------------+
| Basic        Default          | x             | 1             |                                      |
| Channel      Changed          | x             | x             | $3                                   |
+-------------------------------+---------------+---------------+--------------------------------------+
| Mode         Default          | x             | Mode 3        |                                      |
|              Messages         | x             | x             |                                      |
|              Altered          | ************* |               |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Note                          | x             | 0-127         |                                      |
| Number       : True Voice     | ************* | 0-127         |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Velocity     Note ON          | x             | o             |                                      |
|              Note OFF         | x             | o             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| After        Key's            | x             | o             |                                      |
| Touch        Ch's             | x             | o             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Pitch Bend                    | x             | o             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Control                     1 | x             | o             | Modulation Wheel                     |
| Change                      2 | x             | o             | Breath Controller                    |
|                            11 | x             | o             | Expression                           |
|                            64 | x             | o             | Sustain Pedal [Off|On]               |
|                            65 | x             | o             | Portamento Switch [Off|On] $7        |
|                               |               |               |                                      |
|                           102 | x             | o             | Osc 1 Wave [Saw|Sqr|Tri|Sin|WT|Pls]  |
|                            23 | x             | o             | Mixer Noise/Sub Osc [N|S]            |
|                            19 | x             | o             | Osc 1 Shape $1                       |
|                            20 | x             | o             | Osc 1 Morph $1                       |
|                               |               |               |                                      |
|                           104 | x             | o             | Osc 2 Wave [Saw|Sqr|Tri|Sin|O1|Nos]  |
|                            21 | x             | o             | Mixer Osc Mix [1|2]                  |
|                            85 | x             | o             | Osc 2 Coarse [-|+]                   |
|                            76 | x             | o             | Osc 2 Pitch [-|+]                    |
|                               |               |               |                                      |
|                            74 | x             | o             | Filter Cutoff                        |
|                            71 | x             | o             | Filter Resonance                     |
|                            24 | x             | o             | Filter EG Amt [-|+]                  |
|                             9 | x             | o             | Filter Key Track [-|+]               |
|                               |               |               |                                      |
|                            73 | x             | o             | EG Attack                            |
|                            75 | x             | o             | EG Decay                             |
|                            30 | x             | o             | EG Sustain                           |
|                            72 | x             | o             | EG Release                           |
|                               |               |               |                                      |
|                            89 | x             | o             | EG Mod Amt [-|+]                     |
|                             8 | x             | o             | EG Mod Dst [P|P|2P|2P|F|1S]          |
|                            18 | x             | o             | Voice Mode [Pol|Pol|Mon|Mon|LP|Lgt]  |
|                             5 | x             | o             | Portamento Time                      |
|                               |               |               |                                      |
|                            33 | x             | o             | LFO Wave [Tri|Sin|Nos|Saw|S&H|Sqr]   |
|                            56 | x             | o             | LFO Fade Time                        |
|                             3 | x             | o             | LFO Rate                             |
|                            17 | x             | o             | LFO Depth                            |
|                               |               |               |                                      |
|                            13 | x             | o             | LFO Mod Amt [-|+]                    |
|                           103 | x             | o             | LFO Mod Dst [P|P|2P|2P|F|1S]         |
|                            25 | x             | o             | LFO Filter Amt [-|+]                 |
|                            15 | x             | o             | Amp Gain                             |
|                               |               |               |                                      |
|                            52 | x             | o             | Amp Attack                           |
|                            53 | x             | o             | Amp Decay                            |
|                            54 | x             | o             | Amp Sustain                          |
|                            55 | x             | o             | Amp Release                          |
|                               |               |               |                                      |
|                            39 | x             | o             | Filter Mode [LP|BP|HP]               |
|                            57 | x             | o             | Pitch Bend Range                     |
|                            36 | x             | o             | EG Amp Mod [Off|On]                  |
|                           105 | x             | o             | EG/Amp Rel = Dec [Off|On]            |
|                               |               |               |                                      |
|                            51 | x             | o             | EG Att/Dec Velo Sens [-|+]           |
|                            81 | x             | o             | EG Rel Velo Sens [-|+]               |
|                            62 | x             | o             | EG Level Velo Sens                   |
|                            63 | x             | o             | Amp Level Velo Sens                  |
|                               |               |               |                                      |
|                            16 | x             | o             | EG Att/Dec Key Track [-|+]           |
|                           110 | x             | o             | Voice Assign Mode [1|1|3|3|4|2]      |
|                            10 | x             | o             | Pan                                  |
|                            22 | x             | o             | Stretch Tune [-|+]                   |
|                               |               |               |                                      |
|                            82 | x             | o             | Osc/Filter Drift                     |
|                            83 | x             | o             | Osc Saw Wave Mode [Str|Cur]          |
|                            86 | x             | o             | Coarse Tune [-|+]                    |
|                            70 | x             | o             | Fine Tune [-|+]                      |
|                               |               |               |                                      |
|                            60 | x             | o             | Breath Filter Amt [-|+]              |
|                            61 | x             | o             | Breath Amp Mode [Off|Q|L|LO|QO|Opn]  |
|                            37 | x             | o             | After Touch LFO Amt                  |
|                            34 | x             | o             | Portamento Mode [Sw|Aut]             |
|                               |               |               |                                      |
|                            93 | x             | o             | Chorus Level                         |
|                            95 | x             | o             | FX Routing [Cho|Dly|Byp]             |
|                               |               |               |                                      |
|                            58 | x             | o             | Chorus Rate                          |
|                            59 | x             | o             | Chorus Depth                         |
|                               |               |               |                                      |
|                            94 | x             | o             | Delay Level                          |
|                            35 | x             | o             | Delay Mode [Ste|PP|Rev]              |
|                            90 | x             | o             | Delay Time                           |
|                            92 | x             | o             | Delay Feedback                       |
|                               |               |               |                                      |
|                               |               |               |                                      |
|                            87 | x             | o             | Program Number to Write to $2        |
|                           106 | x             | o             | Write Parameters to Program $2       |
|                           112 | x             | o             | Program Change #0 by CC $8           |
|                           113 | x             | o             | Program Change #1 by CC $8           |
|                           114 | x             | o             | Program Change #2 by CC $8           |
|                           115 | x             | o             | Program Change #3 by CC $8           |
|                           116 | x             | o             | Program Change #4 by CC $8           |
|                           117 | x             | o             | Program Change #5 by CC $8           |
|                           118 | x             | o             | Program Change #6 by CC $8           |
|                           119 | x             | o             | Program Change #7 by CC $8           |
|                           111 | x             | o             | Randomize Synth $6                   |
+-------------------------------+---------------+---------------+--------------------------------------+
| Program                       | x             | o             |                                      |
| Change       : True #         | ************* | 0-31, 127     | Default 0, #127: Randomize Synth     |
+-------------------------------+---------------+---------------+--------------------------------------+
| System Exclusive              | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| System       : Song Pos       | x             | x             |                                      |
| Common       : Song Sel       | x             | x             |                                      |
|              : Tune           | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| System       : Clock          | x $4          | x $5          |                                      |
| Real Time    : Commands       | x $4          | x $5          | Compatible only with Start/Stop      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Aux          : All Sound OFF  | x             | o 120         |                                      |
| Messages     : Reset All      | x             | o 121         |                                      |
|                Controllers    |               |               |                                      |
|              : Local ON/OFF   | x             | x             |                                      |
|              : All Notes OFF  | x             | o 123-127     |                                      |
|              : Active Sense   | x             | x             |                                      |
|              : Reset          | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Notes                         | $1 : Disabled if Osc 1 Wave is Tri (Triangle Wave)                   |
|                               | $2 : To write the current parameters to Program #0-15 and the        |
|                               |   flash, set "Program Number to Write to" (# is the value mod 32;    |
|                               |   #16-31 are not written), and then change "Write Parameters to      |
|                               |   Program" from 0-31 to 96-127                                       |
|                               | $3 : Basic Channel can be changed in PRA32-U2/P                      |
|                               | $4 : o in PRA32-U2/P (No transmission via USB MIDI)                  |
|                               | $5 : o in PRA32-U2/P if Seq Clock Src is External                    |
|                               | $6 : Randomize the synth parameters (as "Randomize Synth" in         |
|                               |   PRA32-U2 Editor) when changed from 0-63 to 64-127                  |
|                               | $7 : Effective only if Portamento Mode is Sw                         |
|                               | $8 : Change to the program when changed from 0-63 to 64-127          |
+-------------------------------+----------------------------------------------------------------------+
  Mode 1: Omni On,  Poly          Mode 2: Omni On,  Mono          o: Yes                                
  Mode 3: Omni Off, Poly          Mode 4: Omni Off, Mono          x: No                                 
```
