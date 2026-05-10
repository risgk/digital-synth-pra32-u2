let audioContext;
let synthNode;
let isAudioInitialized = false;

const startBtn = document.getElementById('start-btn');
const statusDiv = document.getElementById('status');

// Helper to fetch the wasm binary
async function loadWasm() {
    const response = await fetch('synth.wasm');
    const buffer = await response.arrayBuffer();
    return buffer;
}

startBtn.addEventListener('click', async () => {
    if (isAudioInitialized) return;

    try {
        startBtn.textContent = 'INITIALIZING...';

        audioContext = new (window.AudioContext || window.webkitAudioContext)({
            sampleRate: 48000
        });

        await audioContext.audioWorklet.addModule('synth-processor.js');

        synthNode = new AudioWorkletNode(audioContext, 'synth-processor', {
            outputChannelCount: [2]
        });

        synthNode.connect(audioContext.destination);

        const wasmBytes = await loadWasm();

        // Wait for processor to say it's loaded
        synthNode.port.onmessage = (e) => {
            if (e.data.type === 'wasmLoaded') {
                isAudioInitialized = true;
                statusDiv.textContent = 'Status: ENGINE RUNNING | MIDI READY';
                startBtn.style.display = 'none';

                setupMidi();
                setupControls();
            }
        };

        // Send WASM to the worklet
        synthNode.port.postMessage({
            type: 'loadWasm',
            wasmBytes: wasmBytes
        });

    } catch (err) {
        console.error(err);
        statusDiv.textContent = 'Error: ' + err.message;
        startBtn.textContent = 'RETRY';
    }
});

// --- MIDI AND CONTROLS ---

function sendCC(cc, value) {
    if (synthNode) {
        synthNode.port.postMessage({ type: 'controlChange', cc, value });
    }
}

function sendNoteOn(note, velocity = 100) {
    if (synthNode) {
        synthNode.port.postMessage({ type: 'noteOn', note, velocity });
    }
}

function sendNoteOff(note) {
    if (synthNode) {
        synthNode.port.postMessage({ type: 'noteOff', note });
    }
}

const factoryPresets = {
  "OSC_1_WAVE"     : [ 0  , 0  , 76 , 127, 0  , 25 , 0  , 0  ],
  "MIXER_SUB_OSC"  : [ 64 , 64 , 64 , 64 , 127, 96 , 127, 64 ],
  "OSC_1_SHAPE"    : [ 64 , 64 , 0  , 0  , 64 , 64 , 0  , 0  ],
  "OSC_1_MORPH"    : [ 0  , 127, 108, 64 , 0  , 127, 0  , 0  ],

  "OSC_2_WAVE"     : [ 0  , 0  , 0  , 0  , 0  , 25 , 0  , 0  ],
  "MIXER_OSC_MIX"  : [ 64 , 0  , 64 , 0  , 64 , 0  , 64 , 0  ],
  "OSC_2_COARSE"   : [ 64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ],
  "OSC_2_PITCH"    : [ 72 , 72 , 72 , 72 , 66 , 72 , 66 , 64 ],

  "FILTER_CUTOFF"  : [ 112, 112, 88 , 127, 88 , 112, 40 , 127],
  "FILTER_RESO"    : [ 48 , 48 , 48 , 48 , 48 , 48 , 80 , 0  ],
  "FILTER_EG_AMT"  : [ 40 , 64 , 64 , 64 , 76 , 64 , 88 , 64 ],
  "FILTER_KEY_TRK" : [ 96 , 96 , 96 , 96 , 64 , 64 , 64 , 64 ],

  "EG_ATTACK"      : [ 96 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ],
  "EG_DECAY"       : [ 96 , 32 , 96 , 32 , 32 , 96 , 100, 0  ],
  "EG_SUSTAIN"     : [ 0  , 127, 0  , 127, 127, 0  , 0  , 127],
  "EG_RELEASE"     : [ 32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ],

  "EG_OSC_AMT"     : [ 64 , 64 , 72 , 64 , 64 , 72 , 64 , 64 ],
  "EG_OSC_DST"     : [ 0  , 0  , 127, 0  , 0  , 127, 0  , 0  ],
  "VOICE_MODE"     : [ 0  , 0  , 0  , 0  , 127, 76 , 76 , 127],
  "PORTAMENTO"     : [ 48 , 0  , 0  , 0  , 48 , 48 , 0  , 0  ],

  "LFO_WAVE"       : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "LFO_FADE_TIME"  : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "LFO_RATE"       : [ 80 , 80 , 80 , 80 , 80 , 80 , 80 , 80 ],
  "LFO_DEPTH"      : [ 0  , 0  , 0  , 127, 8  , 0  , 0  , 0  ],

  "LFO_OSC_AMT"    : [ 64 , 64 , 64 , 64 , 96 , 72 , 64 , 64 ],
  "LFO_OSC_DST"    : [ 0  , 0  , 127, 0  , 0  , 127, 0  , 0  ],
  "LFO_FILTER_AMT" : [ 76 , 76 , 76 , 64 , 64 , 64 , 76 , 64 ],
  "AMP_GAIN"       : [ 100, 100, 120, 100, 100, 90 , 110, 100],

  "AMP_ATTACK"     : [ 32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ],
  "AMP_DECAY"      : [ 32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ],
  "AMP_SUSTAIN"    : [ 127, 127, 127, 127, 127, 127, 127, 127],
  "AMP_RELEASE"    : [ 32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ],

  "FILTER_MODE"    : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "P_BEND_RANGE"   : [ 2  , 2  , 2  , 2  , 2  , 2  , 2  , 2  ],
  "EG_AMP_MOD"     : [ 0  , 127, 127, 127, 0  , 0  , 127, 0  ],
  "REL_EQ_DECAY"   : [ 127, 127, 127, 127, 127, 127, 127, 0  ],

  "BTH_FILTER_AMT" : [ 64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ],
  "BTH_AMP_MOD"    : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "EG_VEL_SENS"    : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "AMP_VEL_SENS"   : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],

  "AFT_T_LFO_AMT"  : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "VOICE_ASGN_MODE": [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "PAN"            : [ 64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ],

  "OSC_DRIFT"      : [ 32 , 32 , 32 , 32 , 32 , 32 , 32 , 32 ],
  "OSC_SAW_W_MODE" : [ 127, 127, 127, 127, 127, 127, 127, 127],

  "CHORUS_MIX"     : [ 127, 127, 127, 127, 127, 127, 127, 0  ],
  "CHORUS_RATE"    : [ 64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ],
  "CHORUS_DEPTH"   : [ 64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ],

  "DELAY_LEVEL"    : [ 64 , 64 , 64 , 64 , 64 , 64 , 64 , 0  ],
  "DELAY_MODE"     : [ 0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ],
  "DELAY_TIME"     : [ 87 , 87 , 87 , 87 , 87 , 87 , 87 , 87 ],
  "DELAY_FEEDBACK" : [ 64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ]
};

const synthParameters = [
  { id: 'osc1Wave', name: 'OSC 1 WAVE', cc: 102, value: 0 },
  { id: 'mixerSubOsc', name: 'MIXER SUB OSC', cc: 23, value: 64 },
  { id: 'osc1Shape', name: 'OSC 1 SHAPE', cc: 19, value: 0 },
  { id: 'osc1Morph', name: 'OSC 1 MORPH', cc: 20, value: 0 },
  { id: 'osc2Wave', name: 'OSC 2 WAVE', cc: 104, value: 0 },
  { id: 'mixerOscMix', name: 'MIXER OSC MIX', cc: 21, value: 0 },
  { id: 'osc2Coarse', name: 'OSC 2 COARSE', cc: 85, value: 64 },
  { id: 'osc2Pitch', name: 'OSC 2 PITCH', cc: 76, value: 64 },
  { id: 'filterCutoff', name: 'FILTER CUTOFF', cc: 74, value: 127 },
  { id: 'filterReso', name: 'FILTER RESO', cc: 71, value: 0 },
  { id: 'filterEgAmt', name: 'FILTER EG AMT', cc: 24, value: 64 },
  { id: 'filterKeyTrk', name: 'FILTER KEY TRK', cc: 9, value: 64 },
  { id: 'egAttack', name: 'EG ATTACK', cc: 73, value: 0 },
  { id: 'egDecay', name: 'EG DECAY', cc: 75, value: 0 },
  { id: 'egSustain', name: 'EG SUSTAIN', cc: 30, value: 127 },
  { id: 'egRelease', name: 'EG RELEASE', cc: 72, value: 0 },
  { id: 'egOscAmt', name: 'EG OSC AMT', cc: 89, value: 64 },
  { id: 'egOscDst', name: 'EG OSC DST', cc: 8, value: 0 },
  { id: 'voiceMode', name: 'VOICE MODE', cc: 18, value: 127 },
  { id: 'portamento', name: 'PORTAMENTO', cc: 5, value: 0 },
  { id: 'lfoWave', name: 'LFO WAVE', cc: 33, value: 0 },
  { id: 'lfoFadeTime', name: 'LFO FADE TIME', cc: 56, value: 0 },
  { id: 'lfoRate', name: 'LFO RATE', cc: 3, value: 80 },
  { id: 'lfoDepth', name: 'LFO DEPTH', cc: 17, value: 0 },
  { id: 'lfoOscAmt', name: 'LFO OSC AMT', cc: 13, value: 64 },
  { id: 'lfoOscDst', name: 'LFO OSC DST', cc: 103, value: 0 },
  { id: 'lfoFilterAmt', name: 'LFO FILTER AMT', cc: 25, value: 64 },
  { id: 'ampGain', name: 'AMP GAIN', cc: 15, value: 100 },
  { id: 'ampAttack', name: 'AMP ATTACK', cc: 52, value: 0 },
  { id: 'ampDecay', name: 'AMP DECAY', cc: 53, value: 0 },
  { id: 'ampSustain', name: 'AMP SUSTAIN', cc: 54, value: 127 },
  { id: 'ampRelease', name: 'AMP RELEASE', cc: 55, value: 0 },
  { id: 'filterMode', name: 'FILTER MODE', cc: 39, value: 0 },
  { id: 'pBendRange', name: 'P BEND RANGE', cc: 57, value: 2 },
  { id: 'egAmpMod', name: 'EG AMP MOD', cc: 36, value: 0 },
  { id: 'relEqDecay', name: 'REL EQ DECAY', cc: 105, value: 0 },
  { id: 'bthFilterAmt', name: 'BTH FILTER AMT', cc: 60, value: 64 },
  { id: 'bthAmpMod', name: 'BTH AMP MOD', cc: 61, value: 0 },
  { id: 'egVelSens', name: 'EG VEL SENS', cc: 62, value: 0 },
  { id: 'ampVelSens', name: 'AMP VEL SENS', cc: 63, value: 0 },
  { id: 'aftTLfoAmt', name: 'AFT T LFO AMT', cc: 109, value: 0 },
  { id: 'voiceAsgnMode', name: 'VOICE ASGN MODE', cc: 110, value: 0 },
  { id: 'pan', name: 'PAN', cc: 10, value: 64 },
  { id: 'oscDrift', name: 'OSC DRIFT', cc: 82, value: 32 },
  { id: 'oscSawWMode', name: 'OSC SAW W MODE', cc: 83, value: 127 },
  { id: 'chorusMix', name: 'CHORUS MIX', cc: 93, value: 0 },
  { id: 'chorusRate', name: 'CHORUS RATE', cc: 58, value: 64 },
  { id: 'chorusDepth', name: 'CHORUS DEPTH', cc: 59, value: 64 },
  { id: 'delayLevel', name: 'DELAY LEVEL', cc: 94, value: 0 },
  { id: 'delayMode', name: 'DELAY MODE', cc: 35, value: 0 },
  { id: 'delayTime', name: 'DELAY TIME', cc: 90, value: 87 },
  { id: 'delayFeedback', name: 'DELAY FEEDBACK', cc: 92, value: 64 }
];

const ccToParam = new Map(synthParameters.map(p => [p.cc, p]));

function setupControls() {
    const controlsDiv = document.getElementById('synth-controls');
    controlsDiv.innerHTML = ''; // Clear initial layout

    synthParameters.forEach(param => {
        const group = document.createElement('div');
        group.className = 'control-group';

        const label = document.createElement('label');
        label.htmlFor = param.id;
        label.textContent = `${param.name} (CC ${param.cc})`;

        const input = document.createElement('input');
        input.type = 'range';
        input.id = param.id;
        input.min = 0;
        input.max = 127;
        input.value = param.value;

        input.addEventListener('input', (e) => {
            sendCC(param.cc, parseInt(e.target.value));
        });

        // Sync initial value immediately
        sendCC(param.cc, param.value);

        group.appendChild(label);
        group.appendChild(input);
        controlsDiv.appendChild(group);
    });

    // Preset selection
    const presetSelect = document.getElementById('preset-select');
    if (presetSelect) {
        presetSelect.addEventListener('change', (e) => {
            const presetIndex = parseInt(e.target.value);
            if (presetIndex < 0) return;

            synthParameters.forEach(param => {
                const paramKeyName = param.name.replace(/ /g, '_');
                if (factoryPresets[paramKeyName] && factoryPresets[paramKeyName][presetIndex] !== undefined) {
                    const newValue = factoryPresets[paramKeyName][presetIndex];
                    param.value = newValue;
                    updateSlider(param.id, newValue);
                    sendCC(param.cc, newValue);
                }
            });
        });
    }

    // Hacker Controls
    const hackerControls = [
        { id: 'bitCrush', name: 'Hacker: Bitcrush', min: 0, max: 1, step: 0.01, value: 0 },
        { id: 'pwmSimulate', name: 'Hacker: PWM Noise', min: 0, max: 1, step: 0.01, value: 0 }
    ];

    hackerControls.forEach(param => {
        const group = document.createElement('div');
        group.className = 'control-group';

        const label = document.createElement('label');
        label.htmlFor = param.id;
        label.textContent = param.name;

        const input = document.createElement('input');
        input.type = 'range';
        input.id = param.id;
        input.min = param.min;
        input.max = param.max;
        input.step = param.step;
        input.value = param.value;

        const synthParam = synthNode.parameters.get(param.id);
        input.addEventListener('input', (e) => {
            synthParam.value = parseFloat(e.target.value);
        });

        group.appendChild(label);
        group.appendChild(input);
        controlsDiv.appendChild(group);
    });

    // Keyboard
    const baseNote = 60; // C4
    const keyboardDiv = document.getElementById('keyboard');
    const keys = [
        { type: 'white', note: 0, key: 'a' },
        { type: 'black', note: 1, key: 'w' },
        { type: 'white', note: 2, key: 's' },
        { type: 'black', note: 3, key: 'e' },
        { type: 'white', note: 4, key: 'd' },
        { type: 'white', note: 5, key: 'f' },
        { type: 'black', note: 6, key: 't' },
        { type: 'white', note: 7, key: 'g' },
        { type: 'black', note: 8, key: 'y' },
        { type: 'white', note: 9, key: 'h' },
        { type: 'black', note: 10, key: 'u' },
        { type: 'white', note: 11, key: 'j' },
        { type: 'white', note: 12, key: 'k' },
    ];

    keys.forEach(k => {
        const el = document.createElement('div');
        el.className = `key ${k.type}`;
        el.dataset.note = baseNote + k.note;

        el.addEventListener('pointerdown', (e) => {
            e.preventDefault();
            sendNoteOn(baseNote + k.note);
            el.classList.add('active');
        });
        el.addEventListener('pointerup', (e) => {
            e.preventDefault();
            if(el.classList.contains('active')) {
                sendNoteOff(baseNote + k.note);
                el.classList.remove('active');
            }
        });
        el.addEventListener('pointercancel', (e) => {
            e.preventDefault();
            if(el.classList.contains('active')) {
                sendNoteOff(baseNote + k.note);
                el.classList.remove('active');
            }
        });
        el.addEventListener('pointerleave', (e) => {
            e.preventDefault();
            if(el.classList.contains('active')) {
                sendNoteOff(baseNote + k.note);
                el.classList.remove('active');
            }
        });
        keyboardDiv.appendChild(el);
    });

    // PC Keyboard mapping
    const keyMap = {};
    keys.forEach(k => {
        keyMap[k.key] = baseNote + k.note;
    });

    const activeNotes = {};
    window.addEventListener('keydown', (e) => {
        if (e.repeat) return;
        const note = keyMap[e.key];
        if (note !== undefined) {
            sendNoteOn(note);
            activeNotes[note] = true;
            const el = document.querySelector(`.key[data-note="${note}"]`);
            if (el) el.classList.add('active');
        }
    });

    window.addEventListener('keyup', (e) => {
        const note = keyMap[e.key];
        if (note !== undefined && activeNotes[note]) {
            sendNoteOff(note);
            delete activeNotes[note];
            const el = document.querySelector(`.key[data-note="${note}"]`);
            if (el) el.classList.remove('active');
        }
    });
}

// --- WEB MIDI API ---

function setupMidi() {
    if (navigator.requestMIDIAccess) {
        navigator.requestMIDIAccess().then(onMIDISuccess, onMIDIFailure);
    } else {
        statusDiv.textContent += ' | Web MIDI API not supported';
    }
}

function onMIDISuccess(midiAccess) {
    const inputs = midiAccess.inputs.values();
    for (let input = inputs.next(); input && !input.done; input = inputs.next()) {
        input.value.onmidimessage = onMIDIMessage;
    }
    midiAccess.onstatechange = (e) => {
        if (e.port.state === 'connected' && e.port.type === 'input') {
            e.port.onmidimessage = onMIDIMessage;
        }
    };
}

function onMIDIFailure() {
    console.log('Could not access your MIDI devices.');
}

function onMIDIMessage(message) {
    const command = message.data[0] >> 4;
    const channel = message.data[0] & 0xf;
    const data1 = message.data[1];
    const data2 = message.data[2];

    if (command === 9) { // Note On
        if (data2 > 0) {
            sendNoteOn(data1, data2);
            highlightKey(data1, true);
        } else {
            sendNoteOff(data1);
            highlightKey(data1, false);
        }
    } else if (command === 8) { // Note Off
        sendNoteOff(data1);
        highlightKey(data1, false);
    } else if (command === 11) { // Control Change
        sendCC(data1, data2);

        // Update UI if we have a matching control
        const param = ccToParam.get(data1);
        if (param) {
            updateSlider(param.id, data2);
        }
    }
}

function updateSlider(id, value) {
    const slider = document.getElementById(id);
    if (slider) slider.value = value;
}

function highlightKey(note, active) {
    const el = document.querySelector(`.key[data-note="${note}"]`);
    if (el) {
        if (active) el.classList.add('active');
        else el.classList.remove('active');
    }
}
