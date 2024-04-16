/**
 * @file lv_font.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_font.h"
#include "lv_font_fmt_txt.h"
#include "../misc/lv_utils.h"
#include "../misc/lv_log.h"
#include "../misc/lv_assert.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Return with the bitmap of a font.
 * @param font_p pointer to a font
 * @param letter a UNICODE character code
 * @return pointer to the bitmap of the letter
 */
const uint8_t * lv_font_get_glyph_bitmap(const lv_font_t * font_p, uint32_t letter)
{
    LV_ASSERT_NULL(font_p);
#ifdef LV_CONF_SUPPORT_WASM
    if (!font_p->get_glyph_bitmap) {
        return lv_font_get_bitmap_fmt_txt(font_p, letter);
    } else if (font_p->module_inst && !esp_ptr_executable(font_p->get_glyph_bitmap)) {
        uint32_t argv[2];

        argv[0] = (uint32_t)font_p;
        argv[1] = letter;

        lv_run_wasm(font_p->module_inst, font_p->get_glyph_bitmap, 2, argv);

        return (const uint8_t *)argv[0];
    } else {
        return font_p->get_glyph_bitmap(font_p, letter);
    }
#else
    return font_p->get_glyph_bitmap(font_p, letter);
#endif
}

/**
 * Get the descriptor of a glyph
 * @param font_p pointer to font
 * @param dsc_out store the result descriptor here
 * @param letter a UNICODE letter code
 * @param letter_next the next letter after `letter`. Used for kerning
 * @return true: descriptor is successfully loaded into `dsc_out`.
 *         false: the letter was not found, no data is loaded to `dsc_out`
 */
bool lv_font_get_glyph_dsc(const lv_font_t * font_p, lv_font_glyph_dsc_t * dsc_out, uint32_t letter,
                           uint32_t letter_next)
{

    LV_ASSERT_NULL(font_p);
    LV_ASSERT_NULL(dsc_out);

#if LV_USE_FONT_PLACEHOLDER
    const lv_font_t * placeholder_font = NULL;
#endif

    const lv_font_t * f = font_p;

    dsc_out->resolved_font = NULL;
	bool found  = false;
    while(f) {
#ifdef LV_CONF_SUPPORT_WASM
    if (!f->get_glyph_dsc) {
        found = lv_font_get_glyph_dsc_fmt_txt(f, dsc_out, letter, letter_next);
    } else if (f->module_inst && !esp_ptr_executable(f->get_glyph_dsc)) {
        uint32_t argv[4];

        argv[0] = (uint32_t)f;
        argv[1] = (uint32_t)dsc_out;
        argv[2] = letter;
        argv[3] = letter_next;

        lv_run_wasm(f->module_inst,f->get_glyph_dsc, 4, argv);
        found = (bool)argv[0];
    } else {
        found = f->get_glyph_dsc(f, dsc_out, letter, letter_next);
    }
#else
        found = f->get_glyph_dsc(f, dsc_out, letter, letter_next);
#endif
        if(found) {
            if(!dsc_out->is_placeholder) {
                dsc_out->resolved_font = f;
                return true;
            }
#if LV_USE_FONT_PLACEHOLDER
            else if(placeholder_font == NULL) {
                placeholder_font = f;
            }
#endif
        }
        f = f->fallback;
    }

#if LV_USE_FONT_PLACEHOLDER
    if(placeholder_font != NULL) {
        placeholder_font->get_glyph_dsc(placeholder_font, dsc_out, letter, letter_next);
        dsc_out->resolved_font = placeholder_font;
        return true;
    }
#endif

    if(letter < 0x20 ||
       letter == 0xf8ff || /*LV_SYMBOL_DUMMY*/
       letter == 0x200c) { /*ZERO WIDTH NON-JOINER*/
        dsc_out->box_w = 0;
        dsc_out->adv_w = 0;
    }
    else {
#if LV_USE_FONT_PLACEHOLDER
        dsc_out->box_w = font_p->line_height / 2;
        dsc_out->adv_w = dsc_out->box_w + 2;
#else
        dsc_out->box_w = 0;
        dsc_out->adv_w = 0;
#endif
    }

    dsc_out->resolved_font = NULL;
    dsc_out->box_h = font_p->line_height;
    dsc_out->ofs_x = 0;
    dsc_out->ofs_y = 0;
    dsc_out->bpp   = 1;
    dsc_out->is_placeholder = true;

    return false;
}

/**
 * Get the width of a glyph with kerning
 * @param font pointer to a font
 * @param letter a UNICODE letter
 * @param letter_next the next letter after `letter`. Used for kerning
 * @return the width of the glyph
 */
uint16_t lv_font_get_glyph_width(const lv_font_t * font, uint32_t letter, uint32_t letter_next)
{
    LV_ASSERT_NULL(font);
    lv_font_glyph_dsc_t g;
    bool ret;
    ret = lv_font_get_glyph_dsc(font, &g, letter, letter_next);
    if(ret) return g.adv_w;
    else return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
