#pragma once

// (Included by "pra32-u2-synth.h", after s_program_table_parameters and s_program_table_panel_parameters)

// The user programs (#0-15) and the panel parameters in the flash, without a copy of them in the RAM (the EEPROM library
// of Arduino-Pico, used before, kept the 3 KB used of its 4 KB sector in the heap)
//
// - The same sector (the last 4 KB of the flash, _EEPROM_start) and the same layout as with the EEPROM library, so that the
//   programs written before are read as they were:
//   - 0-127: the panel parameters ('P', 0, and the values at their control numbers)
//   - 1024 + 128 * # (# = 0-15): the user program # ('U', #, and the values at their control numbers)
// - Read directly from the flash (XIP) by initialize() of the synths, at power-on
// - Written by PRA32_U2_ProgramFlash_commit() (called by the Output Fader, after the output is faded out): it erases the
//   sector, and programs its first 12 pages of 256 bytes, each made on the stack from the tables in the RAM: the program
//   table (g_program_table, shared by the synths, so that a user program written by one of them is the same in all, as in
//   the flash) for the user programs written (with 'U'), and the panel parameters of the synth that wrote them last (with
//   'P'); the other bytes are 0xFF (as erased), as the parameters do not use them (they are not kept)

#if defined(ARDUINO_ARCH_RP2040)
// The program table, shared by the synths (PRA32-U2/M: the 4 synths), as the flash is
static uint8_t g_program_table[128][PROGRAM_NUMBER_MAX + 1];
#endif  // defined(ARDUINO_ARCH_RP2040)

#if defined(ARDUINO_ARCH_RP2040) && defined(PRA32_U2_USE_EMULATED_EEPROM)
#include <hardware/flash.h>

extern "C" uint8_t _EEPROM_start;

static const uint32_t PROGRAM_FLASH_PANEL_ADDRESS   = 0;
static const uint32_t PROGRAM_FLASH_PROGRAM_ADDRESS = 1024;
static const uint32_t PROGRAM_FLASH_PROGRAM_SIZE    = 128;
static const uint32_t PROGRAM_FLASH_USED_SIZE       = PROGRAM_FLASH_PROGRAM_ADDRESS + (PROGRAM_FLASH_PROGRAM_SIZE * (USER_PROGRAM_NUMBER_MAX + 1));

static_assert(PROGRAM_FLASH_USED_SIZE == 3072, "The layout must be the same as before");
static_assert(PROGRAM_FLASH_USED_SIZE <= FLASH_SECTOR_SIZE, "The sector must have the programs");
static_assert((PROGRAM_FLASH_USED_SIZE % FLASH_PAGE_SIZE) == 0, "Programmed in pages");

static uint16_t       g_program_flash_user_programs = 0;        // The user programs in the flash (with 'U'), a bit each
static boolean        g_program_flash_panel         = false;    // The panel parameters in the flash (with 'P')
static const uint8_t* g_program_flash_panel_values  = nullptr;  // The panel parameters (128 + 128) of the synth that wrote them last

static INLINE uint8_t PRA32_U2_ProgramFlash_read(uint32_t address) {
  return (&_EEPROM_start)[address];
}

// Reads the flash at power-on (by initialize() of each synth: the shared program table again, with the same values); in the
// flash (not while playing), and not inlined, so that the loops are not put in each synth (in the RAM)
static __attribute__((noinline)) void PRA32_U2_ProgramFlash_load(uint8_t (&program_table)[128][PROGRAM_NUMBER_MAX + 1], uint8_t panel_values[128 + 128],
                                                                 boolean main_synth) {
  for (uint32_t program_number = 0; program_number <= USER_PROGRAM_NUMBER_MAX; ++program_number) {
    uint32_t program_address = PROGRAM_FLASH_PROGRAM_ADDRESS + (program_number * PROGRAM_FLASH_PROGRAM_SIZE);
    if ((PRA32_U2_ProgramFlash_read(program_address) == 'U') && (PRA32_U2_ProgramFlash_read(program_address + 1) == program_number)) {
      g_program_flash_user_programs |= (1u << program_number);
      for (uint32_t i = 0; i < sizeof(s_program_table_parameters) / sizeof(s_program_table_parameters[0]); ++i) {
        uint32_t control_number = s_program_table_parameters[i];
        uint8_t value_read = PRA32_U2_ProgramFlash_read(program_address + control_number);
        if (value_read < 128) {
          program_table[control_number][program_number] = value_read;
        }
      }
    }
  }

  // The panel parameters also without the panel (not used then), so that they are written back as they were
  // (e.g. by PRA32-U2/M, of those written by PRA32-U2/M/P)
  if ((PRA32_U2_ProgramFlash_read(PROGRAM_FLASH_PANEL_ADDRESS) == 'P') && (PRA32_U2_ProgramFlash_read(PROGRAM_FLASH_PANEL_ADDRESS + 1) == 0)) {
    g_program_flash_panel = true;
    if (main_synth) {
      g_program_flash_panel_values = panel_values;  // The Main Synth's, until a synth writes them
    }
    for (uint32_t i = 0; i < sizeof(s_program_table_panel_parameters) / sizeof(s_program_table_panel_parameters[0]); ++i) {
      uint32_t control_number = s_program_table_panel_parameters[i];
      uint8_t value_read = PRA32_U2_ProgramFlash_read(PROGRAM_FLASH_PANEL_ADDRESS + control_number);
      if (value_read < 128) {
        panel_values[control_number] = value_read;
      }
    }
  }
}

// A byte of the page (the address in the page or not), in the order that the EEPROM library was written in
static INLINE void PRA32_U2_ProgramFlash_put(uint8_t page_data[FLASH_PAGE_SIZE], uint32_t page_address, uint32_t address, uint8_t value) {
  if ((address >= page_address) && (address < (page_address + FLASH_PAGE_SIZE))) {
    page_data[address - page_address] = value;
  }
}

// (The parameters: s_program_table_parameters and s_program_table_panel_parameters of "pra32-u2-synth.h")
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ProgramFlash_make_page)(uint8_t page_data[FLASH_PAGE_SIZE], uint32_t page_address) {
  std::memset(page_data, 0xFF, FLASH_PAGE_SIZE);

  if (g_program_flash_panel && (g_program_flash_panel_values != nullptr)) {  // (also without the panel, as read)
    for (uint32_t i = 0; i < sizeof(s_program_table_panel_parameters) / sizeof(s_program_table_panel_parameters[0]); ++i) {
      uint32_t control_number = s_program_table_panel_parameters[i];
      PRA32_U2_ProgramFlash_put(page_data, page_address, PROGRAM_FLASH_PANEL_ADDRESS + control_number, g_program_flash_panel_values[control_number]);
    }

    PRA32_U2_ProgramFlash_put(page_data, page_address, PROGRAM_FLASH_PANEL_ADDRESS,     'P');
    PRA32_U2_ProgramFlash_put(page_data, page_address, PROGRAM_FLASH_PANEL_ADDRESS + 1, 0);
  }

  for (uint32_t program_number = 0; program_number <= USER_PROGRAM_NUMBER_MAX; ++program_number) {
    if (g_program_flash_user_programs & (1u << program_number)) {
      uint32_t program_address = PROGRAM_FLASH_PROGRAM_ADDRESS + (program_number * PROGRAM_FLASH_PROGRAM_SIZE);
      for (uint32_t i = 0; i < sizeof(s_program_table_parameters) / sizeof(s_program_table_parameters[0]); ++i) {
        uint32_t control_number = s_program_table_parameters[i];
        PRA32_U2_ProgramFlash_put(page_data, page_address, program_address + control_number, g_program_table[control_number][program_number]);
      }

      PRA32_U2_ProgramFlash_put(page_data, page_address, program_address,     'U');
      PRA32_U2_ProgramFlash_put(page_data, page_address, program_address + 1, program_number);
    }
  }
}

// About 50-100 ms (mostly the erase of the sector), with the interrupts disabled and the other core idle, as the EEPROM
// library did; the pages are made from the tables in the RAM by the code in the RAM, between the programming of them
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ProgramFlash_commit)() {
  uint8_t  page_data[FLASH_PAGE_SIZE];
  uint32_t flash_offset = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&_EEPROM_start) - XIP_BASE);

  noInterrupts();
  rp2040.idleOtherCore();
  flash_range_erase(flash_offset, FLASH_SECTOR_SIZE);
  for (uint32_t page_address = 0; page_address < PROGRAM_FLASH_USED_SIZE; page_address += FLASH_PAGE_SIZE) {
    PRA32_U2_ProgramFlash_make_page(page_data, page_address);
    flash_range_program(flash_offset + page_address, page_data, FLASH_PAGE_SIZE);
  }
  rp2040.resumeOtherCore();
  interrupts();
}
#endif  // defined(ARDUINO_ARCH_RP2040) && defined(PRA32_U2_USE_EMULATED_EEPROM)
