#!/usr/bin/env python3
# PRA32-U2 Load Estimate: runs the UF2 build of the sketch (its ELF) on two Unicorn instances, an
# instruction emulator (Cortex-M33, Thumb-2, and the FPU), as core 0 and core 1 sharing the flash
# and the SRAM, and estimates the cycles that loop() (core 0) takes and that core 1 is busy, by a
# first-order Cortex-M33 cost model (see cost() below).
#
#   python3 pra32-u2-load-estimate.py ELF [--program [CH:]N,...] [--cc [CH:]NUMBER=VALUE,...]
#                                         [--write [CH:]N,...] [--notes [CH:]NOTE,...] [--uart-midi HEX] [--usb-midi HEX]
#                                         [--loops N] [--skip N] [--profile]
#
# - The two cores run by turns, a slice of cycles at a time, so that their clocks stay together:
#   core 0 waits for core 1 (and core 1 for core 0) as on the device
# - loop() is measured from its start to its return; the output never blocks (the ring buffer is
#   always writable); the messages before the loops are given to the handlers, and while the loops
#   run, the bytes of --uart-midi arrive at 31250 bps and those of --usb-midi as fast as they are
#   read, through stand-ins for available() and read() of SerialUART and Adafruit_USBD_MIDI
# - Core 1 is busy but while it spins: the cycles since its last store (but to its stack) count if
#   they are few, and are a wait if they are many (see SPIN_CYCLES)
# - Left out: the waits of the flash (XIP), the contention of the cores on the bus, the interrupts
#   (e.g. USB), and the I2C transfers of the panel's display; so the estimate is a little low on
#   these, and may be a little high on the loads (2 cycles each)
#
# Needs: pip install unicorn capstone pyelftools
import sys, re, struct, argparse, bisect, ctypes, math
from elftools.elf.elffile import ELFFile
from unicorn import *
from unicorn.arm_const import *
from capstone import *
from capstone.arm import *

ap = argparse.ArgumentParser(description='Estimate the load of the PRA32-U2 sketch per loop (64 frames)')
ap.add_argument('elf', help='the sketch ELF, e.g. build/u2/Digital-Synth-PRA32-U2.ino.elf')
ap.add_argument('--program', default='', help='Program Changes before the loops: [CH:]N,...')
ap.add_argument('--cc', default='', help='Control Changes after them: [CH:]NUMBER=VALUE,...')
ap.add_argument('--write', default='', help='Write Program after them, of the parameters to the user program: [CH:]N,... (not into the flash)')
ap.add_argument('--notes', default='60,64,67,71', help='Note Ons (velocity 100) after them: [CH:]NOTE,...')
ap.add_argument('--uart-midi', default='', help='bytes (hex) that arrive by UART MIDI over and over, at 31250 bps, e.g. "C0 00 C0 0A"')
ap.add_argument('--usb-midi', default='', help='bytes (hex) that arrive by USB MIDI over and over, as fast as they are read')
ap.add_argument('--adc', type=float, default=0, help='loops in which the knobs (ADC0-2) of the panel (PRA32-U2/P) are turned all the way and back, over and over (0: not turned)')
ap.add_argument('--keys', default='', help='pins of the keys of the panel (PRA32-U2/P) pressed for 16 loops and released for 32, over and over (as fast as the panel takes them), e.g. 19 (SW4: Prog +)')
ap.add_argument('--keys-held', default='', help='pins of the keys of the panel held all the time, e.g. 21 (SW5: Shift)')
ap.add_argument('--loops', type=int, default=24, help='loops to run in all')
ap.add_argument('--skip', type=int, default=8, help='loops left out of the statistics (warm-up)')
ap.add_argument('--mhz', type=float, default=153.6, help='system clock, the sketch\'s 153.6 MHz by default')
ap.add_argument('--slice', type=int, default=200, help='cycles a core runs at a time')
ap.add_argument('--profile', action='store_true', help='print the functions that take the most cycles')
ap.add_argument('--modules', action='store_true', help='print the load of each module (class) of the synth on each core, from the debug info')
ap.add_argument('--brief', action='store_true', help='print a line: core 0 avg%%, max%%, core 1 avg%%, max%%')
args = ap.parse_args()

elf = ELFFile(open(args.elf, 'rb'))
names = {}
funcs = []
for s in elf.get_section_by_name('.symtab').iter_symbols():
    if s['st_value'] == 0:
        continue
    names.setdefault(s.name, s['st_value'])
    if s['st_info']['type'] == 'STT_FUNC':
        funcs.append((s['st_value'] & ~1, s['st_size'], s.name))
funcs.sort()
func_addrs = [a for a, _, _ in funcs]
func_names = {n: (a, size) for a, size, n in funcs}
def sym_of(addr):
    i = bisect.bisect_right(func_addrs, addr) - 1
    if i >= 0:
        a, size, n = funcs[i]
        if addr < a + max(size, 2):
            return n
    return '?'
def func(sub):
    for a, size, n in funcs:
        if sub in n and 'veneer' not in n:
            return a
    raise KeyError(sub)

# The flash (XIP), the SRAM, and the peripherals, shared by the cores; SIO (CPUID, the spinlocks,
# and the interpolators) and PPB, one each.
FLASH, SRAM = 0x10000000, 0x20000000
STACK = [0x20082000, 0x20081000]  # core 0 in SCRATCH_Y, core 1 in SCRATCH_X
STACK_1 = (0x20080000, 0x20081000)
shared = [(FLASH, 0x01000000), (SRAM, 0x00083000), (0x40000000, 0x00200000), (0x50000000, 0x00200000)]
buffers = [ctypes.create_string_buffer(size) for _, size in shared]
cores = []
for c in range(2):
    uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    for (base, size), buf in zip(shared, buffers):
        uc.mem_map_ptr(base, size, UC_PROT_ALL, ctypes.addressof(buf))
    uc.mem_map(0xd0000000, 0x00020000)
    uc.mem_map(0xe0000000, 0x00100000)
    uc.mem_write(0xd0000000, struct.pack('<I', c))          # CPUID
    uc.mem_write(0xd0000100, b'\x01\0\0\0' * 32)             # the spinlocks read as acquired
    uc.mem_write(0xe000ed88, struct.pack('<I', 0xf << 20))  # CPACR: the FPU on
    cores.append(uc)
for seg in elf.iter_segments():
    if seg['p_type'] == 'PT_LOAD':
        for addr in {seg['p_paddr'], seg['p_vaddr']}:
            try:
                cores[0].mem_write(addr, seg.data())
            except UcError:
                pass
RETURN = [0x10fffff0, 0x10fffff8]  # where a call returns to, "b ." for each core
for r in RETURN:
    cores[0].mem_write(r, b'\xfe\xe7')

# INTERP0 as the synth sets it up (setup_interp()): lane 0 blend, lane 1 signed, so that
# PEEK_LANE1 = BASE0 + (BASE1 - BASE0) * ACCUM1[7:0] / 256 (the lerp of the Osc's wave tables)
def s32(x): return x - (1 << 32) if x & 0x80000000 else x
def interp_read(uc, access, address, size, value, _):
    if address == 0xd00000a4:
        accum1, base0, base1 = struct.unpack('<III', bytes(uc.mem_read(0xd0000084, 12)))
        result = s32(base0) + ((s32(base1) - s32(base0)) * (accum1 & 0xff) >> 8)
        uc.mem_write(0xd00000a4, struct.pack('<I', result & 0xffffffff))
for uc in cores:
    uc.hook_add(UC_HOOK_MEM_READ, interp_read, begin=0xd0000080, end=0xd00000bf)

# The ADC (RESULT, 12 bits) of the panel's knobs: turned all the way and back in --adc loops, over
# and over (as a triangle wave of the time); the panel reads it once per sample
ADC_RESULT = 0x400a0004
def adc_read(uc, access, address, size, value, _):
    if args.adc and measuring_['on']:
        t = (max(clock) / (args.mhz * 1e6 * 64 / 48000) / args.adc) % 1
        uc.mem_write(ADC_RESULT, struct.pack('<I', int(4095 * (1 - abs(2 * t - 1)))))
for uc in cores:
    uc.hook_add(UC_HOOK_MEM_READ, adc_read, begin=ADC_RESULT, end=ADC_RESULT + 3)

# Cortex-M33 cost model, cycles per instruction, first-order (as in SPMS-1's sim_load): 1 for most,
# 2 for a single load, 1 more per register for a multiple load or store, 6 for an integer divide,
# 14 for a floating-point divide or square root, and 2 more after a taken branch.
md = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_MCLASS | CS_MODE_V8)
md.detail = True
MULTIPLE = ('ldm', 'ldmia', 'ldmdb', 'stm', 'stmia', 'stmdb', 'vldm', 'vstm', 'vldmia', 'vstmia', 'vldmdb', 'vstmdb')
def cost(insn):
    m = insn.mnemonic.split('.')[0]
    if m in ('vdiv', 'vsqrt'):
        return 14
    if m in ('sdiv', 'udiv'):
        return 6
    if m in MULTIPLE or m in ('push', 'pop', 'vpush', 'vpop'):
        regs = sum(1 for op in insn.operands if op.type == ARM_OP_REG) - (1 if m in MULTIPLE else 0)
        return 1 + max(regs, 1)
    if m.startswith('ld') or m.startswith('vldr'):
        return 2
    return 1
decoded = {}
def decode(uc, addr, size):
    d = decoded.get(addr)
    if d is None:
        insn = next(md.disasm(bytes(uc.mem_read(addr, size)), addr), None)
        # An IT block is not broken by the end of a slice, as a resumed run would lose its state
        it = len(insn.mnemonic) - 1 if insn and re.fullmatch(r'it[te]{0,3}', insn.mnemonic) else 0
        d = (cost(insn) if insn else 1, it)
        decoded[addr] = d
    return d

def ret(uc, value=None):
    if value is not None:
        uc.reg_write(UC_ARM_REG_R0, value & 0xffffffff)
    uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR) | 1)

# The hardware is stood in for by functions that return 0 at once: the clocks, the I2S/PWM output,
# USB and UART, the GPIOs, the ADCs, I2C, the timers, the locks, and the output's wait; never the
# synth's own functions, the MIDI Library's (on top of the stand-ins for the MIDI input below), the
# constructors, nor the static initializers.
STUBBED = ['TinyUSB', 'USBD', 'USBDevice', 'tud_', 'SerialUART', 'SerialPIO', 'SerialUSB', 'Adafruit',
           'pinMode', 'digitalWrite', 'delay', 'gpio_', 'set_sys_clock', 'clock_', 'pio_', 'dma_', 'irq_',
           'I2SOutput', 'PWMAudioOutput', 'mutex', 'micros', 'millis', 'Wire', 'analogRead', 'adc_',
           'busy_wait', 'sleep_', 'multicore', 'rom_', 'unique_id', 'flash_', 'hw_', 'spin_lock', 'alarm',
           'xosc', 'pll_', 'watchdog', 'timer_', 'time_us']
stubs = {}
for a, size, n in funcs:
    if re.search(r'[0-9A-Za-z_]C[12]E', n) or n.startswith('_GLOBAL__sub_I') or 'MidiInterface' in n or 'serialMIDI' in n:
        continue
    if (any(p in n for p in STUBBED) and 'PRA32_U2' not in n) or 'wait_for_writable' in n:
        stubs[a] = lambda uc: ret(uc, 0)
# The keys of the panel (PRA32-U2/P) are released (high, with the pull-ups), but those of --keys
# while they are pressed (low): for 16 loops of each 48, as the panel takes a key pressed for 15 loops
# and released for 30; and those of --keys-held all the time
KEY_PINS = {int(k) for k in filter(None, args.keys.split(','))}
HELD_KEY_PINS = {int(k) for k in filter(None, args.keys_held.split(','))}
def digital_read(uc):
    pin = regs(uc)[0]
    pressed = measuring_['on'] and ((pin in KEY_PINS and len(loops) % 48 < 16) or pin in HELD_KEY_PINS)
    ret(uc, 0 if pressed else 1)
for n in ('digitalRead', '__digitalRead'):
    if n in func_names:
        stubs[func_names[n][0]] = digital_read

# The double-precision helpers of the pico SDK, which run on the RP2350's DCP (not in Unicorn),
# computed here instead; the synth uses them only while it sets up.
def f64(lo, hi): return struct.unpack('<d', struct.pack('<II', lo & 0xffffffff, hi & 0xffffffff))[0]
def ret64(uc, v):
    lo, hi = struct.unpack('<II', struct.pack('<d', v))
    uc.reg_write(UC_ARM_REG_R1, hi)
    ret(uc, lo)
def regs(uc): return [uc.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)]
def sint(x, bits): return x - (1 << bits) if x >> (bits - 1) else x
def dd(uc):
    a, b, c, d = regs(uc)
    return f64(a, b), f64(c, d)
def safe(fn, *a):
    try:
        return fn(*a)
    except (ValueError, OverflowError, ZeroDivisionError):
        return float('nan')
def sincos(uc):
    x = dd(uc)[0]
    uc.mem_write(regs(uc)[2], struct.pack('<d', math.sin(x)))
    uc.mem_write(regs(uc)[3], struct.pack('<d', math.cos(x)))
    ret(uc)
def d2lz(uc):
    v = int(dd(uc)[0]) & 0xffffffffffffffff
    uc.reg_write(UC_ARM_REG_R1, v >> 32)
    ret(uc, v & 0xffffffff)
def d2f(uc): ret(uc, struct.unpack('<I', struct.pack('<f', safe(lambda x: struct.unpack('<f', struct.pack('<f', x))[0], dd(uc)[0])))[0])
helpers = {
    '__wrap___aeabi_dadd': lambda uc: ret64(uc, sum(dd(uc))),
    '__wrap___aeabi_dsub': lambda uc: ret64(uc, dd(uc)[0] - dd(uc)[1]),
    '__wrap___aeabi_dmul': lambda uc: ret64(uc, dd(uc)[0] * dd(uc)[1]),
    '__wrap___aeabi_ddiv': lambda uc: ret64(uc, safe(lambda a, b: a / b, *dd(uc))),
    '__wrap___aeabi_d2f': d2f,
    '__wrap___aeabi_i2d': lambda uc: ret64(uc, float(sint(regs(uc)[0], 32))),
    '__wrap___aeabi_ui2d': lambda uc: ret64(uc, float(regs(uc)[0])),
    '__wrap___aeabi_l2d': lambda uc: ret64(uc, float(sint(regs(uc)[0] | regs(uc)[1] << 32, 64))),
    '__wrap___aeabi_ul2d': lambda uc: ret64(uc, float(regs(uc)[0] | regs(uc)[1] << 32)),
    '__wrap___aeabi_d2iz': lambda uc: ret(uc, int(dd(uc)[0])),
    '__wrap___aeabi_d2uiz': lambda uc: ret(uc, int(dd(uc)[0])),
    '__wrap___aeabi_d2lz': d2lz,
    '__wrap___aeabi_dcmpeq': lambda uc: ret(uc, int(dd(uc)[0] == dd(uc)[1])),
    '__wrap___aeabi_dcmplt': lambda uc: ret(uc, int(dd(uc)[0] < dd(uc)[1])),
    '__wrap___aeabi_dcmple': lambda uc: ret(uc, int(dd(uc)[0] <= dd(uc)[1])),
    '__wrap___aeabi_dcmpgt': lambda uc: ret(uc, int(dd(uc)[0] > dd(uc)[1])),
    '__wrap___aeabi_dcmpge': lambda uc: ret(uc, int(dd(uc)[0] >= dd(uc)[1])),
    '__wrap___aeabi_dcmpun': lambda uc: ret(uc, int(math.isnan(dd(uc)[0]) or math.isnan(dd(uc)[1]))),
    '__wrap_sin': lambda uc: ret64(uc, math.sin(dd(uc)[0])),
    '__wrap_cos': lambda uc: ret64(uc, math.cos(dd(uc)[0])),
    '__wrap_sincos': sincos,
    '__wrap_tan': lambda uc: ret64(uc, math.tan(dd(uc)[0])),
    '__wrap_atan': lambda uc: ret64(uc, math.atan(dd(uc)[0])),
    '__wrap_pow': lambda uc: ret64(uc, safe(math.pow, *dd(uc))),
    '__wrap_exp': lambda uc: ret64(uc, safe(math.exp, dd(uc)[0])),
    '__wrap_log': lambda uc: ret64(uc, safe(math.log, dd(uc)[0])),
    '__wrap_log10': lambda uc: ret64(uc, safe(math.log10, dd(uc)[0])),
    '__wrap_sqrt': lambda uc: ret64(uc, safe(math.sqrt, dd(uc)[0])),
    '__wrap_floor': lambda uc: ret64(uc, math.floor(dd(uc)[0])),
}
measuring = False
helpers_in_loops = set()
for n, fn in helpers.items():
    if n in func_names:
        def run(uc, n=n, fn=fn):
            if measuring:
                helpers_in_loops.add(n)
            fn(uc)
        stubs[func_names[n][0]] = run

# The MIDI input while the loops run: available() and read() of the UART (Serial2) and of USB MIDI,
# each counted at MIDI_CALL_CYCLES, a rough figure for the real ones (a FIFO and a lock)
MIDI_CALL_CYCLES = 20
UART_BYTE_CYCLES = args.mhz * 1e6 * 10 / 31250  # 10 bits a byte
UART_FIFO = 256
midi_in = {
    'uart': {'bytes': bytes.fromhex(args.uart_midi), 'read': 0},
    'usb': {'bytes': bytes.fromhex(args.usb_midi), 'read': 0},
}
measuring_ = {'on': False}
def midi_waiting(port):
    m = midi_in[port]
    if not m['bytes'] or not measuring_['on']:
        return 0
    if port == 'usb':
        return 64
    arrived = int(clock[0] / UART_BYTE_CYCLES)
    m['read'] = max(m['read'], arrived - UART_FIFO)  # an overrun drops the oldest
    return arrived - m['read']
def midi_available(port):
    def fn(uc):
        clock[0] += MIDI_CALL_CYCLES
        ret(uc, midi_waiting(port))
    return fn
def midi_read(port):
    def fn(uc):
        clock[0] += MIDI_CALL_CYCLES
        if midi_waiting(port) == 0:
            ret(uc, -1)
            return
        m = midi_in[port]
        b = m['bytes'][m['read'] % len(m['bytes'])]
        m['read'] += 1
        ret(uc, b)
    return fn
for n, fn in [('_ZN10SerialUART9availableEv', midi_available('uart')), ('_ZN10SerialUART4readEv', midi_read('uart')),
              ('_ZN18Adafruit_USBD_MIDI9availableEv', midi_available('usb')), ('_ZN18Adafruit_USBD_MIDI4readEv', midi_read('usb'))]:
    if n in func_names:
        stubs[func_names[n][0]] = fn

SPIN_CYCLES = 400
clock = [0, 0]
target = [0, 0]
it_left = [0, 0]
busy_1 = {'total': 0, 'pending': 0}
profile = [{}, {}]
pending_profile = {}

def make_hook(c):
    def hook(uc, addr, size, _):
        fn = stubs.get(addr)
        if fn:
            fn(uc)
            return
        cycles, it = decode(uc, addr, size)
        clock[c] += cycles
        if c == 1:
            busy_1['pending'] += cycles
        if (args.profile or args.modules) and measuring:
            p = pending_profile if c == 1 else profile[0]
            k = addr if args.modules else sym_of(addr)
            p[k] = p.get(k, 0) + cycles
        if it_left[c]:
            it_left[c] -= 1
        if it:
            it_left[c] = it
        if clock[c] >= target[c] and it_left[c] == 0:
            uc.emu_stop()
    return hook
def store_1(uc, access, address, size, value, _):
    if STACK_1[0] <= address < STACK_1[1]:
        return
    if busy_1['pending'] <= SPIN_CYCLES:
        busy_1['total'] += busy_1['pending']
        for k, v in pending_profile.items():
            profile[1][k] = profile[1].get(k, 0) + v
    busy_1['pending'] = 0
    pending_profile.clear()
def unmapped(uc, access, address, size, value, _):
    pc = uc.reg_read(UC_ARM_REG_PC)
    print('unmapped access 0x%08x at 0x%08x (%s)' % (address, pc, sym_of(pc)), file=sys.stderr)
    return False
for c, uc in enumerate(cores):
    uc.hook_add(UC_HOOK_CODE, make_hook(c))
    uc.hook_add(UC_HOOK_MEM_UNMAPPED, unmapped)
cores[1].hook_add(UC_HOOK_MEM_WRITE, store_1)

def call(c, addr, *a):
    """Runs a function on a core alone, to its return."""
    uc = cores[c]
    for r, v in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), a):
        uc.reg_write(r, v)
    uc.reg_write(UC_ARM_REG_SP, STACK[c])
    uc.reg_write(UC_ARM_REG_LR, RETURN[c] | 1)
    target[c] = 1 << 62
    try:
        uc.emu_start(addr | 1, RETURN[c], count=100_000_000)
    except UcError as e:
        pc = uc.reg_read(UC_ARM_REG_PC)
        sys.exit('core %d, in %s: %s at 0x%08x (%s)' % (c, sym_of(addr), e, pc, sym_of(pc)))

# Start-up: the static initializers and setup() on core 0, then setup1() on core 1.
for p in range(names['__init_array_start'], names['__init_array_end'], 4):
    f = struct.unpack('<I', bytes(cores[0].mem_read(p, 4)))[0] & ~1
    if f:
        call(0, f)
call(0, func_names['setup'][0])
call(1, func_names['setup1'][0])

# The MIDI messages, as the sketch's handlers take them (MIDI Channel 1 is the Basic Channel)
def items(text):
    for item in filter(None, text.split(',')):
        ch, _, rest = item.rpartition(':')
        yield int(ch or 1), rest
for ch, n in items(args.program):
    call(0, func('handleProgramChange'), ch, int(n))
for ch, kv in items(args.cc):
    k, v = kv.split('=')
    call(0, func('handleControlChange'), ch, int(k), int(v))
# Write Program (CC #87 the program number, and CC #106 from Off to On), as PRA32-U2 Editor writes:
# the user program changes in the RAM, and the commit into the flash that it asks for is dropped
for ch, n in items(args.write):
    for k, v in ((87, int(n)), (106, 0), (106, 127)):
        call(0, func('handleControlChange'), ch, k, v)
if args.write:
    cores[0].mem_write(names['_ZL25g_eeprom_commit_requested'], b'\0')
for ch, n in items(args.notes):
    call(0, func('handleNoteOn'), ch, int(n), 100)
# The panel takes the knobs and the keys after 75 loops from the start: skipped here, for them
INITIALIZE_COUNTER = '_ZZ36PRA32_U2_ControlPanel_update_controlvE20s_initialize_counter'
if (args.adc or args.keys or args.keys_held) and INITIALIZE_COUNTER in names:
    cores[0].mem_write(names[INITIALIZE_COUNTER], struct.pack('<I', 75))

# Both cores together: core 0 runs loop() over and over, and core 1 loop1(), which never returns;
# the core whose clock is behind runs next.
loop, loop_size = func_names['loop']
pc = [loop | 1, func_names['loop1'][0] | 1]
for c, uc in enumerate(cores):
    uc.reg_write(UC_ARM_REG_SP, STACK[c])
    uc.reg_write(UC_ARM_REG_LR, RETURN[c] | 1)
clock = [0, 0]
measuring = True
measuring_['on'] = True
loops = []
loop_start, busy_start = 0, 0
while len(loops) < args.loops:
    c = 0 if clock[0] <= clock[1] else 1
    uc = cores[c]
    target[c] = clock[c] + args.slice
    try:
        uc.emu_start(pc[c], 0xffffffff, count=10_000_000)
    except UcError as e:
        p = uc.reg_read(UC_ARM_REG_PC)
        sys.exit('core %d: %s at 0x%08x (%s)' % (c, e, p, sym_of(p)))
    p = uc.reg_read(UC_ARM_REG_PC) & ~1
    if c == 0 and p == RETURN[0]:
        loops.append((clock[0] - loop_start, busy_1['total'] - busy_start))
        loop_start, busy_start = clock[0], busy_1['total']
        uc.reg_write(UC_ARM_REG_SP, STACK[0])
        uc.reg_write(UC_ARM_REG_LR, RETURN[0] | 1)
        p = loop
    pc[c] = p | 1

loops = loops[args.skip:]
budget_us = 64 / 48000 * 1e6
def percent(cycles): return 100 * cycles / args.mhz / budget_us
c0 = [l[0] for l in loops]
c1 = [l[1] for l in loops]
c0_avg, c1_avg = sum(c0) / len(c0), sum(c1) / len(c1)
if args.brief:
    print('%.1f %.1f %.1f %.1f' % (percent(c0_avg), percent(max(c0)), percent(c1_avg), percent(max(c1))))
else:
    print('loops measured: %d (after %d warm-up), of %.0f us (64 frames at 48 kHz) at %.1f MHz' % (len(loops), args.skip, budget_us, args.mhz))
    print('core 0, loop(): avg %.0f, max %d cycles (%.1f%% / %.1f%%)' % (c0_avg, max(c0), percent(c0_avg), percent(max(c0))))
    print('core 1, busy:   avg %.0f, max %d cycles (%.1f%% / %.1f%%)' % (c1_avg, max(c1), percent(c1_avg), percent(max(c1))))
    for port in ('uart', 'usb'):
        if midi_in[port]['bytes']:
            print('%s MIDI: %d bytes read' % (port.upper(), midi_in[port]['read']))
if helpers_in_loops:
    print('double-precision helpers called in the loops: %s' % ', '.join(sorted(helpers_in_loops)), file=sys.stderr)
if args.profile:
    for c in range(2):
        total = sum(profile[c].values()) or 1
        print('core %d:' % c)
        for k, v in sorted(profile[c].items(), key=lambda kv: -kv[1])[:12]:
            print('  %5.1f%%  %s' % (100 * v / total, k))

# --modules: the cycles of each core by the module (class) of the synth that its code is of, from the
# debug info: the outermost function of a module inlined there (or the function itself, if not
# inlined), else the synth itself (its voices, parameters, and mixing) or the sketch (MIDI, the panel)
if args.modules:
    MODULES = ['Osc', 'Filter', 'Amp', 'EG', 'LFO', 'NoiseGen', 'ChorusFx', 'DelayFx', 'Panner',
               'OutputCompressor', 'OutputLimiter', 'OutputFader']
    def class_of(name):
        for m in MODULES:
            if ('%dPRA32_U2_%s' % (len('PRA32_U2_' + m), m)) in name:
                return m
        return None
    def die_class(die):
        for _ in range(6):
            for attr in ('DW_AT_linkage_name', 'DW_AT_MIPS_linkage_name'):
                if attr in die.attributes:
                    return class_of(die.attributes[attr].value.decode())
            parent = die.get_parent()
            if parent is not None and parent.tag in ('DW_TAG_class_type', 'DW_TAG_structure_type') and 'DW_AT_name' in parent.attributes:
                n = parent.attributes['DW_AT_name'].value.decode()
                return n[len('PRA32_U2_'):] if n[len('PRA32_U2_'):] in MODULES else None
            o = die.attributes.get('DW_AT_abstract_origin') or die.attributes.get('DW_AT_specification')
            if not o:
                return None
            die = die.get_DIE_from_attribute(o.name)
        return None
    executed = sorted(set(profile[0]) | set(profile[1]))
    owner = {}
    dw = elf.get_dwarf_info()
    rl = dw.range_lists()
    def die_ranges(die, cu):
        a = die.attributes
        if 'DW_AT_low_pc' in a and 'DW_AT_high_pc' in a:
            lo = a['DW_AT_low_pc'].value
            hi = a['DW_AT_high_pc'].value
            return [(lo, hi if a['DW_AT_high_pc'].form == 'DW_FORM_addr' else lo + hi)]
        out = []
        if 'DW_AT_ranges' in a:
            base = cu.get_top_DIE().attributes.get('DW_AT_low_pc')
            base = base.value if base else 0
            for e in rl.get_range_list_at_offset(a['DW_AT_ranges'].value, cu=cu):
                if hasattr(e, 'base_address'):
                    base = e.base_address
                elif hasattr(e, 'begin_offset'):
                    out.append((e.begin_offset + base, e.end_offset + base))
        return out
    for cu in dw.iter_CUs():
        def walk(die):  # the outermost function of a module claims its addresses first
            for ch in die.iter_children():
                if ch.tag in ('DW_TAG_subprogram', 'DW_TAG_inlined_subroutine'):
                    cls = die_class(ch)
                    if cls:
                        for lo, hi in die_ranges(ch, cu):
                            for i in range(bisect.bisect_left(executed, lo & ~1), bisect.bisect_left(executed, hi)):
                                owner.setdefault(executed[i], cls)
                        continue
                walk(ch)
        walk(cu.get_top_DIE())
    print('modules, of %.0f us (avg per loop): core 0 / core 1' % budget_us)
    rows = {}
    for c in (0, 1):
        for a, v in profile[c].items():
            k = owner.get(a)
            if k is None:
                n = sym_of(a)
                k = 'Synth' if 'PRA32_U2_Synth' in n else 'Sketch' if n in ('loop', 'loop1') else 'Other'
            rows.setdefault(k, [0, 0])[c] += v
    for k, (v0, v1) in sorted(rows.items(), key=lambda kv: -sum(kv[1])):
        print('  %-16s %5.1f%% / %5.1f%%' % (k, percent(v0 / args.loops), percent(v1 / args.loops)))
