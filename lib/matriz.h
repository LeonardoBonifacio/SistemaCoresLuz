#include "hardware/pio.h"        // Biblioteca para controle do Bloco Pio em uso
#include "ws2812.pio.h"



// Definição do número de LEDs e pinos.
#define LED_COUNT 25
#define MATRIZ_LED_PIN 7

extern bool matriz_preenchida[LED_COUNT];


static inline void put_pixel(uint32_t pixel_grb);
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
void set_one_led(uint8_t r, uint8_t g, uint8_t b, bool desenho[]);
void configura_Inicializa_Pio(void);