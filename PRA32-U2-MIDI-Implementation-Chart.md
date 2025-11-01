```
  [Polyphonic Synthesizer]                                        Date: 2025-11-01                      
  Model: Digital Synth PRA32-U2   MIDI Implementation Chart       Version: 1.5.0                        
+-------------------------------+---------------+---------------+--------------------------------------+
| Function...                   | Transmitted   | Recognized    | Remarks                              |
+-------------------------------+---------------+---------------+--------------------------------------+
| Basic        Default          | x             | 1             |                                      |
| Channel      Changed          | x             | x             | $5                                   |
+-------------------------------+---------------+---------------+--------------------------------------+
| Mode         Default          | x             | Mode 3        |                                      |
|              Messages         | x             | x             |                                      |
|              Altered          | ************* |               |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Note                          | x             | 0-127         |                                      |
| Number       : True Voice     | ************* | 0-127         |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Velocity     Note ON          | x             | o             |                                      |
|              Note OFF         | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| After        Key's            | x             | x             |                                      |
| Touch        Ch's             | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Pitch Bend                    | x             | o             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Control                     1 | x             | o             | Modulation                           |
| Change                      2 | x             | o             | Breath Controller                    |
|                            64 | x             | o             | Sustain Pedal [Off|On]               |
|                               |               |               |                                      |
|                               |               |               |                                      |
|                           102 | x             | o             | Osc 1 Wave [Saw|Sqr|Tri|Sin|WT|Pls]  |
|                            23 | x             | o             | Mixer Noise/Sub Osc [N|S]            |
|                            19 | x             | o             | Osc 1 Shape $2                       |
|                            20 | x             | o             | Osc 1 Morph $2                       |
|                               |               |               |                                      |
|                           104 | x             | o             | Osc 2 Wave [Saw|Sqr|Tri|Sin|-|Nos]   |
|                            21 | x             | o             | Mixer Osc Mix [1|2]                  |
|                            85 | x             | o             | Osc 2 Coarse [-|+]                   |
|                            76 | x             | o             | Osc 2 Pitch [-|+]                    |
|                               |               |               |                                      |
|                            74 | x             | o             | Filter Cutoff                        |
|                            71 | x             | o             | Filter Resonance                     |
|                            24 | x             | o             | Filter EG Amt [-|+]                  |
|                             9 | x             | o             | Filter Key Track [-|+] $1            |
|                               |               |               |                                      |
|                            73 | x             | o             | EG Attack                            |
|                            75 | x             | o             | EG Decay                             |
|                            30 | x             | o             | EG Sustain                           |
|                            72 | x             | o             | EG Release                           |
|                               |               |               |                                      |
|                            89 | x             | o             | EG Mod Amt [-|+]                     |
|                             8 | x             | o             | EG Mod Dst [P|F|-|2P|-|1S]           |
|                            18 | x             | o             | Voice Mode [Pol|-|-|Mon|LP|Lgt]      |
|                             5 | x             | o             | Portamento                           |
|                               |               |               |                                      |
|                            33 | x             | o             | LFO Wave [Tri|Sin|-|Saw|S&H|Sqr]     |
|                            56 | x             | o             | LFO Fade Time                        |
|                             3 | x             | o             | LFO Rate                             |
|                            17 | x             | o             | LFO Depth                            |
|                               |               |               |                                      |
|                            13 | x             | o             | LFO Mod Amt [-|+]                    |
|                           103 | x             | o             | LFO Mod Dst [P|F|-|2P|-|1S]          |
|                            25 | x             | o             | LFO Filter Amt [-|+]                 |
|                            15 | x             | o             | Amp Gain                             |
|                               |               |               |                                      |
|                            52 | x             | o             | Amp Attack                           |
|                            53 | x             | o             | Amp Decay                            |
|                            54 | x             | o             | Amp Sustain                          |
|                            55 | x             | o             | Amp Release                          |
|                               |               |               |                                      |
|                            39 | x             | o             | Filter Mode [LP|HP]                  |
|                            57 | x             | o             | Pitch Bend Range                     |
|                            36 | x             | o             | EG Amp Mod [Off|On]                  |
|                           105 | x             | o             | Release = Decay [Off|On]             |
|                               |               |               |                                      |
|                            60 | x             | o             | Breath Filter Amt [-|+]              |
|                            61 | x             | o             | Breath Amp Mod [Off|Qad|Lin]         |
|                            62 | x             | o             | EG Velocity Sensitivity              |
|                            63 | x             | o             | Amp Velocity Sensitivity             |
|                               |               |               |                                      |
|                           110 | x             | o             | Voice Assign Mode [1|2]              |
|                               |               |               |                                      |
|                               |               |               |                                      |
|                               |               |               |                                      |
|                               |               |               |                                      |
|                            93 | x             | o             | Chorus Mix [Dry|Wet]                 |
|                               |               |               |                                      |
|                            58 | x             | o             | Chorus Rate                          |
|                            59 | x             | o             | Chorus Depth                         |
|                               |               |               |                                      |
|                            94 | x             | o             | Delay Level                          |
|                            35 | x             | o             | Delay Mode [S|P]                     |
|                            90 | x             | o             | Delay Time                           |
|                            92 | x             | o             | Delay Feedback                       |
|                               |               |               |                                      |
|                               |               |               |                                      |
|                            87 | x             | o             | Program Number to Write to $4        |
|                           106 | x             | o             | Write Parameters to Program $4       |
|                           112 | x             | o             | Program Change #0 by CC              |
|                           113 | x             | o             | Program Change #1 by CC              |
|                           114 | x             | o             | Program Change #2 by CC              |
|                           115 | x             | o             | Program Change #3 by CC              |
|                           116 | x             | o             | Program Change #4 by CC              |
|                           117 | x             | o             | Program Change #5 by CC              |
|                           118 | x             | o             | Program Change #6 by CC              |
|                           119 | x             | o             | Program Change #7 by CC              |
|                           111 | x             | x             | [Reserved]                           |
+-------------------------------+---------------+---------------+--------------------------------------+
| Program                       | x             | o             |                                      |
| Change       : True #         | ************* | 0-15          | Default 8                            |
+-------------------------------+---------------+---------------+--------------------------------------+
| System Exclusive              | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| System       : Song Pos       | x             | x             |                                      |
| Common       : Song Sel       | x             | x             |                                      |
|              : Tune           | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| System       : Clock          | x $6          | x $7          |                                      |
| Real Time    : Commands       | x $6          | x $7          | Compatible only with Start/Stop      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Aux          : All Sound OFF  | x             | o 120         |                                      |
| Messages     : Reset All      | x             | o 121         |                                      |
|                Controllers    |               |               |                                      |
|              : Local ON/OFF   | x             | x             |                                      |
|              : All Notes OFF  | x             | o 123-127     |                                      |
|              : Active Sense   | x             | x             |                                      |
|              : Reset          | x             | x             |                                      |
+-------------------------------+---------------+---------------+--------------------------------------+
| Notes                         | $2 : Disabled if Osc 1 Wave is Tri (Triangle Wave)                   |
|                               | $4 : To write the current parameters to Program #0-7 and the         |
|                               |   flash, set "Program Number to Write to" (# is the value mod 16)    |
|                               |   and then change "Write Parameters to Program" from 0 to 1-127      |
|                               | $5 : Basic Channel can be changed in PRA32-U2 with Panel             |
|                               | $6 : o in PRA32-U2 with Panel (No transmission via USB MIDI)         |
|                               | $7 : o in PRA32-U2 with Panel if Seq Clock Src is External           |
+-------------------------------+----------------------------------------------------------------------+
  Mode 1: Omni On,  Poly          Mode 2: Omni On,  Mono          o: Yes                                
  Mode 3: Omni Off, Poly          Mode 4: Omni Off, Mono          x: No                                 
```
