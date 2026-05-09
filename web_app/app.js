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

function setupControls() {
    document.getElementById('cutoff').addEventListener('input', (e) => {
        sendCC(74, parseInt(e.target.value));
    });

    document.getElementById('resonance').addEventListener('input', (e) => {
        sendCC(71, parseInt(e.target.value));
    });

    document.getElementById('delayTime').addEventListener('input', (e) => {
        sendCC(90, parseInt(e.target.value));
    });

    document.getElementById('osc1shape').addEventListener('input', (e) => {
        sendCC(14, parseInt(e.target.value));
    });

    const bitCrushParam = synthNode.parameters.get('bitCrush');
    document.getElementById('bitCrush').addEventListener('input', (e) => {
        bitCrushParam.value = parseFloat(e.target.value);
    });

    const pwmSimulateParam = synthNode.parameters.get('pwmSimulate');
    document.getElementById('pwmSimulate').addEventListener('input', (e) => {
        pwmSimulateParam.value = parseFloat(e.target.value);
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

        el.addEventListener('mousedown', () => {
            sendNoteOn(baseNote + k.note);
            el.classList.add('active');
        });
        el.addEventListener('mouseup', () => {
            sendNoteOff(baseNote + k.note);
            el.classList.remove('active');
        });
        el.addEventListener('mouseleave', () => {
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
        if (data1 === 74) updateSlider('cutoff', data2);
        if (data1 === 71) updateSlider('resonance', data2);
        if (data1 === 90) updateSlider('delayTime', data2);
        if (data1 === 14) updateSlider('osc1shape', data2);
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
