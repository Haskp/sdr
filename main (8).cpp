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



short* read_audio(const char* filename, size_t* num_samples) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Не удалось открыть файл\n");
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("file_size = %ld\n", filesize);  
    
    *num_samples = filesize / sizeof(short);
    short* buffer = (short*)malloc(filesize);
    if (!buffer) {
        printf("Не удалось выделить память\n");
        fclose(file);
        return NULL;
    }


    fread(buffer, sizeof(short), *num_samples, file);
    fclose(file);
    return buffer;
}

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
    size_t audio_samples = 0;
    short* audio_data = read_audio("../dev/audio.pcm", &audio_samples);
    if (!audio_data) {
        printf("Failed to load audio data\n");
        return -1;
    }
    
    printf("Loaded %zu audio samples\n", audio_samples);
    static std::vector <int> vectores (10,1);
    // Инициализация аргументов устройства
    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "plutosdr");        // Говорим какой тип устройства 

    // Проверка доступности USB соединения (упрощенная версия)
    int use_usb = 1; // 1 - использовать USB, 0 - использовать IP
    
    if (use_usb) {
        SoapySDRKwargs_set(&args, "uri", "usb:1.12.5");           // Способ обмена сэмплами (USB)
    } else {
        SoapySDRKwargs_set(&args, "uri", "ip:192.168.2.1"); // Или по IP-адресу
    }
    
    SoapySDRKwargs_set(&args, "direct", "1");    
    SoapySDRKwargs_set(&args, "timestamp_every", "1920");   // Использование временных меток
    SoapySDRKwargs_set(&args, "loopback", "0");             // Режим работы (0 - нормальный, 1 - петля)

    // Инициализация устройства
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);
    
    if (sdr == NULL) {
        printf("Failed to create SDR device\n");
        free(audio_data);
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
        free(audio_data);
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

    // Заполнение tx_buff значениями сэмплов: первые 16 бит - I, вторые 16 бит - Q
    for (int i = 0; i < 2 * tx_mtu; i += 2) {
        int si =i/2;
        int per =tx_mtu/4;
        if (si<per){

            // Простой тестовый сигнал
            tx_buff[i ] = 10000; // I
            tx_buff[i+1] = 10000; // Q
        } else if (si < 2*per) {
            tx_buff[i ] = 10000; // I
            tx_buff[i+1] = -10000; // Q
        } else if (si < 3*per) {
            tx_buff[i ] = -10000; // I
            tx_buff[i+1] = -10000; // Q
        } else {
            tx_buff[i ] = -10000; // I
            tx_buff[i+1] = 10000; // Q
        }


    }
// Переменные для передачи аудиоданных
size_t audio_position = 0;
size_t samples_to_transmit = audio_samples;
long long last_time = 0;
size_t count_buff = audio_samples/(1920*2);
long timeoutUs = 4000000;

FILE *file = fopen("txdata.pcm", "w");
FILE *file2 = fopen("file.pcm", "w");

// Основной цикл обработки - передаем пока есть данные
for (size_t buffer_num = 0; buffer_num < count_buff; buffer_num++) {
    void *rx_buffs[] = {rx_buffer};
    int flags;
    long long timeNs;
    
    // Читаем RX
    int sr = SoapySDRDevice_readStream(sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);
    if (file && sr > 0) fwrite(rx_buffer, sizeof(int16_t), 2 * tx_mtu, file);
    
     
        printf("Buffer: %zu - Samples: %i, Time: %lld\n", buffer_num, sr, timeNs);
        last_time = timeNs;

        // Заполняем TX буфер аудиоданными
        for (size_t i=0; i < 2 * tx_mtu; i += 2) {
            
                tx_buff[i] = audio_data[audio_position];//I
                tx_buff[i + 1] = audio_data[audio_position+1]; // Q
                audio_position+=2;
        
        
        }
        if (file2 && sr > 0) fwrite(tx_buff, sizeof(int16_t), 2 * tx_mtu, file2);


        // Отправляем данные
        void *tx_buffs[] = {tx_buff};

        int tx_flags = SOAPY_SDR_HAS_TIME;
        long long tx_time = timeNs + (4 * 1000 * 1000);
        int st = SoapySDRDevice_writeStream(sdr, txStream, tx_buffs, tx_mtu, &tx_flags, tx_time, timeoutUs);
        
        printf("TX: %d samples, progress: %zu/%zu\n", st, audio_position, samples_to_transmit);
    
}

fclose(file);
printf("Audio transmission completed! Total samples: %zu\n", samples_to_transmit);   

 
fclose(file);

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
