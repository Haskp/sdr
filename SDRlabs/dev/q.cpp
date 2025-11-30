#include <SoapySDR/Device.h>   // Инициализация устройства
#include <SoapySDR/Formats.h>  // Типы данных, используемых для записи сэмплов
#include <SoapySDR/Errors.h>   // Коды ошибок
#include <stdio.h>             // printf
#include <stdlib.h>            // free
#include <stdint.h>            // Стандартные целочисленные типы
#include <complex.h>           // Комплексные числа
#include <string.h>            // memset
#include <unistd.h>            // usleep
#include <vector> 
#include <iostream>



 std::vector<int> convolve(const std::vector<int>& x, const std::vector<int>& h) {
    int N = x.size();
    int M = h.size();
    int len_y = N + M - 1;
    
    // Создаем результирующий вектор и инициализируем нулями
    std::vector<int> y(len_y, 0.0);

    // Вычисление свертки по формуле
    for (int n = 0; n < len_y; n++) {
        for (int k = 0; k < M; k++) {
            // Условие для учета границ массивов x и h
            // x[n-k] * h[k]
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
    SoapySDRKwargs_set(&args, "driver", "plutosdr");        // Говорим какой тип устройства 

    // Проверка доступности USB соединения (упрощенная версия)
    int use_usb = 1; // 1 - использовать USB, 0 - использовать IP
    
    if (use_usb) {
        SoapySDRKwargs_set(&args, "uri", "usb:1.4.5");           // Способ обмена сэмплами (USB)
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.10"); // Или по IP-адресу
    }
    
    SoapySDRKwargs_set(&args, "direct", "1");    
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");   // Использование временных меток
    SoapySDRKwargs_set(&args, "loopback", "0");             // Режим работы (0 - нормальный, 1 - петля)

    // Инициализация устройства
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);
    FILE *file = fopen("txdata.pcm", "wb");
    
    if (sdr == NULL) {
        printf("Failed to create SDR device\n");
        //free();
        return -1;
    }
    //bits
    std::vector<int> bits = {1,0,1,1,0,0,1,0,1,1,0,1,0,0,1,1,0,1,0,0};
   
    //bpsk
    std::vector<int> bpskI;
    std::vector<int> bpskQ;
    for (int bit : bits){
        printf("bit: %d ", bit ); 
        if (bit == 1) {
            bpskI.push_back(1);
            bpskQ.push_back(0);
        } else {
            bpskI.push_back(-1);
        }

     
    }

        printf("bpskI");
    for (int i = 0; i< bpskI.size(); i++){
    printf("%d ", bpskI[i] );
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
for (int i = 0; i< samplI.size(); i++){
    //printf("%d", samplI[i] );
}
std::vector<int> filter = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    // Вызов функции convolve
    std::vector<int> convI = convolve(samplI, filter);
    std::vector<int> convQ = convolve(samplQ, filter);

    // Вывод результата для I
    std::cout << "\n I (convI): ";
    for (int v : convI) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    
    return 0;
    

    int max_val = 0;
    for (int v : convI) {
        if (abs(v) > max_val) max_val = abs(v);
    }
    for (int v : convQ) {
        if (abs(v) > max_val) max_val = abs(v);
    }

    double scale_factor = 10000.0 / (max_val > 0 ? max_val : 1);


    // Настройка параметров
    double sample_rate = 1e6;
    double carrier_freq = 800e6;

    // Параметры RX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, carrier_freq, NULL);

    // Параметры TX части
    SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_TX, 0, carrier_freq, NULL);

    // Инициализация количества каналов RX/TX (в AdalmPluto он один, нулевой)
    size_t channel = 0;
    size_t channel_count = 1;

    // Настройки усилителей на RX/TX
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, channel, 50.0);  // Чувствительность приемника
    SoapySDRDevice_setGain(sdr, SOAPY_SDR_TX, channel, -10.0); // Усиление передатчика

    // Формирование потоков для передачи и приема сэмплов
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, &channel, channel_count, NULL);
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, &channel, channel_count, NULL);
    
    if (rxStream == NULL || txStream == NULL) {
        printf("Failed to setup streams\n");
        SoapySDRDevice_unmake(sdr);
        //free(audio_data);
        return -1;
    }

    // Активация потоков
    int result = SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); // start streaming
    if (result != 0) {
        printf("Failed to activate RX stream: %d\n", result);
    }
    
    result = SoapySDRDevice_activateStream(sdr, txStream, 0, 0, 0); // start streaming
    if (result != 0) {
        printf("Failed to activate TX stream: %d\n", result);
    }

    // Получение MTU (Maximum Transmission Unit) - размер буферов
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(sdr, rxStream);
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(sdr, txStream);
    
    printf("RX MTU: %zu, TX MTU: %zu\n", rx_mtu, tx_mtu);

    // Выделяем память под буферы RX и TX
    int16_t *tx_buff = (int16_t*)malloc(2 * tx_mtu * sizeof(int16_t));
    int16_t *rx_buffer = (int16_t*)malloc(2 * rx_mtu * sizeof(int16_t));
    
    if (tx_buff == NULL || rx_buffer == NULL) {
        printf("Memory allocation failed\n");
        free(tx_buff);
        free(rx_buffer);
        SoapySDRDevice_unmake(sdr);
        return -1;
    }
FILE *file = fopen("txdata.pcm", "w");
FILE *file2 = fopen("file.pcm", "w");

    // ЗАПИСЬ BITS В TX_BUFF - основная задача
    size_t total_samples = convI.size();
    size_t samples_written = 0;
    
    while (samples_written < total_samples) {
        size_t samples_to_write = std::min(tx_mtu, total_samples - samples_written);
        
        // Заполнение tx_buff сэмплами из convI и convQ
        for (size_t i = 0; i < samples_to_write; i++) {
            size_t idx = samples_written + i;
            
            // Масштабирование и преобразование в int16_t
            int16_t I_val = (int16_t)(convI[idx] * scale_factor);
            int16_t Q_val = (int16_t)(convQ[idx] * scale_factor);
            
            tx_buff[2*i] = I_val;     // I компонент
            tx_buff[2*i + 1] = Q_val; // Q компонент
            

            }

        }


        int flags = 0;
        long long timeNs = 0;
        void *tx_buffs[] = {tx_buff};
        int tx_result = SoapySDRDevice_writeStream(sdr, txStream, tx_buffs, samples_to_write, &flags, timeNs, 1000000);
        
        if (tx_result < 0) {
            printf("Failed to write to TX stream: %d\n", tx_result);
            //break;
        }

        
        samples_written += samples_to_write;
        printf("Transmitted %zu samples, total: %zu/%zu\n", samples_to_write, samples_written, total_samples);
    
size_t audio_position = 0;
std::vector<int16_t> audio_data;
size_t audio_samples= audio_data.size();
size_t samples_to_transmit = audio_samples;
long long last_time = 0;
size_t count_buff = audio_samples/(1920*2);
long timeoutUs = 4000000;


for (size_t buffer_num = 0; buffer_num < count_buff; buffer_num++) {
    void *rx_buffs[] = {rx_buffer};
    int flags; 
    long long timeNs;

int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
    if (file && sr > 0) fwrite(rx_buffer, sizeof(int16_t), 2 * tx_mtu, file);

    if (file2 && sr > 0) fwrite(tx_buff, sizeof(int16_t), 2 * tx_mtu, file2);


}

    // Освобождение памяти
    free(tx_buff);
    free(rx_buffer);

    // Stop streaming
    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0);
    SoapySDRDevice_deactivateStream(sdr, txStream, 0, 0);

    // Shutdown the stream
    SoapySDRDevice_closeStream(sdr, rxStream);
    SoapySDRDevice_closeStream(sdr, txStream);

    // Cleanup device handle
    SoapySDRDevice_unmake(sdr);

    printf("Program completed successfully\n");
return 0;
}
