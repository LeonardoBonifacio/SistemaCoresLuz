#include "hardware/pio.h"        // Biblioteca para controle do Bloco Pio em uso
#include "matriz.h"


bool matriz_preenchida[LED_COUNT] = {
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1
};

// Para mandar um valor grb de 32bits(mas so 24 sendo usados) para a maquina de estado 0 do bloco 0 do PIO
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// cria um valor grb de 32 bits
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_one_led(uint8_t r, uint8_t g, uint8_t b, bool desenho[])
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < LED_COUNT; i++)
    {
        if (desenho[i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

// Bloco Pio e state machine usadas na matriz de leds
void configura_Inicializa_Pio(void){
    PIO pio = pio0;// Seleciona o bloco pio que será usado
    int sm = 0; // Define qual state machine será usada
    uint offset = pio_add_program(pio, &ws2812_program);// Carrega o programa PIO para controlar os WS2812 na memória do PIO.
    ws2812_program_init(pio, sm, offset, MATRIZ_LED_PIN, 800000, false); //Inicializa a State Machine para executar o programa PIO carregado.
}
