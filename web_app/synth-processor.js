class SynthProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.wasmInstance = null;
        this.memory = null;
        this.leftBufferPtr = null;
        this.rightBufferPtr = null;

        this.port.onmessage = (event) => {
            if (event.data.type === 'loadWasm') {
                WebAssembly.instantiate(event.data.wasmBytes, {
                    env: {
                        emscripten_notify_memory_growth: () => {},
                    },
                    wasi_snapshot_preview1: {
                        proc_exit: () => {},
                        fd_close: () => {},
                        fd_write: () => {},
                        fd_seek: () => {},
                    }
                }).then(result => {
                    this.wasmInstance = result.instance;
                    this.memory = this.wasmInstance.exports.memory;

                    // The _initialize equivalent in standalone WASM
                    if (this.wasmInstance.exports._initialize) {
                        this.wasmInstance.exports._initialize();
                    }

                    // Initialize the synth
                    this.wasmInstance.exports.initSynth();

                    // Get buffer pointers
                    this.leftBufferPtr = this.wasmInstance.exports.getLeftBuffer();
                    this.rightBufferPtr = this.wasmInstance.exports.getRightBuffer();

                    this.port.postMessage({ type: 'wasmLoaded' });
                }).catch(err => {
                    console.error('WASM instantiation failed', err);
                });
            } else if (event.data.type === 'noteOn') {
                if (this.wasmInstance) this.wasmInstance.exports.noteOn(event.data.note, event.data.velocity);
            } else if (event.data.type === 'noteOff') {
                if (this.wasmInstance) this.wasmInstance.exports.noteOff(event.data.note);
            } else if (event.data.type === 'controlChange') {
                if (this.wasmInstance) this.wasmInstance.exports.controlChange(event.data.cc, event.data.value);
            }
        };
    }

    static get parameterDescriptors() {
        return [
            {
                name: 'bitCrush',
                defaultValue: 0,
                minValue: 0,
                maxValue: 1,
            },
            {
                name: 'pwmSimulate',
                defaultValue: 0,
                minValue: 0,
                maxValue: 1,
            }
        ];
    }

    process(inputs, outputs, parameters) {
        if (!this.wasmInstance) return true;

        const output = outputs[0];
        const numSamples = output[0].length; // Usually 128

        // Tell WASM to generate audio
        this.wasmInstance.exports.processAudioBlock(numSamples);

        // Read the generated audio from WASM memory
        const leftBuffer = new Int16Array(this.memory.buffer, this.leftBufferPtr, numSamples);
        const rightBuffer = new Int16Array(this.memory.buffer, this.rightBufferPtr, numSamples);

        const bitCrush = parameters.bitCrush.length > 0 ? parameters.bitCrush[0] : 0;
        const pwmSimulate = parameters.pwmSimulate.length > 0 ? parameters.pwmSimulate[0] : 0;

        for (let i = 0; i < numSamples; i++) {
            let leftSample = leftBuffer[i] / 32768.0;
            let rightSample = rightBuffer[i] / 32768.0;

            // Hacker touch: bitcrush (reduce resolution from 16-bit to roughly 12-bit or lower)
            if (bitCrush > 0) {
                const levels = Math.max(2, Math.floor(4096 * (1.0 - bitCrush)));
                leftSample = Math.round(leftSample * levels) / levels;
                rightSample = Math.round(rightSample * levels) / levels;
            }

            // Hacker touch: PWM simulate (add some gritty high-frequency PWM ripple noise based on amplitude)
            if (pwmSimulate > 0) {
                const rippleL = (Math.random() * 2 - 1) * 0.05 * pwmSimulate * Math.abs(leftSample);
                const rippleR = (Math.random() * 2 - 1) * 0.05 * pwmSimulate * Math.abs(rightSample);
                leftSample += rippleL;
                rightSample += rippleR;
            }

            // Output to Web Audio channels
            if (output.length > 0) output[0][i] = leftSample;
            if (output.length > 1) output[1][i] = rightSample;
        }

        return true;
    }
}

registerProcessor('synth-processor', SynthProcessor);
