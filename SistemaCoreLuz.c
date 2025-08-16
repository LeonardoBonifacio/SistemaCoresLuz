#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "bh1750_light_sensor.h"
#include "ssd1306.h"
#include "font.h"
#include "pico/bootrom.h"
#include "matriz.h"
#include "buzzer.h"
#define botaoB 6  // Utilizado aqui para o bootsel

// Definições do sensor GY-33
#define GY33_I2C_ADDR 0x29 // Endereço do GY-33 no barramento I2C
#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

// Registros do sensor gy-33
#define ENABLE_REG 0x80
#define ATIME_REG 0x81
#define CONTROL_REG 0x8F
#define ID_REG 0x92
#define STATUS_REG 0x93
#define CDATA_REG 0x94 //  "Clear"
#define RDATA_REG 0x96 //  "Red"
#define GDATA_REG 0x98 //  "Green"
#define BDATA_REG 0x9A //  "Blue"

// Limites para alertas
#define LUX_LIMITE_BAIXO 50 // Limite de luminosidade baixa (lux)

// Display na I2C
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C

// PWM: inicializa PWM nos três pinos RGB com wrap máximo
#define PWM_WRAP 65535

// Protótipos das funções
void gy33_write_register(uint8_t reg, uint8_t value);
uint16_t gy33_read_register(uint8_t reg);
void gy33_init();
void gy33_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c);
const char* determine_color_name(uint16_t r, uint16_t g, uint16_t b);
float calculate_saturation(uint16_t r, uint16_t g, uint16_t b);
uint8_t adjust_brightness(uint16_t lux);
void set_rgb_outputs_for_state(int state);
void init_pwm_rgb(void);
uint16_t get_color_frequency(const char* color_name); // Nova função


// --- Definições dos Pinos ---
const uint BTN_A_PIN = 5;  // Pino para o Botão A
const uint RED_PIN = 13;   // Pino para o LED vermelho
const uint GREEN_PIN = 11; // Pino para o LED verde
const uint BLUE_PIN = 12;  // Pino para o LED azul

// --- Variáveis de Controle para botão / led_state ---
volatile int led_state = 0;            // 0=Vermelho, 1=Verde, 2=Azul, 3=Amarelo, 4=Magenta, 5=Ciano, 6=Branco
volatile uint32_t last_press_time = 0; // debounce (ms)


int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();
    sleep_ms(2000);

    // I2C do Display pode ser diferente dos sensores. Funcionando em 400Khz.
    i2c_init(I2C_PORT_DISP, 400 * 1000);

    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    init_pwm_rgb();

    // Configuração do pino do botão A (ciclar LED)
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);


    // Inicializa o sensor de cor
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    printf("Iniciando GY-33...\n");
    gy33_init();

    // Inicializa o sensor de luz BH1750
    bh1750_power_on(I2C_PORT);

    // Inicializa o buzzer
    init_buzzer(BUZZER_A_PIN, 50.0f);
    init_buzzer(BUZZER_B_PIN, 50.0f);

    // Inicializa a matriz LED
    configura_Inicializa_Pio();

    // Buffer para mensagens do display
    char buffer[64];

    // Variáveis para armazenar os valores dos sensores
    uint16_t r, g, b, c;
    uint16_t lux;
    float saturation;
    const char* color_name;
    const char* last_color_name = ""; // Armazena a última cor detectada
    uint8_t brightness;
    bool brightness_alert = false;

    // Mensagem de inicialização
    ssd1306_draw_string(&ssd,"Sistema de cor", 5, 10);
    ssd1306_draw_string(&ssd, "e Luminosidade", 5, 25);
    ssd1306_draw_string(&ssd, "Inicializando...", 5, 40);
    ssd1306_send_data(&ssd);
    sleep_ms(2000);


    // Define LED inicial (aplica estado inicial)
    set_rgb_outputs_for_state(led_state);

    while (true) {
        // --- Aplique o estado físico do LED (caso o usuário tenha mudado) ---
        // Quando o led_state for alterado pelo botão, set_rgb_outputs_for_state é chamada a cada ciclo.
        // Para ajudar a detecção, se a cor física mudou, aguarde um pouco antes de ler o sensor.
        static int prev_led_state = -1;
        if (prev_led_state != led_state) {
            prev_led_state = led_state;
            set_rgb_outputs_for_state(led_state);
            // Espera para estabilizar iluminação (facilita leitura consistente)
            sleep_ms(120);
        }

        // Lê os valores de cor do GY-33
        gy33_read_color(&r, &g, &b, &c);

        // Lê o valor de luminosidade do BH1750
        lux = bh1750_read_measurement(I2C_PORT);

        // Calcula a saturação
        saturation = calculate_saturation(r, g, b);

        // Determina o nome da cor
        color_name = determine_color_name(r, g, b);

        // Verifica se a cor mudou e toca o buzzer com frequência correspondente
        if (strcmp(color_name, last_color_name) != 0 && strcmp(color_name, "Escuro") != 0 && strcmp(color_name, "Indefinido") != 0) {
            // Obtém a frequência para a cor detectada
            uint16_t freq = get_color_frequency(color_name);

            // Toca o buzzer brevemente com a frequência da cor
            play_tone(BUZZER_A_PIN, freq);
            sleep_ms(150);
            stop_tone(BUZZER_A_PIN);

            // Atualiza a última cor detectada
            last_color_name = color_name;
        }

        // Ajusta o brightness com base na luminosidade
        brightness = adjust_brightness(lux);

        // Limpa o display
        ssd1306_fill(&ssd, false);

        // Exibe as informações no display OLED
        ssd1306_draw_string(&ssd, "Cor Detectada:", 0, 0);

        snprintf(buffer, sizeof(buffer), "%s", color_name);
        ssd1306_draw_string(&ssd, buffer, 0, 10);

        snprintf(buffer, sizeof(buffer), "R:%d G:%d B:%d", r, g, b);
        ssd1306_draw_string(&ssd, buffer, 0, 20);

        snprintf(buffer, sizeof(buffer), "Sat: %.1f%%", saturation);
        ssd1306_draw_string(&ssd, buffer, 0, 30);

        snprintf(buffer, sizeof(buffer), "Lux: %d", lux);
        ssd1306_draw_string(&ssd, buffer, 0, 40);

        // Exibe o estado do sistema
        if (lux < LUX_LIMITE_BAIXO) {
            ssd1306_draw_string(&ssd, "ALERTA: Luz Baixa!", 0, 50);
        } else {
            ssd1306_draw_string(&ssd, "Sistema OK", 0, 50);
        }

        // Atualiza o display
        ssd1306_send_data(&ssd);

        // Controla a matriz LED
        // Normaliza os valores RGB para 0-255
        uint8_t r_norm = (r > 255) ? 255 : (uint8_t)r;
        uint8_t g_norm = (g > 255) ? 255 : (uint8_t)g;
        uint8_t b_norm = (b > 255) ? 255 : (uint8_t)b;

        // Ajusta o brightness com base na luminosidade ambiente
        r_norm = (uint8_t)((r_norm * brightness) / 255);
        g_norm = (uint8_t)((g_norm * brightness) / 255);
        b_norm = (uint8_t)((b_norm * brightness) / 255);

        // Atualiza a matriz LED com a cor detectada
        set_one_led(r_norm, g_norm, b_norm, matriz_preenchida);

        // Verifica condições para alertas sonoros de luminosidade (mantém o código existente)
        if (lux < LUX_LIMITE_BAIXO && !brightness_alert) {
            // Alerta de luminosidade baixa
            play_tone(BUZZER_B_PIN, 10000); // Tom de 1000Hz
            sleep_ms(200);
            stop_tone(BUZZER_B_PIN);
            sleep_ms(200);
            play_tone(BUZZER_B_PIN, 10000);
            sleep_ms(200);
            stop_tone(BUZZER_B_PIN);
            brightness_alert = true;
        } else if (lux >= LUX_LIMITE_BAIXO) {
            brightness_alert = false;
        }

        // Aguarda antes da próxima leitura
        sleep_ms(100);
    }
}

// --- Função para inicializar PWM nos pinos RGB ---
void init_pwm_rgb(void) {
    // Configure função PWM nos pinos
    gpio_set_function(RED_PIN, GPIO_FUNC_PWM);
    gpio_set_function(GREEN_PIN, GPIO_FUNC_PWM);
    gpio_set_function(BLUE_PIN, GPIO_FUNC_PWM);

    // Slices
    uint slice_r = pwm_gpio_to_slice_num(RED_PIN);
    uint slice_g = pwm_gpio_to_slice_num(GREEN_PIN);
    uint slice_b = pwm_gpio_to_slice_num(BLUE_PIN);

    // Configura wrap para cada slice
    pwm_set_wrap(slice_r, PWM_WRAP);
    pwm_set_wrap(slice_g, PWM_WRAP);
    pwm_set_wrap(slice_b, PWM_WRAP);

    // Inicia com nível 0
    pwm_set_chan_level(slice_r, pwm_gpio_to_channel(RED_PIN), 0);
    pwm_set_chan_level(slice_g, pwm_gpio_to_channel(GREEN_PIN), 0);
    pwm_set_chan_level(slice_b, pwm_gpio_to_channel(BLUE_PIN), 0);

    // Habilita as slices
    pwm_set_enabled(slice_r, true);
    pwm_set_enabled(slice_g, true);
    pwm_set_enabled(slice_b, true);
}

// --- Interrupção (tratamento do BOOTSEL e do BOTÃO A com debounce) ---
void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == botaoB) {
        // BOOTSEL
        reset_usb_boot(0, 0);
    } else if (gpio == BTN_A_PIN) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_press_time > 250) {
            last_press_time = current_time;
            // avança estado (0..6)
            led_state++;
            if (led_state > 6) led_state = 0;
        }
    }
}

// Função para escrever um valor em um registro do GY-33
void gy33_write_register(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
}

// Função para ler um valor de um registro do GY-33
uint16_t gy33_read_register(uint8_t reg)
{
    uint8_t buffer[2];
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0]; // Combina os bytes em um valor de 16 bits
}

// Inicializa o GY-33
void gy33_init()
{
    gy33_write_register(ENABLE_REG, 0x03);  // Ativa o sensor (Power ON e Ativação do ADC)
    gy33_write_register(ATIME_REG, 0xF5);   // Tempo de integração (ajusta a sensibilidade) D5 => 103ms
    gy33_write_register(CONTROL_REG, 0x00); // Configuração de ganho padrão (1x) (pode ir até 60x)
}

// Lê os valores RGB e Clear do GY-33
void gy33_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
    *c = gy33_read_register(CDATA_REG);
    *r = gy33_read_register(RDATA_REG);
    *g = gy33_read_register(GDATA_REG);
    *b = gy33_read_register(BDATA_REG);
}

// Determina nome da cor lida pelo GY-33
const char* determine_color_name(uint16_t r, uint16_t g, uint16_t b) {
    // Evita divisão por zero / leituras muito baixas
    float total = (float)r + (float)g + (float)b;
    if (total < 100.0f) return "Escuro";

    // Valores absolutos (para trabalhar com sensores que saturam em diferentes ranges)
    float rf = (float)r;
    float gf = (float)g;
    float bf = (float)b;

    // Encontre máximo e mínimo
    float maxv = rf;
    if (gf > maxv) maxv = gf;
    if (bf > maxv) maxv = bf;
    float minv = rf;
    if (gf < minv) minv = gf;
    if (bf < minv) minv = bf;

    // Relações relativas ao máximo
    float r_rel = rf / maxv;
    float g_rel = gf / maxv;
    float b_rel = bf / maxv;

    // Se todos próximos -> branco
    if (minv > 0.6f * maxv) return "Branco";

    // Se dois componentes dominam e o terceiro é pequeno -> combinações
    if (r_rel > 0.8f && g_rel > 0.8f && b_rel < 0.5f) return "Amarelo";
    if (r_rel > 0.8f && b_rel > 0.8f && g_rel < 0.5f) return "Magenta";
    if (g_rel > 0.8f && b_rel > 0.8f && r_rel < 0.5f) return "Ciano";

    // Se um componente domina com folga razoável -> cor pura
    if (rf > 1.2f * gf && rf > 1.2f * bf) return "Vermelho";
    if (gf > 1.2f * rf && gf > 1.2f * bf) return "Verde";
    if (bf > 1.2f * rf && bf > 1.2f * gf) return "Azul";

    // Se não entrou em nenhum dos casos, tentar decidir por proporcionalidade
    float r_norm = rf / total;
    float g_norm = gf / total;
    float b_norm = bf / total;

    if (r_norm > 0.45f && g_norm > 0.3f && b_norm < 0.25f) return "Amarelo";
    if (g_norm > 0.45f && r_norm < 0.3f && b_norm < 0.3f) return "Verde";
    if (b_norm > 0.45f && r_norm < 0.3f && g_norm < 0.3f) return "Azul";

    return "Indefinido";
}

// Função para calcular a saturação da cor
float calculate_saturation(uint16_t r, uint16_t g, uint16_t b) {
    uint16_t max_val = r;
    if (g > max_val) max_val = g;
    if (b > max_val) max_val = b;

    uint16_t min_val = r;
    if (g < min_val) min_val = g;
    if (b < min_val) min_val = b;

    if (max_val == 0) return 0;

    return (float)(max_val - min_val) / max_val * 100.0f;
}

// Função para ajustar o brightness da matriz LED com base na luminosidade ambiente
uint8_t adjust_brightness(uint16_t lux) {
    // Mapeia o valor de lux para um valor de brightness entre 10 e 255
    // Assume que lux varia de 0 a 1000 (ajuste conforme necessário)
    if (lux < 10) return 10; // brightness mínimo
    if (lux > 1000) return 255; // brightness máximo

    return 10 + (uint8_t)((lux * 245) / 1000);
}

// Função para definir os níveis PWM dos LEDs RGB com base no estado
void set_rgb_outputs_for_state(int state)
{
    const uint16_t FULL = PWM_WRAP;         // 65535
    const uint16_t HALF = PWM_WRAP * 0.5;   // ~32767
    // ajustes específicos para aproximar cores:
    const uint16_t YEL_G = 45000;   // verde para amarelo (um pouco menor que o vermelho)
    const uint16_t MAG_B = 55000;
    const uint16_t MAG_R = 60000;
    const uint16_t CY_GB = 58000;
    const uint16_t WHITE_ALL = 52000; // branco menos saturado para não "estourar" sensor

    uint slice_r = pwm_gpio_to_slice_num(RED_PIN);
    uint slice_g = pwm_gpio_to_slice_num(GREEN_PIN);
    uint slice_b = pwm_gpio_to_slice_num(BLUE_PIN);

    uint chan_r = pwm_gpio_to_channel(RED_PIN);
    uint chan_g = pwm_gpio_to_channel(GREEN_PIN);
    uint chan_b = pwm_gpio_to_channel(BLUE_PIN);

    switch (state) {
        case 0: // Vermelho
            pwm_set_chan_level(slice_r, chan_r, FULL);
            pwm_set_chan_level(slice_g, chan_g, 0);
            pwm_set_chan_level(slice_b, chan_b, 0);
            break;
        case 1: // Verde
            pwm_set_chan_level(slice_r, chan_r, 0);
            pwm_set_chan_level(slice_g, chan_g, FULL);
            pwm_set_chan_level(slice_b, chan_b, 0);
            break;
        case 2: // Azul
            pwm_set_chan_level(slice_r, chan_r, 0);
            pwm_set_chan_level(slice_g, chan_g, 0);
            pwm_set_chan_level(slice_b, chan_b, FULL);
            break;
        case 3: // Amarelo (R forte + G intermediário)
            pwm_set_chan_level(slice_r, chan_r, FULL);
            pwm_set_chan_level(slice_g, chan_g, YEL_G);
            pwm_set_chan_level(slice_b, chan_b, 0);
            break;
        case 4: // Magenta (R + B)
            pwm_set_chan_level(slice_r, chan_r, MAG_R);
            pwm_set_chan_level(slice_g, chan_g, 0);
            pwm_set_chan_level(slice_b, chan_b, MAG_B);
            break;
        case 5: // Ciano (G + B)
            pwm_set_chan_level(slice_r, chan_r, 0);
            pwm_set_chan_level(slice_g, chan_g, CY_GB);
            pwm_set_chan_level(slice_b, chan_b, CY_GB);
            break;
        case 6: // Branco (todos com nível médio-alto)
            pwm_set_chan_level(slice_r, chan_r, WHITE_ALL);
            pwm_set_chan_level(slice_g, chan_g, WHITE_ALL);
            pwm_set_chan_level(slice_b, chan_b, WHITE_ALL);
            break;
        default: // Apaga
            pwm_set_chan_level(slice_r, chan_r, 0);
            pwm_set_chan_level(slice_g, chan_g, 0);
            pwm_set_chan_level(slice_b, chan_b, 0);
            break;
    }
}

// Função para mapear cores a frequências específicas para o buzzer
uint16_t get_color_frequency(const char* color_name) {
    if (strcmp(color_name, "Vermelho") == 0) return 262; // Dó (C4)
    if (strcmp(color_name, "Verde") == 0) return 330; // Mi (E4)
    if (strcmp(color_name, "Azul") == 0) return 392; // Sol (G4)
    if (strcmp(color_name, "Amarelo") == 0) return 294; // Ré (D4)
    if (strcmp(color_name, "Magenta") == 0) return 349; // Fá (F4)
    if (strcmp(color_name, "Ciano") == 0) return 440; // Lá (A4)
    if (strcmp(color_name, "Branco") == 0) return 523; // Dó (C5)

    // Frequência padrão para outras cores
    return 196; // Sol (G3)
}
