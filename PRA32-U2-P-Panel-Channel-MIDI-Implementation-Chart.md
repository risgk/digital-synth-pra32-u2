```
  [Panel Channel]                                                 Experimental                           
  Model: Digital Synth PRA32-U2/P   MIDI Implementation Chart                                            
+-------------------------------+---------------+---------------+---------------------------------------+
| Function...                   | Transmitted   | Recognized    | Remarks                               |
+-------------------------------+---------------+---------------+---------------------------------------+
| Basic        Default          | x             | 11            | Basic Channel + 10 $1                 |
| Channel      Changed          | x             | x             | $1                                    |
+-------------------------------+---------------+---------------+---------------------------------------+
| Mode         Default          | x             | x             |                                       |
|              Messages         | x             | x             |                                       |
|              Altered          | ************* |               |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| Note                          | x             | x             |                                       |
| Number       : True Voice     | ************* | x             |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| Velocity     Note ON          | x             | x             |                                       |
|              Note OFF         | x             | x             |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| After        Key's            | x             | x             |                                       |
| Touch        Ch's             | x             | x             |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| Pitch Bend                    | x             | x             |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| Control                 16-23 | x             | o             | Seq Pitch 0-7                         |
| Change                  24-31 | x             | o             | Seq Velo 0-7                          |
|                               |               |               |                                       |
|                            40 | x             | o             | Panel Scale [Maj|Min|Mel|MaP|MiP|Chr] |
|                            41 | x             | o             | Panel Transpose [-|+]                 |
|                            42 | x             | o             | Panel Play Mode [Nrm|Seq]             |
|                            44 | x             | o             | Panel Play Pitch                      |
|                            45 | x             | o             | Panel Play Velo                       |
|                            46 | x             | o             | Panel Pitch Ofst [-|+]                |
|                               |               |               |                                       |
|                            48 | x             | o             | Seq Tempo                             |
|                            49 | x             | o             | Seq Clock Src [Int|Ext]               |
|                            50 | x             | o             | Seq Gate Time [1/6|...|6/6]           |
|                            51 | x             | o             | Seq Num Steps                         |
|                            52 | x             | o             | Seq Mode [Fwd|Rvs|Bnc]                |
|                            53 | x             | o             | Seq Act Steps                         |
|                            54 | x             | o             | Seq Pitch Ofst [-|+]                  |
|                            55 | x             | o             | Seq Step Note [4|8|16]                |
|                            56 | x             | o             | Seq On Steps                          |
|                            57 | x             | o             | Seq T/Rx St/Sp [Off|On]               |
|                               |               |               |                                       |
|                            58 | x             | o             | Panel Target Ch $2                    |
|                               |               |               |                                       |
|                           102 | x             | o             | Write Panel Prms $3                   |
|                           103 | x             | o             | Read Panel Prms $3                    |
|                           104 | x             | o             | Init Panel Prms $3                    |
|                           105 | x             | o             | Undo Write $3                         |
|                           106 | x             | o             | Undo Read $3                          |
|                           107 | x             | o             | Seq Rand Pitch $3                     |
|                           108 | x             | o             | Seq Rand Velo $3                      |
|                               |               |               |                                       |
|                           111 | x             | o             | Panel Play [Off|On] $4                |
+-------------------------------+---------------+---------------+---------------------------------------+
| Program                       | x             | x             |                                       |
| Change       : True #         | ************* | x             |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| System Exclusive              | x             | x             |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| System       : Song Pos       | x             | x             |                                       |
| Common       : Song Sel       | x             | x             |                                       |
|              : Tune           | x             | x             |                                       |
+-------------------------------+---------------+---------------+---------------------------------------+
| System       : Clock          | x             | x             | See PRA32-U2                          |
| Real Time    : Commands       | x             | x             | See PRA32-U2                          |
+-------------------------------+---------------+---------------+---------------------------------------+
| Aux          : All Sound OFF  | x             | x             |                                       |
| Messages     : Reset All      | x             | x             |                                       |
|                Controllers    |               |               |                                       |
|              : Local ON/OFF   | x             | x             |                                       |
|              : All Notes OFF  | x             | x             |                                       |
|              : Active Sense   | x             | x             |                                       |
|              : Reset          | x             | x             |                                       |
+-------------------------------+-----------------------------------------------------------------------+
| Notes                         | $1 : The Panel Channel is the Basic Channel (of PRA32-U2) + 10, which |
|                               |   PRA32-U2/P has with the panel or without it, enabled by default     |
|                               |   (see "Panel Channel" in README-PRA32-U2-P.md); it follows the Basic |
|                               |   Channel                                                             |
|                               | $2 : PRA32-U2/M/P: the channel that the panel selects, as the Basic   |
|                               |   Channel + the value (0-15, e.g. 1 for Sub Synth 1)                  |
|                               | $3 : Done when changed from 0-31 to 96-127 (as by the panel)          |
|                               | $4 : 64-127: as the Play key pressed (Normal Mode) or the Step        |
|                               |   Sequencer started (Seq Mode); 0-63: as released or stopped          |
+-------------------------------+-----------------------------------------------------------------------+
  Mode 1: Omni On,  Poly          Mode 2: Omni On,  Mono          o: Yes                                 
  Mode 3: Omni Off, Poly          Mode 4: Omni Off, Mono          x: No                                  
```
