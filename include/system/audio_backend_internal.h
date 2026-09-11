/**
 * @file audio_backend_internal.h
 * @brief Neo Geo lifecycle controls for the serialized 68k-to-Z80 sound transport.
 *
 * These entry points are runtime-only because they can issue reserved BIOS/nullsound commands and
 * participate in USER 2 -> USER 3 ROM handoff behavior. Normal gameplay uses `audio/audio.h`.
 */

#ifndef UNSIGNED_SYSTEM_AUDIO_BACKEND_INTERNAL_H
#define UNSIGNED_SYSTEM_AUDIO_BACKEND_INTERNAL_H

#include "audio/audio_types.h"

/** Reset the 68k-side transport state for one BIOS USER runtime entry. */
void neo_geo_audio_transport_init(void);

/** Dispatch at most one queued gameplay command at the safe post-VBlank boundary. */
void neo_geo_audio_transport_tick(void);

/** Send a BIOS sound immediately when no cartridge/board-ROM handoff is in progress. */
void neo_geo_audio_play_bios_sound(USoundCommand command);

/** Persist one forced-start coin event across ngdevkit's USER 2 -> USER 3 C runtime reset. */
void neo_geo_audio_defer_coin_sound(void);

/** Drop deferred coin events when starting a fresh USER 2 attract lifecycle. */
void neo_geo_audio_discard_deferred_coin_sounds(void);

/** Replay deferred coin events after USER 3 has remapped M1 and reset the Z80 sound driver. */
void neo_geo_audio_resume_deferred_coin_sounds(USoundCommand command);

/** Neo Geo runtime-only Z80 driver reset (BIOS command 3, which has no acknowledgement). */
void neo_geo_audio_reset(void);

#endif
