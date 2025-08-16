# Sistema de Detecção de Cores e Luminosidade

Um sistema embarcado para Raspberry Pi Pico que detecta cores, mede luminosidade e fornece feedback visual e sonoro em tempo real.

## 📝 Descrição do Projeto

Este projeto integra sensores de cor (GY-33) e luminosidade (BH1750) em um sistema embarcado que processa e exibe informações sobre cores detectadas e níveis de luz ambiente. O sistema utiliza um display OLED para mostrar dados, uma matriz de LEDs para representação visual da cor, e alertas sonoros para indicar mudanças de cor e baixa luminosidade.

## ✨ Funcionalidades

- **Detecção de Cores**: Identifica cores como vermelho, verde, azul, amarelo, magenta, ciano e branco
- **Medição de Luminosidade**: Leitura em tempo real de níveis de luz em lux
- **Display OLED**: Mostra a cor detectada, valores RGB, saturação e nível de luminosidade
- **Matriz de LEDs**: Exibe visualmente a cor detectada, com brilho ajustado automaticamente
- **Feedback Sonoro**:
  - Tons musicais específicos para cada cor detectada
  - Alertas sonoros quando a luminosidade está abaixo do limite seguro
- **Botões de Controle**:
  - Botão A: Cicla entre diferentes cores do LED RGB
  - Botão B (BOOTSEL): Reinicia o dispositivo em modo de boot USB

## 🔧 Requisitos

### Hardware
- Raspberry Pi Pico
- Sensor de cor GY-33
- Sensor de luminosidade BH1750
- Display OLED SSD1306
- Matriz de LEDs
- Buzzer (2 unidades)
- LEDs RGB
- Botões (2 unidades)
- Cabos e resistores

### Software
- Raspberry Pi Pico SDK
- Bibliotecas:
  - `pico/stdlib.h`
  - `hardware/i2c.h`
  - `hardware/gpio.h`
  - `hardware/pwm.h`
  - `hardware/pio.h`
  - `bh1750_light_sensor.h`
  - `ssd1306.h`
  - `font.h`
  - Bibliotecas personalizadas: `matriz.h` e `buzzer.h`

## 🚀 Como Rodar

1. **Configuração do Ambiente**
   ```bash
   # Clone o repositório Pico SDK
   git clone https://github.com/LeonardoBonifacio/SistemaCoresLuz
   ```

2. **Compilação do Projeto**
   ```bash
   mkdir build
   cd build
   cmake -G Ninja ..
   ninja
   ```

3. **Programação do Pico**
   - Conecte o Raspberry Pi Pico ao computador enquanto pressiona o botão BOOTSEL
   - Copie o arquivo `.uf2` gerado para o volume que aparecer
   - O Pico reiniciará automaticamente com o programa carregado

4. **Conexões de Hardware**
   - GY-33: Conecte aos pinos SDA (0) e SCL (1)
   - BH1750: Conecte ao barramento I2C0
   - Display OLED: Conecte aos pinos SDA (14) e SCL (15)
   - Botão A: Pino 5
   - Botão B (BOOTSEL): Pino 6
   - LEDs RGB: Pinos 13 (R), 11 (G), 12 (B)
   - Matriz de LEDs e Buzzer: Conforme definido nas bibliotecas específicas

## 📹 Link para o Vídeo

[Assista à demonstração do projeto](https://www.youtube.com/seu_link_aqui)

## 👥 Membros do Projeto

- [Matheus Santos Silva](https://github.com/matheusssilva991)

