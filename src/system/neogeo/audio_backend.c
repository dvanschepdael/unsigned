#include "audio/audio_backend.h"

#include "system/neogeo/audio_backend_internal.h"
#include "system/neogeo/config.h"

#include <ngdevkit/backup-ram.h>
#include <ngdevkit/registers.h>

enum {
    NEO_GEO_SOUND_COMMAND_RESET_DRIVER = 3u,
    NEO_GEO_AUDIO_HANDOFF_MAGIC = 0x5541u, /* "UA": Unsigned Audio */
};

typedef struct UNeoGeoAudioHandoffState {
    u16 magic;
    u8 pending_coin_count;
    u8 pending_coin_count_inverse;
} UNeoGeoAudioHandoffState;

/*
 * REG_SOUND is a single-byte hardware latch, not a FIFO. nullsound already owns a 64-entry FIFO on
 * the Z80 side, but a command only reaches that FIFO after the Z80 NMI has sampled the latch.
 *
 * Normal game audio is therefore queued here and written only from neo_geo_audio_transport_tick(),
 * immediately after VBlank has completed. A regular COIN_SOUND can still use the immediate BIOS
 * path. Under MVS GAME START COMPULSION, however, USER 3 can switch FIX/M1 ownership through the
 * BIOS. nullsound command 1 deliberately silences the YM2610 during that switch, so the coin event
 * is persisted and replayed only after CRTFIX/M1 is restored and command 3 restarts the driver.
 */
static USoundCommand neo_geo_audio_queue[UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY];
static volatile bool neo_geo_audio_bios_sound_sent;
static USoundCommand neo_geo_audio_deferred_coin_command;
static u8 neo_geo_audio_deferred_coin_count;
static u8 neo_geo_audio_queue_head;
static u8 neo_geo_audio_queue_count;

/*
 * USER 3 runs ngdevkit's init_c_runtime(), which clears normal .bss. The cartridge backup block is
 * intentionally excluded from that clear, making it the supported place for the tiny transient
 * handoff token that must survive USER 2 -> USER 3. The token is consumed immediately in USER 3;
 * USER 2 also clears it defensively so it cannot become a saved-game event.
 */
static UNeoGeoAudioHandoffState _backup_ram neo_geo_audio_handoff_state;

static bool neo_geo_audio_handoff_state_is_valid(void) {
    return neo_geo_audio_handoff_state.magic == (u16)NEO_GEO_AUDIO_HANDOFF_MAGIC && neo_geo_audio_handoff_state.pending_coin_count_inverse == (u8)~neo_geo_audio_handoff_state.pending_coin_count;
}

static void neo_geo_audio_handoff_state_store(u8 pending_coin_count) {
    neo_geo_audio_handoff_state.magic = (u16)NEO_GEO_AUDIO_HANDOFF_MAGIC;
    neo_geo_audio_handoff_state.pending_coin_count = pending_coin_count;
    neo_geo_audio_handoff_state.pending_coin_count_inverse = (u8)~pending_coin_count;
}

static u8 neo_geo_audio_handoff_pending_coin_count(void) {
    if (!neo_geo_audio_handoff_state_is_valid()) {
        neo_geo_audio_handoff_state_store(0u);
        return 0u;
    }
    return neo_geo_audio_handoff_state.pending_coin_count;
}

/** The only function in the engine allowed to write the 68k -> Z80 sound latch. */
static void neo_geo_audio_transport_write(USoundCommand command) {
    *REG_SOUND = command;
}

static void neo_geo_audio_transport_clear_queue(void) {
    for (u8 i = 0u; i < neo_geo_audio_queue_count; ++i) {
        u16 index = (u16)neo_geo_audio_queue_head + i;
        if (index >= UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY) {
            index -= UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY;
        }
        neo_geo_audio_queue[(u8)index] = U_AUDIO_COMMAND_NONE;
    }

    neo_geo_audio_queue_head = 0u;
    neo_geo_audio_queue_count = 0u;
}

void neo_geo_audio_transport_init(void) {
    neo_geo_audio_transport_clear_queue();
    neo_geo_audio_bios_sound_sent = false;
    neo_geo_audio_deferred_coin_command = U_AUDIO_COMMAND_NONE;
    neo_geo_audio_deferred_coin_count = 0u;
}

bool unsigned_audio_backend_send(USoundCommand command) {
    if (command == U_AUDIO_COMMAND_NONE || neo_geo_audio_queue_count >= UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY) {
        return false;
    }

    u16 index = (u16)neo_geo_audio_queue_head + neo_geo_audio_queue_count;
    if (index >= UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY) {
        index -= UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY;
    }

    neo_geo_audio_queue[(u8)index] = command;
    ++neo_geo_audio_queue_count;
    return true;
}

void neo_geo_audio_transport_tick(void) {
    /*
     * COIN_SOUND runs inside BIOS SYSTEM_IO/VBlank. Giving it one complete transport slot keeps the
     * hardware latch stable until the Z80 has had ample opportunity to enter its NMI and copy the
     * command into nullsound's own FIFO. This is lifecycle serialization, not a timing delay.
     */
    if (neo_geo_audio_bios_sound_sent) {
        neo_geo_audio_bios_sound_sent = false;
        return;
    }

    /*
     * A deferred forced-start coin is intentionally not sent in neo_geo_init(): command 3 has no
     * ready acknowledgement and nullsound clears its FIFO while initializing. Reaching this first
     * post-VBlank slot gives command 3 a complete frame boundary before the coin NMI is generated.
     */
    if (neo_geo_audio_deferred_coin_count != 0u && neo_geo_audio_deferred_coin_command != U_AUDIO_COMMAND_NONE) {
        neo_geo_audio_transport_write(neo_geo_audio_deferred_coin_command);
        --neo_geo_audio_deferred_coin_count;
        if (neo_geo_audio_deferred_coin_count == 0u) {
            neo_geo_audio_deferred_coin_command = U_AUDIO_COMMAND_NONE;
        }
        return;
    }

    if (neo_geo_audio_queue_count == 0u) {
        return;
    }

    const USoundCommand command = neo_geo_audio_queue[neo_geo_audio_queue_head];
    neo_geo_audio_queue[neo_geo_audio_queue_head] = U_AUDIO_COMMAND_NONE;

    ++neo_geo_audio_queue_head;
    if (neo_geo_audio_queue_head >= UNSIGNED_NEO_GEO_AUDIO_TRANSPORT_QUEUE_CAPACITY) {
        neo_geo_audio_queue_head = 0u;
    }
    --neo_geo_audio_queue_count;

    neo_geo_audio_transport_write(command);
}

void neo_geo_audio_play_bios_sound(USoundCommand command) {
    if (command == U_AUDIO_COMMAND_NONE) {
        return;
    }

    /*
     * Never suppress a BIOS coin event: this callback may also run while no USER runtime frame is
     * active. The flag only reserves the next normal transport slot; it is not a coin-event latch.
     */
    neo_geo_audio_transport_write(command);
    neo_geo_audio_bios_sound_sent = true;
}

void neo_geo_audio_defer_coin_sound(void) {
    u8 pending = neo_geo_audio_handoff_pending_coin_count();
    if (pending != 0xffu) {
        ++pending;
    }
    neo_geo_audio_handoff_state_store(pending);
}

void neo_geo_audio_discard_deferred_coin_sounds(void) {
    neo_geo_audio_handoff_state_store(0u);
    neo_geo_audio_deferred_coin_command = U_AUDIO_COMMAND_NONE;
    neo_geo_audio_deferred_coin_count = 0u;
}

void neo_geo_audio_resume_deferred_coin_sounds(USoundCommand command) {
    const u8 pending = neo_geo_audio_handoff_pending_coin_count();

    /* Consume the persistent token before arming volatile playback so a later reset cannot replay it. */
    neo_geo_audio_handoff_state_store(0u);

    if (!unsigned_audio_command_is_game(command) || command >= 128u || pending == 0u) {
        neo_geo_audio_deferred_coin_command = U_AUDIO_COMMAND_NONE;
        neo_geo_audio_deferred_coin_count = 0u;
        return;
    }

    neo_geo_audio_deferred_coin_command = command;
    neo_geo_audio_deferred_coin_count = pending;
}

void neo_geo_audio_reset(void) {
    /* Command 3 is BIOS-reserved and has no acknowledgement. Never queue it behind game audio. */
    neo_geo_audio_transport_clear_queue();
    neo_geo_audio_bios_sound_sent = false;
    neo_geo_audio_deferred_coin_command = U_AUDIO_COMMAND_NONE;
    neo_geo_audio_deferred_coin_count = 0u;
    neo_geo_audio_transport_write((USoundCommand)NEO_GEO_SOUND_COMMAND_RESET_DRIVER);
}
