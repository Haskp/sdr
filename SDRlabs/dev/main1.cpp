#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <SoapySDR/Errors.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <string.h>
#include <vector> 
#include <iostream>
#include <algorithm>

std::vector<int> convolve(const std::vector<int>& x, const std::vector<int>& h) {
    int N = x.size();
    int M = h.size();
    int len_y = N + M - 1;
    
    std::vector<int> y(len_y, 0);

    for (int n = 0; n < len_y; n++) {
        for (int k = 0; k < M; k++) {
            if (n >= k && (n - k) < N) {
                y[n] += x[n - k] * h[k];
            }
        }
    }
    
    return y;
}

int main() {
    // Инициализация аргументов устройства
    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");

    int use_usb = 1;
    if (use_usb) {
        SoapySDRKwargs_set(&args, "uri", "usb:1.5.5");
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.10");
    }
    
    SoapySDRKwargs_set(&args, "direct", "1");    
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");
    SoapySDRKwargs_set(&args, "loopback", "0");

    // Инициализация устройства
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);
    
    if (sdr == NULL) {
        printf("Failed to create SDR device\n");
        return -1;
    }

    // Генерация сигнала
    std::vector<int> bits = {1,0,1,1,0,0,1,0,1,1,0,1,0,0,1,1,0,1,0,0};
   
    // BPSK модуляция
    std::vector<int> bpskI;
    std::vector<int> bpskQ;
    for (int bit : bits) {
        if (bit == 1) {
            bpskI.push_back(1);
            bpskQ.push_back(0);
        } else {
            bpskI.push_back(-1);
            bpskQ.push_back(0);
        }
    }

    //printf("bpskI: ");
    for (int i = 0; i < bpskI.size(); i++) {
        //printf("%d ", bpskI[i]);
    }
    //printf("\n");

   
    
    for (int i = 0; i< bpskI.size(); i++){
    //printf("%d \n", bpskI[i] );
}
    std::vector<int> samplI;
    std::vector<int> samplQ;
    int samplesps = 9;
    for (size_t i = 0; i < bpskI.size(); i++) {
        samplI.push_back(bpskI[i]*1);

        for (int j = 0; j < samplesps; j++) {
            samplI.push_back(bpskI[i]*0);
            samplQ.push_back(bpskQ[i]*1);
        }
    }
        /*for (int i = 0; i< samplI.size(); i++){
            printf("=%d\n", samplI[i] );
        }*/

    std::vector<int> filter = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    // Свертка
    std::vector<int> convI = convolve(samplI, filter);
    std::vector<int> convQ = convolve(samplQ, filter);
    printf("%d", convI);

    // Нормализация
    int max_val = 0;
    for (int v : convI) {
        if (abs(v) > max_val) max_val = abs(v);
    }
    for (int v : convQ) {
        if (abs(v) > max_val) max_val = abs(v);
    }

    double scale_factor = 10000.0 / (max_val > 0 ? max_val : 1);

    std::vector<int16_t> audio_data;
    for (size_t i = 0; i < convI.size(); i++) {
        int16_t I_val = (int16_t)(convI[i] * scale_factor);
        int16_t Q_val = (int16_t)(convQ[i] * scale_factor);
        audio_data.push_back(I_val);
        audio_data.push_back(Q_val);
    }

    size_t audio_samples = audio_data.size();
    printf("Generated %zu audio samples\n", audio_samples);

    // Настройка SDR параметров
    double sample_rate = 1e6;
    double carrier_freq = 800e6;

    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq, NULL);
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq, NULL);

    size_t channel = 0;
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, channel, 50.0);
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, channel, -10.0);

    // Настройка потоков
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, &channel, 1, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, &channel, 1, NULL);
    
    if (rxStream == NULL || txStream == NULL) {
        printf("Failed to setup streams\n");
        SoapySDRDevice_unmake(sdr);
        return -1;
    }
    // Активация потоков
    int result = SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0);
    if (result != 0) printf("Failed to activate RX stream: %d\n", result);
    
    result = SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0);
    if (result != 0) printf("Failed to activate TX stream: %d\n", result);

    // Получение MTU
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);
    printf("RX MTU: %zu, TX MTU: %zu\n", rx_mtu, tx_mtu);

    // Выделение памяти под буферы
    int16_t *tx_buff = (int16_t*)malloc(2 * tx_mtu * sizeof(int16_t));
    int16_t *rx_buffer = (int16_t*)malloc(2 * rx_mtu * sizeof(int16_t));
    
    if (tx_buff == NULL || rx_buffer == NULL) {
        printf("Memory allocation failed\n");
        SoapySDRDevice_unmake(sdr);
        return -1;
    }

    FILE *tx_pcm_file = fopen("tx_signal.pcm", "wb");  // Передаваемый 
    FILE *rx_pcm_file = fopen("rx_signal.pcm", "wb");     // Принятый 
    
    if (!tx_pcm_file) {
    }
    if (!rx_pcm_file) {
    }

    //передача
    size_t total_samples = convI.size();
    size_t samples_written = 0;
    long timeoutUs = 1000000;
    
    printf("Starting transmission of %zu samples...\n", total_samples);
    
    while (samples_written < total_samples) {
        size_t samples_to_write = std::min(tx_mtu, total_samples - samples_written);
        
        // Заполнение tx_buff
        for (size_t i = 0; i < samples_to_write; i++) {
            size_t idx = samples_written + i;
            
            int16_t I_val = (int16_t)(convI[idx] * scale_factor);
            int16_t Q_val = (int16_t)(convQ[idx] * scale_factor);
            
            tx_buff[2*i] = I_val;
            tx_buff[2*i + 1] = Q_val;
        }

        if (tx_pcm_file) {
            fwrite(tx_buff, sizeof(int16_t), 2 * samples_to_write, tx_pcm_file);
        }

        // Передача
        int flags = 0;
        long long timeNs = 0;
        void *tx_buffs[] = {tx_buff};
        int tx_result = SoapySDRDevice_writeStream(sdr, txStream, tx_buffs, samples_to_write, &flags, timeNs, timeoutUs);
        
        if (tx_result < 0) {
            printf("Failed to write to TX stream: %d\n", tx_result);
            break;
        }
        
        samples_written += tx_result;
        printf("Transmitted %d samples, total: %zu/%zu\n", tx_result, samples_written, total_samples);
        
        // Чтение RX 
        void *rx_buffs[] = {rx_buffer};
        int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
        
        if (rx_pcm_file && sr > 0) {
            fwrite(rx_buffer, sizeof(int16_t), 2 * sr, rx_pcm_file);
        }
    }

    if (tx_pcm_file) {
        fclose(tx_pcm_file);
    }
    if (rx_pcm_file) {
        fclose(rx_pcm_file);

    }

    // Также сохраняем исходный сгенерированный сигнал в отдельный PCM файл
    FILE *original_pcm_file = fopen("original_signal.pcm", "wb");
    if (original_pcm_file) {
        fwrite(audio_data.data(), sizeof(int16_t), audio_data.size(), original_pcm_file);
        fclose(original_pcm_file);
        printf("Original signal saved to original_signal.pcm\n");
    }

    // Освобождение ресурсов
    free(tx_buff);
    free(rx_buffer);

    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);

    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_closeStream(sdr, txStream);

    SoapySDRDevice_unmake(sdr);

    printf("Program completed successfully\n");
    return 0;
}