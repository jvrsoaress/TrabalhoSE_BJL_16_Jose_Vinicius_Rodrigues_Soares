/**
 * Este código integra um sensor de cor (gy-33) e um sensor de luz (gy-302)
 * para controlar uma matriz de leds 5x5 (ws2812) e um buzzer de forma interativa.
 * Ademais, possui um menu de 3 telas e atende a todos os requisitos da atividade proposta.
 */

// ==========================================================
// BIBLIOTECAS 
// ==========================================================
#include <stdio.h>                  // biblioteca padrão para entrada/saída 
#include <string.h>                 // biblioteca para manipulação de strings 
#include <stdlib.h>                 // biblioteca para funções gerais 
#include "pico/stdlib.h"            // biblioteca principal do sdk do raspberry pi pico
#include "hardware/i2c.h"           // controle da comunicação i2c
#include "hardware/gpio.h"          // controle dos pinos de entrada/saída
#include "hardware/pio.h"           // controle do pio para a matriz de leds
#include "hardware/pwm.h"           // controle do pwm para o buzzer
#include "ssd1306.h"                // biblioteca para o display oled ssd1306
#include "font.h"                   // fonte para o display oled
#include "bh1750_light_sensor.h"    // biblioteca para o sensor de luz bh1750
#include "generated/ws2812.pio.h"   // arquivo para a matriz ws2812

// ==========================================================
// DEFINIÇÕES DE PINOS E CONSTANTES
// ==========================================================
#define BOTAO_A 5                   // pino gpio para o botão a
#define BOTAO_B 6                   // pino gpio para o botão b
#define BOTAO_JOYSTICK 22           // pino gpio para o botão do joystick
#define PINO_SDA_SENSORES 0         // pino sda para a comunicação i2c com os sensores
#define PINO_SCL_SENSORES 1         // pino scl para a comunicação i2c com os sensores
#define I2C_PORTA_SENSORES i2c0     // porta i2c0 para os sensores
#define ENDERECO_GY33 0x29          // endereço i2c do sensor de cor gy-33
#define PINO_SDA_DISPLAY 14         // pino sda para a comunicação i2c com o display
#define PINO_SCL_DISPLAY 15         // pino scl para a comunicação i2c com o display
#define I2C_PORTA_DISPLAY i2c1      // porta i2c1 para o display
#define ENDERECO_DISPLAY 0x3C       // endereço i2c do display oled
#define PINO_BUZZER 10              // pino gpio para o buzzer
#define PINO_WS2812 7               // pino gpio para a matriz de leds ws2812
#define NUMERO_PIXELS 25            // número total de leds na matriz 

// registradores do sensor gy-33 (tcs34725)
#define REG_ENABLE 0x80       // registrador para habilitar o sensor
#define REG_ATIME 0x81        // registrador para o tempo de integração do adc
#define REG_CONTROL 0x8F      // registrador de controle (ganho)
#define REG_CDATA 0x94        // registrador do valor de luz (clear)
#define REG_RDATA 0x96        // registrador do valor de vermelho (red)
#define REG_GDATA 0x98        // registrador do valor de verde (green)
#define REG_BDATA 0x9A        // registrador do valor de azul (blue)

// ==========================================================
// VARIÁVEIS GLOBAIS

// enum para definir os estados do sistema
typedef enum {
    TELA_MENU,
    TELA_STATUS,
    TELA_VALORES
} estado_tela_t;

volatile estado_tela_t tela_atual = TELA_MENU;      // variável para controlar a tela ativa no display
volatile uint32_t ultimo_aperto = 0;                // armazena o tempo do último aperto de botão para o debounce

// ==========================================================
// FUNÇÕES DO PROJETO
// ==========================================================

// Normaliza um valor bruto do sensor para a escala 0-255
// raw_value: valor bruto lido do sensor (0 a 300)
// return: valor normalizado (0 a 255)
uint8_t normalizar_cor(uint16_t raw_value) {
    // Considera o intervalo máximo do sensor (0 a 300)
    const uint16_t raw_max = 255;
    float normalized = ((float)raw_value / raw_max) * 255.0;
    
    // Limita o valor entre 0 e 255
    if (normalized < 0) normalized = 0;
    if (normalized > 255) normalized = 255;
    
    return (uint8_t)normalized;
}

// Escreve um valor em um registrador específico do sensor gy-33 via i2c
void gy33_escrever_reg(uint8_t reg, uint8_t valor) {
    uint8_t buffer[2] = {reg, valor};
    i2c_write_blocking(I2C_PORTA_SENSORES, ENDERECO_GY33, buffer, 2, false);
}

// Lê um valor de 16 bits de um registrador específico do sensor gy-33
uint16_t gy33_ler_reg(uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(I2C_PORTA_SENSORES, ENDERECO_GY33, &reg, 1, true);
    i2c_read_blocking(I2C_PORTA_SENSORES, ENDERECO_GY33, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0];
}

// Inicializa o sensor de cor gy-33 com configurações padrão
void gy33_init() {
    gy33_escrever_reg(REG_ENABLE, 0x03);
    gy33_escrever_reg(REG_ATIME, 0xD5);
    gy33_escrever_reg(REG_CONTROL, 0x00);
}

// Lê os valores de cor (r, g, b) e de luz total (c) do sensor gy-33
void gy33_ler_cor(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c) {
    *c = gy33_ler_reg(REG_CDATA);
    *r = gy33_ler_reg(REG_RDATA);
    *g = gy33_ler_reg(REG_GDATA);
    *b = gy33_ler_reg(REG_BDATA);
}

// Envia os dados de um pixel para a matriz de leds ws2812 via pio
static inline void enviar_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Converte valores de 8 bits de r, g, b para o formato de 32 bits grb da matriz
static inline uint32_t converter_rgb_para_32bit(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

// Define a cor de todos os pixels da matriz de leds
void definir_cor_matriz(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t cor = converter_rgb_para_32bit(r, g, b);
    for (int i = 0; i < NUMERO_PIXELS; i++) {
        enviar_pixel(cor);
    }
}

// Inicializa o pino do buzzer para operar com pwm
void pwm_init_buzzer(uint pino, uint freq) {
    gpio_set_function(pino, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pino);
    float div = (float)clock_get_hz(clk_sys) / (1000.0f * freq);
    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, 1000);
    pwm_set_enabled(slice_num, true);
}

// Liga ou desliga o buzzer
void apito_buzzer(bool ligar) {
    if (ligar) {
        pwm_set_gpio_level(PINO_BUZZER, 50);
    } else {
        pwm_set_gpio_level(PINO_BUZZER, 0);
    }
}

// Analisa os valores r, g, b e retorna o nome da cor predominante
const char* obter_nome_da_cor(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t limiar_baixo = 20;  // Ajustado para valores normalizados (0-255)
    uint8_t limiar_alto = 200;  // Ajustado para valores normalizados (0-255)
    uint8_t limiar_rosa = 40;   // Limiar para G e B na cor rosa (baseado na dica: > 35)
    
    // Identifica a cor com maior, menor e intermediária intensidade
    uint8_t max_val, min_val, mid_val;
    char max_cor, min_cor, mid_cor;
    
    if (r >= g && r >= b) {
        max_val = r;
        max_cor = 'r';
        if (g >= b) {
            mid_val = g;
            mid_cor = 'g';
            min_val = b;
            min_cor = 'b';
        } else {
            mid_val = b;
            mid_cor = 'b';
            min_val = g;
            min_cor = 'g';
        }
    } else if (g >= r && g >= b) {
        max_val = g;
        max_cor = 'g';
        if (r >= b) {
            mid_val = r;
            mid_cor = 'r';
            min_val = b;
            min_cor = 'b';
        } else {
            mid_val = b;
            mid_cor = 'b';
            min_val = r;
            min_cor = 'r';
        }
    } else {
        max_val = b;
        max_cor = 'b';
        if (r >= g) {
            mid_val = r;
            mid_cor = 'r';
            min_val = g;
            min_cor = 'g';
        } else {
            mid_val = g;
            mid_cor = 'g';
            min_val = r;
            min_cor = 'r';
        }
    }
    
    // Aplica fatores de correção
    float r_ajustado = r;
    float g_ajustado = g;
    float b_ajustado = b;
    
    if (max_cor == 'r') r_ajustado *= 1.1;
    else if (max_cor == 'g') g_ajustado *= 1.1;
    else if (max_cor == 'b') b_ajustado *= 1.1;
    
    if (min_cor == 'r') r_ajustado *= 0.9;
    else if (min_cor == 'g') g_ajustado *= 0.9;
    else if (min_cor == 'b') b_ajustado *= 0.9;
    
    // Limita os valores ajustados a 255
    if (r_ajustado > 255) r_ajustado = 255;
    if (g_ajustado > 255) g_ajustado = 255;
    if (b_ajustado > 255) b_ajustado = 255;
    if (r_ajustado < 0) r_ajustado = 0;
    if (g_ajustado < 0) g_ajustado = 0;
    if (b_ajustado < 0) b_ajustado = 0;
    
    // Usa valores ajustados para identificação da cor
    uint8_t r_final = (uint8_t)r_ajustado;
    uint8_t g_final = (uint8_t)g_ajustado;
    uint8_t b_final = (uint8_t)b_ajustado;
    
    // Verifica a cor rosa antes de vermelho
    if (r_final > g_final * 1.5 && r_final > b_final * 1.5 && r_final > limiar_baixo &&
        (g_final > limiar_rosa || b_final > limiar_rosa)) return "Rosa";
    
    // Prioriza a detecção de cores antes de casos especiais
    if (r_final > g_final * 1.8 && r_final > b_final * 1.8 && r_final > limiar_baixo &&
        g_final <= limiar_rosa && b_final <= limiar_rosa) return "Vermelho";
    if (g_final > r_final * 1.8 && g_final > b_final * 1.8 && g_final > limiar_baixo) return "Verde";
    if (b_final > r_final * 1.8 && b_final > g_final * 1.8 && b_final > limiar_baixo) return "Azul";
    if (r_final > b_final * 2.0 && g_final > b_final * 2.0 && abs(r_final - g_final) < 50 && r_final > limiar_baixo) return "Amarelo";
    if (g_final > r_final * 2.0 && b_final > r_final * 2.0 && abs(g_final - b_final) < 50 && g_final > limiar_baixo) return "Ciano";
    if (r_final > g_final * 2.0 && b_final > g_final * 2.0 && abs(r_final - b_final) < 50 && r_final > limiar_baixo) return "Magenta";

    // Casos especiais
    if (r_final > limiar_alto && g_final > limiar_alto && b_final > limiar_alto) return "Branco";
    if (r_final < limiar_baixo && g_final < limiar_baixo && b_final < limiar_baixo) return "Escuro";
    
    return "Indefinido";
}

// Função de callback chamada na interrupção dos pinos gpio dos botões
void callback_botoes(uint pino, uint32_t eventos) {
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    if (tempo_atual - ultimo_aperto < 250) return;
    ultimo_aperto = tempo_atual;

    if (tela_atual == TELA_MENU) {
        if (pino == BOTAO_A) tela_atual = TELA_STATUS;
        if (pino == BOTAO_B) tela_atual = TELA_VALORES;
    } else {
        if (pino == BOTAO_JOYSTICK) tela_atual = TELA_MENU;
    }
}

// ==========================================================
// FUNÇÃO PRINCIPAL (MAIN)
// ==========================================================
int main() {
    // Inicializa a comunicação serial para debug via usb
    stdio_init_all();
    sleep_ms(2000); // Aguarda para estabilizar

    // INICIALIZAÇÃO DOS PERIFÉRICOS 
    
    // Inicializa i2c e display oled
    i2c_init(I2C_PORTA_DISPLAY, 400 * 1000);
    gpio_set_function(PINO_SDA_DISPLAY, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL_DISPLAY, GPIO_FUNC_I2C);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_DISPLAY, I2C_PORTA_DISPLAY);
    ssd1306_config(&ssd);

    // Inicializa i2c e sensores
    i2c_init(I2C_PORTA_SENSORES, 100 * 1000);
    gpio_set_function(PINO_SDA_SENSORES, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL_SENSORES, GPIO_FUNC_I2C);
    bh1750_power_on(I2C_PORTA_SENSORES);
    gy33_init();

    // Inicializa buzzer com pwm
    pwm_init_buzzer(PINO_BUZZER, 1500);

    // Inicializa matriz de leds com pio
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, PINO_WS2812, 800000, false);
    
    // Inicializa os botões do menu
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);
    
    // Habilita as interrupções para os botões
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &callback_botoes);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &callback_botoes);
    gpio_set_irq_enabled_with_callback(BOTAO_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &callback_botoes);

    // LOOP INFINITO 
    char texto_buffer[30];
    
    while (1) {
        // ETAPA 1: LEITURA DOS SENSORES 
        uint16_t lux = bh1750_read_measurement(I2C_PORTA_SENSORES);
        uint16_t r, g, b, c;
        gy33_ler_cor(&r, &g, &b, &c);
        
        // Normaliza os valores RGB para a escala 0-255
        uint8_t r8 = normalizar_cor(r);
        uint8_t g8 = normalizar_cor(g);
        uint8_t b8 = normalizar_cor(b);
        
        // ETAPA 2: LÓGICA DO SISTEMA 
        const char* nome_cor = obter_nome_da_cor(r8, g8, b8);
        bool alerta_ativo = (lux < 25) || (strcmp(nome_cor, "Vermelho") == 0 && r8 > 100);

        // Controla o buzzer de forma intermitente
        if (alerta_ativo) {
            if ((to_ms_since_boot(get_absolute_time()) % 2000) < 200) apito_buzzer(true);
            else apito_buzzer(false);
        } else {
            apito_buzzer(false);
        }

        // LÓGICA DA MATRIZ DE LEDS 
        // Calcula o brilho baseado no sensor de luz
        float brilho = lux / 1000.0f;
        if (lux < 5) brilho = 0.0f; // Apaga a matriz 
        else if (brilho > 1000.0f) brilho = 1.0f;
        
        // Define a cor final da matriz
        definir_cor_matriz((uint8_t)(r8 * brilho), (uint8_t)(g8 * brilho), (uint8_t)(b8 * brilho));

        // ETAPA 3: ATUALIZAÇÃO DO DISPLAY COM MENU 
        ssd1306_fill(&ssd, false);
        switch (tela_atual) {
            case TELA_MENU:
                ssd1306_draw_string(&ssd, "MENU INICIAL", 16, 4);
                ssd1306_draw_string(&ssd, "A: Tela Status", 4, 24);
                ssd1306_draw_string(&ssd, "B: Tela Valores", 4, 40);
                printf("--- TELA MENU ---\n");
                printf("MENU INICIAL\n");
                printf("A: Tela Status\n");
                printf("B: Tela Valores\n");
                printf("-----------------\n");
                break;

            case TELA_STATUS: {
                char estado_sistema[20];
                if (alerta_ativo) strcpy(estado_sistema, lux < 50 ? "Luz Baixa" : "Alerta Cor");
                else strcpy(estado_sistema, "Normal");

                ssd1306_draw_string(&ssd, "- STATUS -", 24, 2);
                sprintf(texto_buffer, "Luz: %d Lux", lux);
                ssd1306_draw_string(&ssd, texto_buffer, 4, 16);
                sprintf(texto_buffer, "Cor: %s", nome_cor);
                ssd1306_draw_string(&ssd, texto_buffer, 4, 30);
                sprintf(texto_buffer, "Estado: %s", estado_sistema);
                ssd1306_draw_string(&ssd, texto_buffer, 4, 44);
                printf("--- TELA STATUS ---\n");
                printf("- STATUS -\n");
                printf("Luz: %d Lux\n", lux);
                printf("Cor: %s\n", nome_cor);
                printf("Estado: %s\n", estado_sistema);
                printf("-------------------\n");
                break;
            }
            case TELA_VALORES:
                ssd1306_draw_string(&ssd, "- VALORES RGB -", 4, 2);
                sprintf(texto_buffer, "Vermelho: %d", r8);
                ssd1306_draw_string(&ssd, texto_buffer, 4, 20);
                sprintf(texto_buffer, "Verde:    %d", g8);
                ssd1306_draw_string(&ssd, texto_buffer, 4, 34);
                sprintf(texto_buffer, "Azul:     %d", b8);
                ssd1306_draw_string(&ssd, texto_buffer, 4, 48);
                printf("--- TELA VALORES ---\n");
                printf("- VALORES RGB -\n");
                printf("Vermelho: %d\n", r8);
                printf("Verde:    %d\n", g8);
                printf("Azul:     %d\n", b8);
                printf("--------------------\n");
                break;
        }
        ssd1306_send_data(&ssd);
        sleep_ms(100);
    }
}
