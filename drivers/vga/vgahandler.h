#ifndef VGAHANDLER_H
#define VGAHANDLER_H

#include <stdint.h>
#include <stdint.h>
#include "../../lib/flopmath.h"
#include "../../drivers/time/floptime.h"
#include "framebuffer.h"

#define VGA_TEXT_ADDRESS 0xB8000
#define VGA_GRAPHICS_ADDRESS 0xA0000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_GRAPHICS_WIDTH 320
#define VGA_GRAPHICS_HEIGHT 200

#define	VGA_AC_INDEX		0x3C0
#define	VGA_AC_WRITE		0x3C0
#define	VGA_AC_READ		    0x3C1
#define	VGA_INSTAT_READ		0x3DA
#define	VGA_MISC_WRITE		0x3C2
#define	VGA_MISC_READ		0x3CC

// VGA Graphics Register Ports
#define VGA_CRTC_INDEX		0x3D4		
#define VGA_CRTC_DATA		0x3D5		
#define VGA_GC_INDEX 		0x3CE
#define VGA_GC_DATA 		0x3CF
#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5

#define	VGA_NUM_AC_REGS		21
#define	VGA_NUM_CRTC_REGS	25
#define	VGA_NUM_GC_REGS		9
#define	VGA_NUM_SEQ_REGS	5

// text mode colors
#define BLACK        0
#define BLUE         1
#define GREEN        2
#define CYAN         3
#define RED          4
#define MAGENTA      5
#define BROWN        6
#define LIGHT_GRAY   7
#define DARK_GRAY    8
#define LIGHT_BLUE   9
#define LIGHT_GREEN  10
#define LIGHT_CYAN   11
#define LIGHT_RED    12
#define LIGHT_MAGENTA 13
#define LIGHT_BROWN  14
#define WHITE        15
#define YELLOW       14

// Standard VGA graphics colors
#define VGA_BLACK              0x00
#define VGA_BLUE               0x01
#define VGA_GREEN              0x02
#define VGA_CYAN               0x03
#define VGA_RED                0x04
#define VGA_MAGENTA            0x05
#define VGA_BROWN              0x06
#define VGA_LIGHT_GRAY         0x07
#define VGA_DARK_GRAY          0x08
#define VGA_LIGHT_BLUE         0x09
#define VGA_LIGHT_GREEN        0x0A
#define VGA_LIGHT_CYAN         0x0B
#define VGA_LIGHT_RED          0x0C
#define VGA_LIGHT_MAGENTA      0x0D
#define VGA_LIGHT_BROWN        0x0E
#define VGA_WHITE              0x0F

// Extended VGA Colors (0x10 to 0xFF)
#define VGA_GRAY_1             0x10
#define VGA_GRAY_2             0x11
#define VGA_GRAY_3             0x12
#define VGA_GRAY_4             0x13
#define VGA_GRAY_5             0x14
#define VGA_GRAY_6             0x15
#define VGA_GRAY_7             0x16
#define VGA_GRAY_8             0x17
#define VGA_GRAY_9             0x18
#define VGA_GRAY_10            0x19
#define VGA_GRAY_11            0x1A
#define VGA_GRAY_12            0x1B
#define VGA_GRAY_13            0x1C
#define VGA_GRAY_14            0x1D
#define VGA_GRAY_15            0x1E
#define VGA_GRAY_16            0x1F
#define VGA_LAVENDER           0x20
#define VGA_SKY_BLUE           0x21
#define VGA_SPRING_GREEN       0x22
#define VGA_TURQUOISE          0x23
#define VGA_ORANGE             0x24
#define VGA_PINK               0x25
#define VGA_GOLD               0x26
#define VGA_PEACH              0x27
#define VGA_SEA_GREEN          0x28
#define VGA_TEAL               0x29
#define VGA_CORAL              0x2A
#define VGA_ROSE               0x2B
#define VGA_LIME_GREEN         0x2C
#define VGA_AQUA               0x2D
#define VGA_CHERRY             0x2E
#define VGA_INDIGO             0x2F
#define VGA_VIOLET             0x30
#define VGA_BEIGE              0x31
#define VGA_TAN                0x32
#define VGA_BRICK_RED          0x33
#define VGA_OLIVE              0x34
#define VGA_MIDNIGHT_BLUE      0x35
#define VGA_FUCHSIA            0x36
#define VGA_PERIWINKLE         0x37
#define VGA_CHARCOAL           0x38
#define VGA_IVORY              0x39
#define VGA_SAND               0x3A
#define VGA_HONEY              0x3B
#define VGA_PLUM               0x3C
#define VGA_MINT_GREEN         0x3D
#define VGA_SLATE_BLUE         0x3E
#define VGA_MAROON             0x3F
#define VGA_FOREST_GREEN       0x40
#define VGA_LIGHT_PINK         0x41
#define VGA_WHEAT              0x42
#define VGA_MUSTARD            0x43
#define VGA_AQUAMARINE         0x44
#define VGA_DARK_TEAL          0x45
#define VGA_HOT_PINK           0x46
#define VGA_CRIMSON            0x47
#define VGA_POWDER_BLUE        0x48
#define VGA_AMBER              0x49
#define VGA_GRAPE              0x4A
#define VGA_LIGHT_CORAL        0x4B
#define VGA_DARK_LAVENDER      0x4C
#define VGA_SILVER             0x4D
#define VGA_PEARL              0x4E
#define VGA_SMOKE              0x4F
#define VGA_FLAME              0x50
#define VGA_DARK_BROWN         0x51
#define VGA_NAVY               0x52
#define VGA_MAHOGANY           0x53
#define VGA_MAUVE              0x54
#define VGA_LIGHT_LAVENDER     0x55
#define VGA_SAGE_GREEN         0x56
#define VGA_DARK_RED           0x57
#define VGA_RUST               0x58
#define VGA_SLATE_GRAY         0x59
#define VGA_LIGHT_YELLOW       0x5A
#define VGA_SUNNY_YELLOW       0x5B
#define VGA_CHAMPAGNE          0x5C
#define VGA_CREAM              0x5D
#define VGA_DARK_OLIVE         0x5E
#define VGA_LIGHT_TEAL         0x5F
#define VGA_FOG                0x60
#define VGA_PETAL              0x61
#define VGA_BLUSH              0x62
#define VGA_MULBERRY           0x63
#define VGA_ORCHID             0x64
#define VGA_CREAM_SODA         0x65
#define VGA_PAPAYA             0x66
#define VGA_LIGHT_MINT         0x67
#define VGA_JADE               0x68
#define VGA_PEACOCK            0x69
#define VGA_EMERALD            0x6A
#define VGA_DRIFTWOOD          0x6B
#define VGA_EARTH              0x6C
#define VGA_DARK_PEACH         0x6D
#define VGA_CEDAR              0x6E
#define VGA_ONYX               0x6F
#define VGA_COBALT             0x70
#define VGA_ASH                0x71
#define VGA_COTTON_CANDY       0x72
#define VGA_NIGHT_SKY          0x73
#define VGA_DAWN               0x74
#define VGA_TWILIGHT           0x75
#define VGA_LIGHT_PEACH        0x76
#define VGA_TANGERINE          0x77
#define VGA_SUNNY_ORANGE       0x78
#define VGA_VERMILLION         0x79
#define VGA_SAPPHIRE           0x7A
#define VGA_RUBY               0x7B
#define VGA_HAZELWOOD          0x7C
#define VGA_WISTERIA           0x7D
#define VGA_MOSS_GREEN         0x7E
#define VGA_DEEP_SEA           0x7F
#define VGA_BRIGHT_GREEN       0x80
#define VGA_DARK_PURPLE        0x81
#define VGA_DEEP_ORANGE        0x82
#define VGA_PALE_YELLOW        0x83
#define VGA_COOL_GRAY          0x84
#define VGA_BRIGHT_AQUA        0x85
#define VGA_PURE_RED           0x86
#define VGA_BANANA_YELLOW      0x87
#define VGA_CRYSTAL_BLUE       0x88
#define VGA_GOLDENROD          0x89
#define VGA_SHADOW_GRAY        0x8A
#define VGA_MINT_BLUE          0x8B
#define VGA_CANDY_APPLE_RED    0x8C
#define VGA_LIME_CITRUS        0x8D
#define VGA_DARK_CHARCOAL      0x8E
#define VGA_SOFT_PEACH         0x8F
#define VGA_DEEP_VIOLET        0x90
#define VGA_BOLD_ORANGE        0x91
#define VGA_CITRINE            0x92
#define VGA_BLOOD_ORANGE       0x93
#define VGA_BRIGHT_WHITE       0x94
#define VGA_LIGHT_INDIGO       0x95
#define VGA_GLEAMING_GOLD      0x96
#define VGA_ICY_BLUE           0x97
#define VGA_MIDNIGHT_BLACK     0x98
#define VGA_DARK_FOREST        0x99
#define VGA_NEON_GREEN         0x9A
#define VGA_BOLD_CYAN          0x9B
#define VGA_FLUORESCENT_LIME   0x9C
#define VGA_RICH_MAROON        0x9D
#define VGA_VIVID_AZURE        0x9E
#define VGA_CHROMATIC_RED      0x9F
#define VGA_LIGHT_CHARTREUSE   0xA0
#define VGA_PASTEL_PINK        0xA1
#define VGA_COBALT_BLUE        0xA2
#define VGA_DEEP_MAGENTA       0xA3
#define VGA_ELECTRIC_GREEN     0xA4
#define VGA_RICH_TURQUOISE     0xA5
#define VGA_SOFT_GOLD          0xA6
#define VGA_LILAC              0xA7
#define VGA_TAUPE              0xA8
#define VGA_MOCHA              0xA9
#define VGA_BERRY_RED          0xAA
#define VGA_DUSK_BLUE          0xAB
#define VGA_MORNING_MIST       0xAC
#define VGA_ICE_PURPLE         0xAD
#define VGA_MANGO              0xAE
#define VGA_LIGHT_OLIVE        0xAF
#define VGA_BRIGHT_COBALT      0xB0
#define VGA_DARK_SAPPHIRE      0xB1
#define VGA_PEARL_WHITE        0xB2
#define VGA_RASPBERRY          0xB3
#define VGA_CHERRY_BLOSSOM     0xB4
#define VGA_SAPPHIRE_SHIMMER   0xB5
#define VGA_CARROT_ORANGE      0xB6
#define VGA_DARK_SLATE         0xB7
#define VGA_LIGHT_TANGERINE    0xB8
#define VGA_METALLIC_GOLD      0xB9
#define VGA_LIGHT_COBALT       0xBA
#define VGA_GRAPEFRUIT         0xBB
#define VGA_BLUSH_PINK         0xBC
#define VGA_PALE_CREAM         0xBD
#define VGA_BRIGHT_INDIGO      0xBE
#define VGA_LIGHT_PLUM         0xBF
#define VGA_SUNSET_ORANGE      0xC0
#define VGA_CELESTIAL_BLUE     0xC1
#define VGA_MYSTIC_PURPLE      0xC2
#define VGA_SHIMMERING_SILVER  0xC3
#define VGA_VINTAGE_BROWN      0xC4
#define VGA_BRIGHT_VERMILION   0xC5
#define VGA_SOFT_MAUVE         0xC6
#define VGA_PEACH_BLOSSOM      0xC7
#define VGA_PURE_YELLOW        0xC8
#define VGA_TITANIUM_GRAY      0xC9
#define VGA_NEON_ORANGE        0xCA
#define VGA_BRIGHT_ROSE        0xCB
#define VGA_DARK_LIME          0xCC
#define VGA_SEPIA              0xCD
#define VGA_CREAM_YELLOW       0xCE
#define VGA_SUNFLOWER_YELLOW   0xCF
#define VGA_RUBY_RED           0xD0
#define VGA_COBALT_AQUA        0xD1
#define VGA_DEEP_EMERALD       0xD2
#define VGA_LIGHT_CRIMSON      0xD3
#define VGA_PASTEL_PURPLE      0xD4
#define VGA_TURMERIC           0xD5
#define VGA_COPPER             0xD6
#define VGA_BRIGHT_BROWN       0xD7
#define VGA_ICY_AQUA           0xD8
#define VGA_MOSSY_GREEN        0xD9
#define VGA_WINE_RED           0xDA
#define VGA_SHIMMER_BLUE       0xDB
#define VGA_PEARL_GRAY         0xDC
#define VGA_IVORY_WHITE        0xDD
#define VGA_FROSTED_MINT       0xDE
#define VGA_ROYAL_PURPLE       0xDF
#define VGA_LUSH_GREEN         0xE0
#define VGA_TROPICAL_ORANGE    0xE1
#define VGA_SPRING_MINT        0xE2
#define VGA_DUSK_PURPLE        0xE3
#define VGA_CHAMPAGNE_GOLD     0xE4
#define VGA_SANDSTONE          0xE5
#define VGA_LIGHT_RUBY         0xE6
#define VGA_DARK_MINT          0xE7
#define VGA_GLOSSY_BLUE        0xE8
#define VGA_CORAL_RED          0xE9
#define VGA_DARK_WISTERIA      0xEA
#define VGA_SHIMMER_GOLD       0xEB
#define VGA_SOFT_CHARCOAL      0xEC
#define VGA_PASTEL_GREEN       0xED
#define VGA_SALMON_PINK        0xEE
#define VGA_SAPPHIRE_MIST      0xEF
#define VGA_MIDNIGHT_PURPLE    0xF0
#define VGA_CLOUDY_SKY         0xF1
#define VGA_LIGHT_BRONZE       0xF2
#define VGA_BRIGHT_TURQUOISE   0xF3
#define VGA_BUBBLEGUM_PINK     0xF4
#define VGA_LEMON_YELLOW       0xF5
#define VGA_GLOWING_GREEN      0xF6
#define VGA_SUNSET_GOLD        0xF7
#define VGA_DARK_BLUE_GRAY     0xF8
#define VGA_LIGHT_AMETHYST     0xF9
#define VGA_RICH_RUBY          0xFA
#define VGA_ELECTRIC_PURPLE    0xFB
#define VGA_BRIGHT_ORANGE      0xFC
#define VGA_SNOW_WHITE         0xFD
#define VGA_BRIGHT_CRIMSON     0xFE
#define VGA_ULTRA_BLACK        0xFF

// External framebuffer structure
extern framebuffer_t fb;

typedef struct {
    int pos;
    char name[50];
    int pid;
    void *memory;

} Window;

void vga_clear_terminal(void);
void framebuffer_initialize_wrapper(multiboot_info_t *mbi);
void vga_clear_screen();
void draw_pixel(int x, int y, uint8_t color);
void draw_line(int y, uint8_t color);
void vga_place_char(uint16_t x, uint16_t y, char c, uint8_t color);
void fill_screen(uint8_t color) ;
void vga_set_cursor_position(uint16_t x, uint16_t y);
void vga_save_terminal_state(void);
void vga_restore_terminal_state(void);
void textmode_draw_hline(int x, int y, int length, uint32_t color);
void textmode_draw_vline(int x, int y, int length, uint32_t color);
void textmode_draw_rectangle(int x, int y, int width, int height, uint32_t color);
void textmode_draw_filled_rectangle(int x, int y, int width, int height, uint32_t border_color, uint32_t fill_color);
void textmode_draw_diagonal_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_spinning_cube(void);
void set_vga_mode();
void test_graphics_mode();
void vga_place_bold_char(uint16_t x, uint16_t y, char c, uint8_t color);
void clear_screen(uint8_t color);
void vga_test();
void vga_desktop();
void vga_init();
void console_render();
void console_print(const char *str, unsigned short color);
void console_clear_screen();
void vga_plot_pixel(int x, int y, unsigned short color);
void vga_draw_string(int x, int y, const char *str, uint32_t color);
#endif // VGAHANDLER_H