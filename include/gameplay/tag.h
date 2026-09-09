/**
 * @file tag.h
 * @brief Reference-counted gameplay tag container.
 */

#ifndef UNSIGNED_GAMEPLAY_TAG_H
#define UNSIGNED_GAMEPLAY_TAG_H

#include "core/types.h"
#include "gameplay/config.h"

typedef u16 UGameplayTag;

#define UNSIGNED_GAMEPLAY_TAG_NONE ((UGameplayTag)0)

typedef struct UGameplayTagReference {
    u8 count;
    UGameplayTag tag;
} UGameplayTagReference;

typedef struct UGameplayTagContainer {
    u8 count;
    UGameplayTagReference instances[UNSIGNED_GAMEPLAY_MAX_TAG];
} UGameplayTagContainer;

/**
 * @brief Clears every gameplay tag and reference count from a fixed-capacity container.
 *
 * @param container Tag container to reset; NULL is ignored.
 */
void unsigned_gameplay_tag_clear(UGameplayTagContainer *container);

/**
 * @brief Adds one reference to a gameplay tag, creating the tag entry when necessary.
 *
 * @param container Destination tag container.
 * @param tag Non-zero gameplay tag identifier to add.
 * @return true when the reference is added; false for invalid input, full container capacity or reference-count overflow.
 */
bool unsigned_gameplay_tag_add(UGameplayTagContainer *container, UGameplayTag tag);

/**
 * @brief Removes one reference from a gameplay tag and removes the entry when its count reaches zero.
 *
 * @param container Destination tag container.
 * @param tag Non-zero gameplay tag identifier to remove.
 * @return true when one reference is removed; false when the tag/input is invalid or the tag is absent.
 */
bool unsigned_gameplay_tag_remove_one(UGameplayTagContainer *container, UGameplayTag tag);

/**
 * @brief Atomically merges all tag reference counts from another container into the destination.
 *
 * @param container Destination tag container.
 * @param to_add Source tag container; entries must be active and unique.
 * @return true when every source count can be merged; false without modifying the destination when validation, capacity or count overflow would fail.
 */
bool unsigned_gameplay_tag_add_all(UGameplayTagContainer *container, const UGameplayTagContainer *to_add);

/**
 * @brief Atomically subtracts all tag reference counts in another container from the destination.
 *
 * @param container Destination tag container.
 * @param to_remove Source counts to subtract; entries must be active and unique.
 * @return true when all counts are available and removed; false without partial removal when validation or insufficient counts reject the request.
 */
bool unsigned_gameplay_tag_remove(UGameplayTagContainer *container, const UGameplayTagContainer *to_remove);

/**
 * @brief Tests whether a gameplay tag currently has at least one active reference.
 *
 * @param container Tag container to query.
 * @param tag Gameplay tag identifier to find.
 * @return true when the tag is present with a non-zero reference count; false otherwise.
 */
bool unsigned_gameplay_tag_has(const UGameplayTagContainer *container, UGameplayTag tag);

/**
 * @brief Tests whether two tag containers share at least one active gameplay tag.
 *
 * @param container Tag container to query.
 * @param other Tag set to compare against container.
 * @return true when at least one active tag is present in both containers; false for no match or invalid containers.
 */
bool unsigned_gameplay_tag_has_any(const UGameplayTagContainer *container, const UGameplayTagContainer *other);

/**
 * @brief Tests whether the destination contains every active tag required by another container.
 *
 * @param container Tag container to query.
 * @param required Required tag set; an empty valid set succeeds.
 * @return true when every required tag is present; false for a missing tag or invalid required/container data.
 */
bool unsigned_gameplay_tag_has_all(const UGameplayTagContainer *container, const UGameplayTagContainer *required);

/**
 * @brief Returns the active reference count for one gameplay tag.
 *
 * @param container Tag container to query.
 * @param tag Gameplay tag identifier to find.
 * @return Reference count, or 0 when the tag/container is invalid or the tag is absent.
 */
u8 unsigned_gameplay_tag_reference_count(const UGameplayTagContainer *container, UGameplayTag tag);

#endif
