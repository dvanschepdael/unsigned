/**
 * @file tag.c
 * @brief Implements reference-counted gameplay tag containers.
 */

#include "gameplay/tag.h"

#define GAMEPLAY_TAG_INDEX_NONE UINT8_MAX

/** Finds an active reference entry, or the sentinel when the tag is not owned. */
static u8 gameplay_tag_find_index(const UGameplayTagContainer *container, UGameplayTag tag) {
    if (container->count == 0u) {
        return GAMEPLAY_TAG_INDEX_NONE;
    }

    for (u8 i = 0u; i < container->count; ++i) {
        if (container->instances[i].tag == tag) {
            return i;
        }
    }
    return GAMEPLAY_TAG_INDEX_NONE;
}

static void gameplay_tag_reference_clear(UGameplayTagReference *reference) {
    reference->count = 0u;
    reference->tag = UNSIGNED_GAMEPLAY_TAG_NONE;
}

/** Adds a known-valid reference count to the compact tag set. */
static void gameplay_tag_add_count(UGameplayTagContainer *container, UGameplayTag tag, u8 count) {
    const u8 index = gameplay_tag_find_index(container, tag);
    if (index != GAMEPLAY_TAG_INDEX_NONE) {
        container->instances[index].count = (u8)(container->instances[index].count + count);
        return;
    }

    container->instances[container->count++] = (UGameplayTagReference){
        .count = count,
        .tag = tag,
    };
}

/** Removes a known-owned reference count and compacts the active prefix when needed. */
static void gameplay_tag_remove_count(UGameplayTagContainer *container, UGameplayTag tag, u8 count) {
    const u8 index = gameplay_tag_find_index(container, tag);
    UGameplayTagReference *reference = &container->instances[index];

    reference->count = (u8)(reference->count - count);
    if (reference->count != 0u) {
        return;
    }

    for (u8 i = index; (u8)(i + 1u) < container->count; ++i) {
        container->instances[i] = container->instances[i + 1u];
    }

    gameplay_tag_reference_clear(&container->instances[--container->count]);
}

void unsigned_gameplay_tag_clear(UGameplayTagContainer *container) {
    for (u8 i = 0u; i < container->count; ++i) {
        gameplay_tag_reference_clear(&container->instances[i]);
    }
    container->count = 0u;
}

void unsigned_gameplay_tag_add(UGameplayTagContainer *container, UGameplayTag tag) {
    gameplay_tag_add_count(container, tag, 1u);
}

void unsigned_gameplay_tag_remove_one(UGameplayTagContainer *container, UGameplayTag tag) {
    gameplay_tag_remove_count(container, tag, 1u);
}

void unsigned_gameplay_tag_add_all(UGameplayTagContainer *container, const UGameplayTagContainer *to_add) {
    for (u8 i = 0u; i < to_add->count; ++i) {
        const UGameplayTagReference *source = &to_add->instances[i];
        gameplay_tag_add_count(container, source->tag, source->count);
    }
}

void unsigned_gameplay_tag_remove(UGameplayTagContainer *container, const UGameplayTagContainer *to_remove) {
    for (u8 i = 0u; i < to_remove->count; ++i) {
        const UGameplayTagReference *source = &to_remove->instances[i];
        gameplay_tag_remove_count(container, source->tag, source->count);
    }
}

bool unsigned_gameplay_tag_has(const UGameplayTagContainer *container, UGameplayTag tag) {
    return gameplay_tag_find_index(container, tag) != GAMEPLAY_TAG_INDEX_NONE;
}

bool unsigned_gameplay_tag_has_any(const UGameplayTagContainer *container, const UGameplayTagContainer *other) {
    if (container->count == 0u || other->count == 0u) {
        return false;
    }

    for (u8 i = 0u; i < other->count; ++i) {
        if (unsigned_gameplay_tag_has(container, other->instances[i].tag)) {
            return true;
        }
    }
    return false;
}

bool unsigned_gameplay_tag_has_all(const UGameplayTagContainer *container, const UGameplayTagContainer *required) {
    if (required->count == 0u) {
        return true;
    }
    if (container->count == 0u) {
        return false;
    }

    for (u8 i = 0u; i < required->count; ++i) {
        if (!unsigned_gameplay_tag_has(container, required->instances[i].tag)) {
            return false;
        }
    }
    return true;
}

u8 unsigned_gameplay_tag_reference_count(const UGameplayTagContainer *container, UGameplayTag tag) {
    const u8 index = gameplay_tag_find_index(container, tag);
    return index == GAMEPLAY_TAG_INDEX_NONE ? 0u : container->instances[index].count;
}
