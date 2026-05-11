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
        synthNode.port.onmessage = async (e) => {
            if (e.data.type === 'wasmLoaded') {
                isAudioInitialized = true;
                statusDiv.textContent = 'Status: ENGINE RUNNING | MIDI READY';
                startBtn.style.display = 'none';

                setupMidi();
                const presetsLoaded = await loadPresets();
                setupControls(presetsLoaded);
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

let factoryPresets = null;

async function loadPresets() {
    try {
        const response = await fetch('data/presets.json');
        const data = await response.json();

        // Store both "current" values (index 0) and factory preset bank (index 1).
        factoryPresets = {};
        for (const key in data) {
            if (!key.startsWith('_')) {
                const paramName = key.trim();
                const currentValues = Array.isArray(data[key]?.[0]) ? data[key][0] : [];
                const presetBank = Array.isArray(data[key]?.[1]) ? data[key][1] : [];
                factoryPresets[paramName] = {
                    current: currentValues,
                    presets: presetBank,
                };
            }
        }

        return true;
    } catch (e) {
        console.error('Failed to load presets', e);
        return false;
    }
}

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

function setupControls(presetsLoaded) {
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

            if (!presetsLoaded || !factoryPresets) {
                console.warn('Preset bank not loaded; preset change ignored.');
                return;
            }

            if (presetIndex < 0) {
                // Restore to current/init value from presets.json first array.
                synthParameters.forEach(param => {
                    const paramKeyName = param.name.replace(/ /g, '_');
                    const presetData = factoryPresets[paramKeyName];
                    if (presetData && presetData.current[0] !== undefined) {
                        const newValue = presetData.current[0];
                        param.value = newValue;
                        updateSlider(param.id, newValue);
                        sendCC(param.cc, newValue);
                    }
                });
                return;
            }

            synthParameters.forEach(param => {
                const paramKeyName = param.name.replace(/ /g, '_');
                const presetData = factoryPresets[paramKeyName];
                if (presetData && presetData.presets[presetIndex] !== undefined) {
                    const newValue = presetData.presets[presetIndex];
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
    const isMobileLayout = window.matchMedia('(max-width: 900px)').matches;
    const keyboardKeyCount = isMobileLayout ? 25 : 13;
    let octaveOffset = 0;
    const octaveMin = -2;
    const octaveMax = 3;
    const baseNote = 60; // C4
    const keyboardDiv = document.getElementById('keyboard');
    keyboardDiv.classList.toggle('mobile', isMobileLayout);
    const keyPattern = ['white', 'black', 'white', 'black', 'white', 'white', 'black', 'white', 'black', 'white', 'black', 'white'];
    const desktopKeys = ['a', 'w', 's', 'e', 'd', 'f', 't', 'g', 'y', 'h', 'u', 'j', 'k'];
    const keys = [];
    for (let i = 0; i < keyboardKeyCount; i += 1) {
        keys.push({
            type: keyPattern[i % 12],
            note: i,
            key: desktopKeys[i]
        });
    }

    function getNoteValue(noteOffset) {
        return baseNote + noteOffset + (octaveOffset * 12);
    }

    keys.forEach(k => {
        const el = document.createElement('div');
        el.className = `key ${k.type}`;
        el.dataset.noteOffset = k.note;
        el.dataset.note = getNoteValue(k.note);

        el.addEventListener('pointerdown', (e) => {
            e.preventDefault();
            sendNoteOn(getNoteValue(k.note));
            el.classList.add('active');
        });
        el.addEventListener('pointerup', (e) => {
            e.preventDefault();
            if(el.classList.contains('active')) {
                sendNoteOff(getNoteValue(k.note));
                el.classList.remove('active');
            }
        });
        el.addEventListener('pointercancel', (e) => {
            e.preventDefault();
            if(el.classList.contains('active')) {
                sendNoteOff(getNoteValue(k.note));
                el.classList.remove('active');
            }
        });
        el.addEventListener('pointerleave', (e) => {
            e.preventDefault();
            if(el.classList.contains('active')) {
                sendNoteOff(getNoteValue(k.note));
                el.classList.remove('active');
            }
        });
        keyboardDiv.appendChild(el);
    });

    const octaveControls = document.getElementById('octave-controls');
    const octaveDisplay = document.getElementById('octave-display');
    const octaveDownBtn = document.getElementById('octave-down');
    const octaveUpBtn = document.getElementById('octave-up');
    const updateOctaveDisplay = () => {
        octaveDisplay.textContent = `C${4 + octaveOffset}`;
        octaveDownBtn.disabled = octaveOffset <= octaveMin;
        octaveUpBtn.disabled = octaveOffset >= octaveMax;
    };

    if (isMobileLayout && octaveControls && octaveDisplay && octaveDownBtn && octaveUpBtn) {
        const refreshKeyNotes = () => {
            document.querySelectorAll('.key[data-note-offset]').forEach((el) => {
                const noteOffset = parseInt(el.dataset.noteOffset);
                el.dataset.note = getNoteValue(noteOffset);
            });
            updateOctaveDisplay();
        };

        octaveDownBtn.addEventListener('click', () => {
            if (octaveOffset > octaveMin) {
                octaveOffset -= 1;
                refreshKeyNotes();
            }
        });

        octaveUpBtn.addEventListener('click', () => {
            if (octaveOffset < octaveMax) {
                octaveOffset += 1;
                refreshKeyNotes();
            }
        });

        updateOctaveDisplay();
    }

    // PC Keyboard mapping
    const keyMap = {};
    keys.forEach(k => {
        if (k.key) keyMap[k.key] = k.note;
    });

    const activeNotes = {};
    window.addEventListener('keydown', (e) => {
        if (e.repeat) return;
        const noteOffset = keyMap[e.key];
        if (noteOffset !== undefined) {
            const note = getNoteValue(noteOffset);
            sendNoteOn(note);
            activeNotes[noteOffset] = true;
            const el = document.querySelector(`.key[data-note-offset="${noteOffset}"]`);
            if (el) el.classList.add('active');
        }
    });

    window.addEventListener('keyup', (e) => {
        const noteOffset = keyMap[e.key];
        if (noteOffset !== undefined && activeNotes[noteOffset]) {
            const note = getNoteValue(noteOffset);
            sendNoteOff(note);
            delete activeNotes[noteOffset];
            const el = document.querySelector(`.key[data-note-offset="${noteOffset}"]`);
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
