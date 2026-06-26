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
    // OSC Tab
    { id: 'osc1Wave', label: 'OSC 1 WAVE', cc: 24, min: 0, max: 127, val: 0, tab: 'OSC', type: 'h' },
    { id: 'osc1Shape', label: 'OSC 1 SHAPE', cc: 70, min: 0, max: 127, val: 0, tab: 'OSC', type: 'h' },
    { id: 'osc1Morph', label: 'OSC 1 MORPH', cc: 104, min: 0, max: 127, val: 0, tab: 'OSC', type: 'h' },
    { id: 'osc2Wave', label: 'OSC 2 WAVE', cc: 25, min: 0, max: 127, val: 0, tab: 'OSC', type: 'h' },
    { id: 'osc2Coarse', label: 'OSC 2 COARSE', cc: 20, min: 0, max: 127, val: 64, tab: 'OSC', type: 'h' },
    { id: 'osc2Pitch', label: 'OSC 2 PITCH', cc: 21, min: 0, max: 127, val: 64, tab: 'OSC', type: 'h' },
    { id: 'oscMix', label: 'OSC MIX', cc: 22, min: 0, max: 127, val: 64, tab: 'OSC', type: 'v' },
    { id: 'subOsc', label: 'SUB OSC', cc: 23, min: 0, max: 127, val: 0, tab: 'OSC', type: 'v' },
    { id: 'oscDrift', label: 'OSC DRIFT', cc: 26, min: 0, max: 127, val: 0, tab: 'OSC', type: 'h' },
    { id: 'sawWMode', label: 'SAW W MODE', cc: 27, min: 0, max: 127, val: 0, tab: 'OSC', type: 'h' },

    // FILTER Tab
    { id: 'filterCutoff', label: 'CUTOFF', cc: 74, min: 0, max: 127, val: 127, tab: 'FILTER', type: 'h' },
    { id: 'filterReso', label: 'RESO', cc: 71, min: 0, max: 127, val: 0, tab: 'FILTER', type: 'h' },
    { id: 'filterMode', label: 'MODE', cc: 28, min: 0, max: 127, val: 0, tab: 'FILTER', type: 'h' },
    { id: 'egFltAmt', label: 'EG -> FLT', cc: 29, min: 0, max: 127, val: 64, tab: 'FILTER', type: 'h' },
    { id: 'filterKeyTrk', label: 'KEY TRK', cc: 30, min: 0, max: 127, val: 64, tab: 'FILTER', type: 'h' },
    { id: 'bthFltAmt', label: 'BTH -> FLT', cc: 31, min: 0, max: 127, val: 64, tab: 'FILTER', type: 'h' },
    { id: 'relEqDcy', label: 'REL EQ DCY', cc: 88, min: 0, max: 127, val: 0, tab: 'FILTER', type: 'h' },

    // ENVS Tab
    { id: 'egOscAmt', label: 'EG -> OSC', cc: 105, min: 0, max: 127, val: 64, tab: 'ENVS', type: 'h' },
    { id: 'egAttack', label: 'EG ATTACK', cc: 73, min: 0, max: 127, val: 0, tab: 'ENVS', type: 'v' },
    { id: 'egDecay', label: 'EG DECAY', cc: 75, min: 0, max: 127, val: 64, tab: 'ENVS', type: 'v' },
    { id: 'egSustain', label: 'EG SUSTAIN', cc: 33, min: 0, max: 127, val: 127, tab: 'ENVS', type: 'v' },
    { id: 'egRelease', label: 'EG RELEASE', cc: 72, min: 0, max: 127, val: 64, tab: 'ENVS', type: 'v' },
    { id: 'ampAttack', label: 'AMP ATTACK', cc: 106, min: 0, max: 127, val: 0, tab: 'ENVS', type: 'v' },
    { id: 'ampDecay', label: 'AMP DECAY', cc: 107, min: 0, max: 127, val: 64, tab: 'ENVS', type: 'v' },
    { id: 'ampSustain', label: 'AMP SUSTAIN', cc: 108, min: 0, max: 127, val: 127, tab: 'ENVS', type: 'v' },
    { id: 'ampRelease', label: 'AMP RELEASE', cc: 109, min: 0, max: 127, val: 64, tab: 'ENVS', type: 'v' },

    // MOD Tab
    { id: 'lfoWave', label: 'LFO WAVE', cc: 14, min: 0, max: 127, val: 0, tab: 'MOD', type: 'h' },
    { id: 'lfoRate', label: 'LFO RATE', cc: 76, min: 0, max: 127, val: 64, tab: 'MOD', type: 'h' },
    { id: 'lfoFltAmt', label: 'LFO -> FLT', cc: 15, min: 0, max: 127, val: 64, tab: 'MOD', type: 'h' },
    { id: 'lfoOscAmt', label: 'LFO -> OSC', cc: 16, min: 0, max: 127, val: 64, tab: 'MOD', type: 'h' },
    { id: 'lfoFadeTime', label: 'LFO FADE', cc: 17, min: 0, max: 127, val: 0, tab: 'MOD', type: 'h' },
    { id: 'pbRange', label: 'PB RANGE', cc: 85, min: 0, max: 127, val: 2, tab: 'MOD', type: 'h' },
    { id: 'coarseTune', label: 'COARSE TUNE', cc: 86, min: 0, max: 127, val: 64, tab: 'MOD', type: 'h' },
    { id: 'fineTune', label: 'FINE TUNE', cc: 87, min: 0, max: 127, val: 64, tab: 'MOD', type: 'h' },
    { id: 'portaTime', label: 'PORTA TIME', cc: 5, min: 0, max: 127, val: 0, tab: 'MOD', type: 'h' },
    { id: 'modRate', label: 'MOD RATE', cc: 110, min: 0, max: 127, val: 64, tab: 'MOD', type: 'h' },
    { id: 'modDepth', label: 'MOD DEPTH', cc: 111, min: 0, max: 127, val: 0, tab: 'MOD', type: 'h' },

    // FX Tab
    { id: 'choRate', label: 'CHO RATE', cc: 93, min: 0, max: 127, val: 0, tab: 'FX', type: 'h' },
    { id: 'choDepth', label: 'CHO DEPTH', cc: 114, min: 0, max: 127, val: 0, tab: 'FX', type: 'h' },
    { id: 'delayTime', label: 'DLY TIME', cc: 112, min: 0, max: 127, val: 64, tab: 'FX', type: 'h' },
    { id: 'delayDepth', label: 'DLY DEPTH', cc: 113, min: 0, max: 127, val: 0, tab: 'FX', type: 'h' },
    { id: 'pan', label: 'PAN', cc: 10, min: 0, max: 127, val: 64, tab: 'FX', type: 'h' },
    { id: 'volume', label: 'VOLUME', cc: 7, min: 0, max: 127, val: 100, tab: 'FX', type: 'v' },
    { id: 'ampExpnt', label: 'AMP EXP', cc: 116, min: 0, max: 127, val: 0, tab: 'FX', type: 'h' },
    { id: 'ampGain', label: 'AMP GAIN', cc: 117, min: 0, max: 127, val: 64, tab: 'FX', type: 'h' },
    { id: 'pannerType', label: 'PAN TYPE', cc: 118, min: 0, max: 127, val: 0, tab: 'FX', type: 'h' },
    { id: 'choType', label: 'CHO TYPE', cc: 119, min: 0, max: 127, val: 0, tab: 'FX', type: 'h' },

    // LO-FI Tab
    { id: 'lfoWaveForm', label: 'LFO FORM', cc: 89, min: 0, max: 127, val: 0, tab: 'LO-FI', type: 'h' },
    { id: 'portaMode', label: 'PRTA MODE', cc: 90, min: 0, max: 127, val: 0, tab: 'LO-FI', type: 'h' },
    { id: 'pwmRate', label: 'PWM RATE', cc: 115, min: 0, max: 127, val: 64, tab: 'LO-FI', type: 'h' },
    { id: 'lfoSync', label: 'LFO SYNC', cc: 120, min: 0, max: 127, val: 0, tab: 'LO-FI', type: 'h' },
    { id: 'noiseMode', label: 'NOISE MODE', cc: 121, min: 0, max: 127, val: 0, tab: 'LO-FI', type: 'h' },
    { id: 'distMode', label: 'DIST MODE', cc: 122, min: 0, max: 127, val: 0, tab: 'LO-FI', type: 'h' },
    { id: 'bitCrush', label: 'BITCRUSH', cc: 123, min: 0, max: 127, val: 0, tab: 'LO-FI', type: 'h' },
    { id: 'vcfType', label: 'VCF TYPE', cc: 124, min: 0, max: 127, val: 0, tab: 'LO-FI', type: 'h' }
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
    const mobileMediaQuery = window.matchMedia('(max-width: 900px)');
    let isMobileLayout = mobileMediaQuery.matches;
    let octaveOffset = 0;
    const octaveMin = -2;
    const octaveMax = 3;
    const baseNote = 60; // C4
    const keyboardDiv = document.getElementById('keyboard');
    const keyPattern = ['white', 'black', 'white', 'black', 'white', 'white', 'black', 'white', 'black', 'white', 'black', 'white'];
    const desktopKeys = ['a', 'w', 's', 'e', 'd', 'f', 't', 'g', 'y', 'h', 'u', 'j', 'k'];
    let keys = [];

    function getNoteValue(noteOffset) {
        return baseNote + noteOffset + (octaveOffset * 12);
    }

    const activePointerNotes = new Map();

    const octaveControls = document.getElementById('octave-controls');
    const octaveDisplay = document.getElementById('octave-display');
    const octaveDownBtn = document.getElementById('octave-down');
    const octaveUpBtn = document.getElementById('octave-up');
    const updateOctaveDisplay = () => {
        octaveDisplay.textContent = `C${4 + octaveOffset}`;
        octaveDownBtn.disabled = octaveOffset <= octaveMin;
        octaveUpBtn.disabled = octaveOffset >= octaveMax;
    };

    const refreshKeyNotes = () => {
            document.querySelectorAll('.key').forEach((el) => {
                el.classList.remove('active');
                if (el.dataset.noteOffset) {
                    const noteOffset = parseInt(el.dataset.noteOffset, 10);
                    if (activePointerNotes.has(el)) {
                        sendNoteOff(activePointerNotes.get(el));
                        activePointerNotes.delete(el);
                    }
                    delete activeNotes[noteOffset];
                    el.dataset.activeNote = '';
                    el.dataset.note = getNoteValue(noteOffset);
                }
            });
            if (isMobileLayout) {
                updateOctaveDisplay();
            }
    };

    const updateOctaveControlsVisibility = () => {
        if (!octaveControls) return;
        octaveControls.style.display = isMobileLayout ? 'flex' : 'none';
    };

    const buildKeyboard = () => {
        keyboardDiv.innerHTML = '';
        keyboardDiv.classList.toggle('mobile', isMobileLayout);
        keys = [];
        const keyboardKeyCount = isMobileLayout ? 25 : 13;
        for (let i = 0; i < keyboardKeyCount; i += 1) {
            keys.push({
                type: keyPattern[i % 12],
                note: i,
                key: desktopKeys[i]
            });
        }
        keys.forEach(k => {
            const el = document.createElement('div');
            el.className = `key ${k.type}`;
            el.dataset.noteOffset = k.note;
            el.dataset.note = getNoteValue(k.note);
            el.dataset.activeNote = '';

            const handleRelease = (e) => {
                e.preventDefault();
                const activeNote = parseInt(el.dataset.activeNote, 10);
                if (!Number.isNaN(activeNote)) {
                    sendNoteOff(activeNote);
                    activePointerNotes.delete(el);
                    el.dataset.activeNote = '';
                    el.classList.remove('active');
                }
            };

            el.addEventListener('pointerdown', (e) => {
                e.preventDefault();
                const activeNote = getNoteValue(k.note);
                el.dataset.activeNote = `${activeNote}`;
                activePointerNotes.set(el, activeNote);
                sendNoteOn(activeNote);
                el.classList.add('active');
            });
            el.addEventListener('pointerup', handleRelease);
            el.addEventListener('pointercancel', handleRelease);
            el.addEventListener('pointerleave', handleRelease);
            keyboardDiv.appendChild(el);
        });
    };

    buildKeyboard();
    updateOctaveControlsVisibility();

    if (octaveControls && octaveDisplay && octaveDownBtn && octaveUpBtn) {
        if (isMobileLayout) updateOctaveDisplay();

        octaveDownBtn.addEventListener('click', () => {
            if (!isMobileLayout) return;
            if (octaveOffset > octaveMin) {
                octaveOffset -= 1;
                refreshKeyNotes();
            }
        });

        octaveUpBtn.addEventListener('click', () => {
            if (!isMobileLayout) return;
            if (octaveOffset < octaveMax) {
                octaveOffset += 1;
                refreshKeyNotes();
            }
        });
    }

    // PC Keyboard mapping
    const keyMap = {};
    desktopKeys.forEach((key, index) => {
        keyMap[key] = index;
    });

    const activeNotes = {};
    mobileMediaQuery.addEventListener('change', (e) => {
        isMobileLayout = e.matches;
        refreshKeyNotes();
        buildKeyboard();
        updateOctaveControlsVisibility();
        if (isMobileLayout) updateOctaveDisplay();
    });

    window.addEventListener('keydown', (e) => {
        if (e.repeat) return;
        const noteOffset = keyMap[e.key];
        if (noteOffset !== undefined) {
            const note = getNoteValue(noteOffset);
            sendNoteOn(note);
            activeNotes[noteOffset] = note;
            const el = document.querySelector(`.key[data-note-offset="${noteOffset}"]`);
            if (el) el.classList.add('active');
        }
    });

    window.addEventListener('keyup', (e) => {
        const noteOffset = keyMap[e.key];
        const activeNote = activeNotes[noteOffset];
        if (noteOffset !== undefined && activeNote !== undefined) {
            sendNoteOff(activeNote);
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
