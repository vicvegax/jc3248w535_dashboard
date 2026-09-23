#include "baterry_monitor.h"

void BatteryMonitor::begin(uint8_t adcPin, float voltageDividerRatio) {
    _pin = adcPin;
    _dividerRatio = voltageDividerRatio;
    
    // Configura o ADC para 11db (leitura até ~3.3V no pino)

    
    // Preenche o buffer inicial para evitar delays no boot do LVGL
    for (uint8_t i = 0; i < MEDIAN_BUFFER_SIZE; i++) {
        _samples[i] = analogReadMilliVolts(_pin);
        delay(2);
    }
    _bufferFull = true;
    
    // Inicializa o EMA
    uint16_t medianMv = getMedian(_samples, MEDIAN_BUFFER_SIZE);
    _filteredVoltage = (medianMv / 1000.0f) * _dividerRatio;
    _previousVoltage = _filteredVoltage;
}

void BatteryMonitor::update() {
    uint32_t now = millis();

    // 1. Fase de Amostragem (Coleta dados brutos)
    if (now - _lastSampleTime >= SAMPLE_INTERVAL_MS) {
        _lastSampleTime = now;
        
        // analogReadMilliVolts já aplica a calibração do eFuse do ESP32
        _samples[_sampleIndex] = analogReadMilliVolts(_pin);
        _sampleIndex++;
        
        if (_sampleIndex >= MEDIAN_BUFFER_SIZE) {
            _sampleIndex = 0;
            _bufferFull = true;
        }
    }

    // 2. Fase de Processamento (Aplica filtros e regras de negócio)
    if (_bufferFull && (now - _lastProcessTime >= PROCESS_INTERVAL_MS)) {
        _lastProcessTime = now;

        // Tira a mediana para matar os ruídos do Wi-Fi (quedas bruscas de 1-2 amostras)
        uint16_t medianMv = getMedian(_samples, MEDIAN_BUFFER_SIZE);
        float currentVoltage = (medianMv / 1000.0f) * _dividerRatio;

        // Aplica EMA para suavizar a curva de descida
        _filteredVoltage = (EMA_ALPHA * currentVoltage) + ((1.0f - EMA_ALPHA) * _filteredVoltage);

        // Lógica de Detecção do Carregador (Charger Spike Handling)
        // Se a tensão saltar mais de 0.15V subitamente ou estiver acima de 4.22V, o carregador entrou.
        float deltaV = _filteredVoltage - _previousVoltage;
        
        if (_filteredVoltage > 4.25f || deltaV > 0.15f) {
            _isCharging = true;
        } else if (deltaV < -0.10f || _filteredVoltage < 4.15f) {
            // Histerese simples para sair do estado de carga
            if(_filteredVoltage < 4.20f) _isCharging = false;
        }
        
        _previousVoltage = _filteredVoltage;
    }
}

float BatteryMonitor::getVoltage() const {
    return _filteredVoltage;
}

uint8_t BatteryMonitor::getPercentage() const {
    if (_isCharging && _filteredVoltage >= 4.15f) return 100;
    return voltageToPercentage(_filteredVoltage);
}

bool BatteryMonitor::isCharging() const {
    return _isCharging;
}

// Algoritmo simples de Insertion Sort para achar a Mediana
uint16_t BatteryMonitor::getMedian(uint16_t* data, uint8_t size) {
    uint16_t sorted[size];
    memcpy(sorted, data, size * sizeof(uint16_t));

    for (uint8_t i = 1; i < size; ++i) {
        uint16_t key = sorted[i];
        int8_t j = i - 1;
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            j = j - 1;
        }
        sorted[j + 1] = key;
    }
    return sorted[size / 2];
}

// Mapeamento não-linear para baterias Li-Po/Li-Ion padrão 3.7V (4.2V Max)
uint8_t BatteryMonitor::voltageToPercentage(float voltage) const {
    // Tabela de Look-up (Tensão -> Porcentagem)
    if (voltage >= 4.20f) return 100;
    if (voltage >= 4.06f) return 90;
    if (voltage >= 3.98f) return 80;
    if (voltage >= 3.92f) return 70;
    if (voltage >= 3.87f) return 60;
    if (voltage >= 3.82f) return 50;
    if (voltage >= 3.79f) return 40;
    if (voltage >= 3.77f) return 30;
    if (voltage >= 3.74f) return 20;
    if (voltage >= 3.68f) return 10;
    if (voltage >= 3.45f) return 5;
    return 0;
}