#include "orquestrador.h"
#include "../common/device_ids.h"
#include <iostream>
#include <sstream>
using namespace std;

namespace orquestrador {

vector<ConfigDisp> listaDispoDef() {
    /*
    Sobe um exemplar de cada subtipo de sensor e de atuador previsto na
    especificacao da estufa, suficiente pra demonstrar todos os
    requisitos funcionais numa unica execucao
    */
    vector<ConfigDisp> lista;

    lista.push_back(ConfigDisp("sensor", plant::subtipo_sensor::TEMPERATURA, "01"));
    lista.push_back(ConfigDisp("sensor", plant::subtipo_sensor::UMIDADE, "01"));
    lista.push_back(ConfigDisp("sensor", plant::subtipo_sensor::CO2, "01"));

    lista.push_back(ConfigDisp("atuador", plant::subtipo_atuador::AQUECEDOR, "01"));
    lista.push_back(ConfigDisp("atuador", plant::subtipo_atuador::RESFRIADOR, "01"));
    lista.push_back(ConfigDisp("atuador", plant::subtipo_atuador::IRRIGACAO, "01"));
    lista.push_back(ConfigDisp("atuador", plant::subtipo_atuador::INJETOR_CO2, "01"));

    return lista;
}

vector<ConfigDisp> parsearArgs(int argumentosTotais, char* argumentos[]) {
    vector<ConfigDisp> lista;

    // Primeiro argumento (indice 0) eh o nome do proprio binario, por isso comecamos do indice 1
    for (int indice = 1; indice < argumentosTotais; indice++) {
        string argumentoAtual = argumentos[indice];

        // Cada argumento deve ter exatamente dois separadores ":", formando tres campos
        size_t primeiroSeparador = argumentoAtual.find(':');
        size_t segundoSeparador = (primeiroSeparador == string::npos) ? string::npos
                                    : argumentoAtual.find(':', primeiroSeparador + 1);

        if (primeiroSeparador == string::npos || segundoSeparador == string::npos) {
            cerr << "Argumento mal formatado: " << argumentoAtual << "\n";
            cerr << "Use o formato tipo:subtipo:numero, exemplo sensor:TERMO:01\n";
            return vector<ConfigDisp>();
        }

        string tipoBinario = argumentoAtual.substr(0, primeiroSeparador);
        string subtipo = argumentoAtual.substr(primeiroSeparador + 1, segundoSeparador - primeiroSeparador - 1);
        string numero = argumentoAtual.substr(segundoSeparador + 1);

        if (tipoBinario != "sensor" && tipoBinario != "atuador") {
            cerr << "Tipo de binario invalido em " << argumentoAtual << ", use sensor ou atuador\n";
            return vector<ConfigDisp>();
        }

        lista.push_back(ConfigDisp(tipoBinario, subtipo, numero));
    }

    return lista;
}

}