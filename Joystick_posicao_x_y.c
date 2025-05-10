#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "example_http_client_util.h"

// --- Configurações da rede e ThingSpeak ---
#define THINGSPEAK_API_KEY  "JIV4RFS126JPPYDX"
#define HOST                "api.thingspeak.com"
#define PORT                80
#define INTERVALO_MS        200

// Pinos do joystick (GPIOs ADC)
#define JOY_X_PIN           26  // ADC0
#define JOY_Y_PIN           27  // ADC1

// Inicializa ADC
void adc_setup() {
    adc_init();
    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);
}

// Lê canal ADC (0 ou 1)
uint16_t read_adc_channel(uint8_t channel) {
    adc_select_input(channel);
    return adc_read();  // 0–4095
}

int main() {
    stdio_init_all();
    adc_setup();

    // Inicializa Wi-Fi
    if (cyw43_arch_init()) return 1;
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000))
        return 1;

    printf("Conectado ao Wi-Fi! Iniciando envio para ThingSpeak...\n");

    while (1) {
        // Leitura raw 0-4095
        uint16_t raw_x = read_adc_channel(0);
        uint16_t raw_y = read_adc_channel(1);

        // Normaliza para 0–100%
        int pct_x = raw_x * 100 / 4095;
        int pct_y = raw_y * 100 / 4095;

        // Monta URL de update para campos 1 e 2
        char path[128];
        snprintf(path, sizeof(path),
                 "/update?api_key=%s&field1=%d&field2=%d",
                 THINGSPEAK_API_KEY, pct_x, pct_y);

        // Prepara requisição HTTP
        EXAMPLE_HTTP_REQUEST_T req = {
            .hostname   = HOST,
            .url        = path,
            .port       = PORT,
            .headers_fn = http_client_header_print_fn,
            .recv_fn    = http_client_receive_print_fn
        };

        // Envia e exibe resultado
        int res = http_client_request_sync(cyw43_arch_async_context(), &req);
        printf("GET http://%s%s -> %s\n", HOST, path,
               res == 0 ? "Sucesso" : "Erro");

        sleep_ms(INTERVALO_MS);
    }
    return 0;
}
