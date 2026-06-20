#include "sensor_registry.h"
using namespace std;

namespace plant {

void RegistroSensores::registrarSensor(const string& idSensor) {
    lock_guard<mutex> guarda(mutexAcesso);
    // O operator[] do map cria uma entrada nova com valores padrao se ainda nao existir
    // Se ja existir, so deixa o estado atual intacto (nao sobrescreve leitura/limites)
    if (sensores.find(idSensor) == sensores.end()) {
        sensores[idSensor] = EstadoSensor();
    }
}

bool RegistroSensores::sensorExiste(const string& idSensor) {
    lock_guard<mutex> guarda(mutexAcesso);
    return sensores.find(idSensor) != sensores.end();
}

void RegistroSensores::atualizarLeitura(const string& idSensor, float valor) {
    lock_guard<mutex> guarda(mutexAcesso);
    sensores[idSensor].ultimoValor = valor;
}

float RegistroSensores::obterUltimoValor(const string& idSensor, bool& encontrado) {
    lock_guard<mutex> guarda(mutexAcesso);
    auto iterador = sensores.find(idSensor);
    if (iterador == sensores.end()) {
        encontrado = false;
        return 0.0f;
    }
    encontrado = true;
    return iterador->second.ultimoValor;
}

void RegistroSensores::definirLimites(const string& idSensor, float minimo, float maximo) {
    lock_guard<mutex> guarda(mutexAcesso);
    sensores[idSensor].limiteMinimo = minimo;
    sensores[idSensor].limiteMaximo = maximo;
    sensores[idSensor].limitesConfigurados = true;
}

bool RegistroSensores::obterEstado(const string& idSensor, EstadoSensor& estadoSaida) {
    lock_guard<mutex> guarda(mutexAcesso);
    auto iterador = sensores.find(idSensor);
    if (iterador == sensores.end()) {
        return false;
    }
    estadoSaida = iterador->second;
    return true;
}

map<string, EstadoSensor> RegistroSensores::obterTodosOsSensores() {
    lock_guard<mutex> guarda(mutexAcesso);
    // Retorna uma copia pra quem chamou poder iterar sem segurar o mutex por muito tempo
    return sensores;
}

} 