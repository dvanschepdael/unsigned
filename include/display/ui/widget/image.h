/**
 * @file image.h
 * @brief Image/asset widget.
 */

#ifndef UNSIGNED_DISPLAY_UI_WIDGET_IMAGE_H
#define UNSIGNED_DISPLAY_UI_WIDGET_IMAGE_H

#include "display/ui/element.h"

typedef struct UIImage {
    UUIElement element;
    UUIAssetId asset;
} UIImage;

/**
 * @brief Initializes the UI image to a valid empty runtime state.
 *
 * @param image Image widget to configure.
 * @param bounds Bounds used for layout, culling, collision, or effect calculations.
 * @param style UI style identifier resolved by the active theme.
 * @param asset UI asset identifier displayed by the image widget.
 */
void unsigned_ui_image_init(UIImage *image, UUIRect bounds, UUIStyleId style, UUIAssetId asset);

/**
 * @brief Sets asset on the UI image.
 *
 * @param image Image widget to configure.
 * @param asset UI asset identifier displayed by the image widget.
 */
void unsigned_ui_image_set_asset(UIImage *image, UUIAssetId asset);

#endif
