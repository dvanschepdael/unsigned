/**
 * @file tag.c
 * @brief Implements reference-counted gameplay tag container.
 */

#include "gameplay/tag.h"

#define GAMEPLAY_TAG_INDEX_NONE UINT8_MAX

/** Validates the fixed-capacity tag container count before indexed access. */
static bool gameplay_tag_container_is_valid(const UGameplayTagContainer *container) {
    return container != NULL && container->count <= UNSIGNED_GAMEPLAY_MAX_TAG;
}

/** Treats a tag reference as active only when both its tag id and reference count are non-zero. */
static bool gameplay_tag_reference_is_active(const UGameplayTagReference *reference) {
    return reference != NULL && reference->tag != UNSIGNED_GAMEPLAY_TAG_NONE && reference->count != 0u;
}

/** Detects duplicate tag ids earlier in a source container before performing an all-or-nothing merge. */
static bool gameplay_tag_exists_before(const UGameplayTagContainer *container, u8 index) {
    const UGameplayTag tag = container->instances[index].tag;

    for (u8 i = 0u; i < index; ++i) {
        if (container->instances[i].tag == tag) {
            return true;
        }
    }

    return false;
}

/** Finds the active reference entry for a tag id, or the sentinel when the tag is absent. */
static u8 gameplay_tag_find_index(const UGameplayTagContainer *container, UGameplayTag tag) {
    if (!gameplay_tag_container_is_valid(container) || tag == UNSIGNED_GAMEPLAY_TAG_NONE) {
        return GAMEPLAY_TAG_INDEX_NONE;
    }

    for (u8 i = 0u; i < container->count; ++i) {
        const UGameplayTagReference *reference = &container->instances[i];

        if (gameplay_tag_reference_is_active(reference) && reference->tag == tag) {
            return i;
        }
    }

    return GAMEPLAY_TAG_INDEX_NONE;
}

/** Clears the gameplay tag reference state without freeing caller-owned storage. */
static void gameplay_tag_reference_clear(UGameplayTagReference *reference) {
    if (reference == NULL) {
        return;
    }

    reference->count = 0u;
    reference->tag = UNSIGNED_GAMEPLAY_TAG_NONE;
}

/** Increments an existing tag reference count or appends a new entry without overflowing fixed capacity/counts. */
static bool gameplay_tag_add_count(UGameplayTagContainer *container, UGameplayTag tag, u8 count) {
    if (!gameplay_tag_container_is_valid(container) || tag == UNSIGNED_GAMEPLAY_TAG_NONE || count == 0u) {
        return false;
    }

    u8 index = gameplay_tag_find_index(container, tag);
    if (index != GAMEPLAY_TAG_INDEX_NONE) {
        UGameplayTagReference *reference = &container->instances[index];

        if ((u16)reference->count + count > UINT8_MAX) {
            return false;
        }

        reference->count = (u8)(reference->count + count);
        return true;
    }

    if (container->count >= UNSIGNED_GAMEPLAY_MAX_TAG) {
        return false;
    }

    container->instances[container->count] = (UGameplayTagReference){
        .count = count,
        .tag = tag,
    };
    ++container->count;
    return true;
}

/** Decrements a tag reference count and compacts the fixed array when the count reaches zero. */
static bool gameplay_tag_remove_count(UGameplayTagContainer *container, UGameplayTag tag, u8 count) {
    if (!gameplay_tag_container_is_valid(container) || tag == UNSIGNED_GAMEPLAY_TAG_NONE || count == 0u) {
        return false;
    }

    u8 index = gameplay_tag_find_index(container, tag);
    if (index == GAMEPLAY_TAG_INDEX_NONE) {
        return false;
    }

    UGameplayTagReference *reference = &container->instances[index];
    if (reference->count < count) {
        return false;
    }

    reference->count = (u8)(reference->count - count);
    if (reference->count != 0u) {
        return true;
    }

    for (u8 i = index; (u8)(i + 1u) < container->count; ++i) {
        container->instances[i] = container->instances[i + 1u];
    }

    --container->count;
    gameplay_tag_reference_clear(&container->instances[container->count]);
    return true;
}

void unsigned_gameplay_tag_clear(UGameplayTagContainer *container) {
    if (container == NULL) {
        return;
    }

    for (u8 i = 0u; i < UNSIGNED_GAMEPLAY_MAX_TAG; ++i) {
        gameplay_tag_reference_clear(&container->instances[i]);
    }

    container->count = 0u;
}

bool unsigned_gameplay_tag_add(UGameplayTagContainer *container, UGameplayTag tag) {
    return gameplay_tag_add_count(container, tag, 1u);
}

bool unsigned_gameplay_tag_remove_one(UGameplayTagContainer *container, UGameplayTag tag) {
    return gameplay_tag_remove_count(container, tag, 1u);
}

bool unsigned_gameplay_tag_add_all(UGameplayTagContainer *container, const UGameplayTagContainer *to_add) {
    u8 missing = 0u;

    if (!gameplay_tag_container_is_valid(container) || !gameplay_tag_container_is_valid(to_add)) {
        return false;
    }

    for (u8 i = 0u; i < to_add->count; ++i) {
        const UGameplayTagReference *source = &to_add->instances[i];
        const u8 target_index = gameplay_tag_find_index(container, source->tag);

        if (!gameplay_tag_reference_is_active(source) || gameplay_tag_exists_before(to_add, i)) {
            return false;
        }

        if (target_index == GAMEPLAY_TAG_INDEX_NONE) {
            ++missing;
            continue;
        }

        if ((u16)container->instances[target_index].count + source->count > UINT8_MAX) {
            return false;
        }
    }

    if ((u16)container->count + missing > UNSIGNED_GAMEPLAY_MAX_TAG) {
        return false;
    }

    for (u8 i = 0u; i < to_add->count; ++i) {
        const UGameplayTagReference *source = &to_add->instances[i];

        if (!gameplay_tag_add_count(container, source->tag, source->count)) {
            return false;
        }
    }

    return true;
}

bool unsigned_gameplay_tag_remove(UGameplayTagContainer *container, const UGameplayTagContainer *to_remove) {
    if (!gameplay_tag_container_is_valid(container) || !gameplay_tag_container_is_valid(to_remove)) {
        return false;
    }

    if (container == to_remove) {
        unsigned_gameplay_tag_clear(container);
        return true;
    }

    for (u8 i = 0u; i < to_remove->count; ++i) {
        const UGameplayTagReference *source = &to_remove->instances[i];
        const u8 target_index = gameplay_tag_find_index(container, source->tag);

        if (!gameplay_tag_reference_is_active(source) || gameplay_tag_exists_before(to_remove, i) || target_index == GAMEPLAY_TAG_INDEX_NONE || container->instances[target_index].count < source->count) {
            return false;
        }
    }

    for (u8 i = 0u; i < to_remove->count; ++i) {
        const UGameplayTagReference *source = &to_remove->instances[i];

        if (!gameplay_tag_remove_count(container, source->tag, source->count)) {
            return false;
        }
    }

    return true;
}

bool unsigned_gameplay_tag_has(const UGameplayTagContainer *container, UGameplayTag tag) {
    return gameplay_tag_find_index(container, tag) != GAMEPLAY_TAG_INDEX_NONE;
}

bool unsigned_gameplay_tag_has_any(const UGameplayTagContainer *container, const UGameplayTagContainer *other) {
    if (!gameplay_tag_container_is_valid(container) || !gameplay_tag_container_is_valid(other)) {
        return false;
    }

    for (u8 i = 0u; i < other->count; ++i) {
        const UGameplayTagReference *reference = &other->instances[i];

        if (gameplay_tag_reference_is_active(reference) && unsigned_gameplay_tag_has(container, reference->tag)) {
            return true;
        }
    }

    return false;
}

bool unsigned_gameplay_tag_has_all(const UGameplayTagContainer *container, const UGameplayTagContainer *required) {
    if (!gameplay_tag_container_is_valid(required)) {
        return false;
    }
    if (required->count == 0u) {
        return true;
    }
    if (!gameplay_tag_container_is_valid(container)) {
        return false;
    }

    for (u8 i = 0u; i < required->count; ++i) {
        const UGameplayTagReference *reference = &required->instances[i];

        if (!gameplay_tag_reference_is_active(reference) || !unsigned_gameplay_tag_has(container, reference->tag)) {
            return false;
        }
    }

    return true;
}

u8 unsigned_gameplay_tag_reference_count(const UGameplayTagContainer *container, UGameplayTag tag) {
    const u8 index = gameplay_tag_find_index(container, tag);

    if (index == GAMEPLAY_TAG_INDEX_NONE) {
        return 0u;
    }

    return container->instances[index].count;
}
