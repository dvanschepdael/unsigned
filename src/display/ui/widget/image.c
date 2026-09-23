/**
 * @file image.c
 * @brief Implements image/asset widget.
 */

#include "display/ui/widget/image.h"

void unsigned_ui_image_init(UIImage *image, UUIRect bounds, UUIStyleId style, UUIAssetId asset) {
    *image = (UIImage){
        .asset = asset,
    };
    unsigned_ui_element_init(&image->element, U_UI_ELEMENT_IMAGE, bounds, style, false);
}
