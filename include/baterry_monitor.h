#pragma once
#include <Arduino.h>

class BatteryMonitor {
public:
    static BatteryMonitor& getInstance() {
        static BatteryMonitor instance;
        return instance;
    }

    // Inicializa o pino e carrega o buffer inicial para evitar leitura 0 no boot
    void begin(uint8_t adcPin, float voltageDividerRatio);
    
    // Deve ser chamado no loop() principal, não bloqueia.
    void update();

    // Retorna a tensão filtrada em Volts
    float getVoltage() const;
    
    // Retorna a porcentagem considerando a curva do Li-Po (0-100%)
    uint8_t getPercentage() const;
    
    // Detecta se está na tomada baseado no comportamento da tensão
    bool isCharging() const;

private:
    BatteryMonitor() = default;

    uint8_t _pin;
    float _dividerRatio;
    
    // Timers não-bloqueantes
    uint32_t _lastSampleTime = 0;
    uint32_t _lastProcessTime = 0;
    
    // Configurações de tempo
    static constexpr uint32_t SAMPLE_INTERVAL_MS = 20;   // Amostra a cada 20ms
    static constexpr uint32_t PROCESS_INTERVAL_MS = 200; // Processa a cada 200ms
    
    // Filtro de Mediana
    static constexpr uint8_t MEDIAN_BUFFER_SIZE = 11; // Ímpar é melhor. 11 amostras = ~220ms de janela
    uint16_t _samples[MEDIAN_BUFFER_SIZE] = {0};
    uint8_t _sampleIndex = 0;
    bool _bufferFull = false;

    // Filtro EMA (0.0 a 1.0). Menor = mais suave, porém mais lento para reagir.
    static constexpr float EMA_ALPHA = 0.15f; 
    float _filteredVoltage = 0.0f;

    // Detecção de carga
    bool _isCharging = false;
    float _previousVoltage = 0.0f;

    uint16_t getMedian(uint16_t* data, uint8_t size);
    uint8_t voltageToPercentage(float voltage) const;
};