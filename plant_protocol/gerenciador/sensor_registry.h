#ifndef SENSOR_REGISTRY_H
#define SENSOR_REGISTRY_H

#include <string>
#include <map>
#include <mutex>
using namespace std;

/*
Esse modulo guarda o estado de todos os sensores conhecidos pelo
Gerenciador: ultima leitura recebida e limites MIN/MAX configurados
pelo Cliente pra cada sensor

Como o Gerenciador roda uma thread por conexao, varias threads (sensor
enviando DATA, cliente pedindo CONFIG ou REQ_DATA) podem acessar esse
registro ao mesmo tempo
Por isso toda leitura e escrita eh protegida por mutex, garantindo que
o acesso concorrente nao corrompa os dados
*/

namespace plant {

// Guarda o ultimo valor lido e os limites configurados pra um sensor especifico
struct EstadoSensor {
    float ultimoValor;
    float limiteMinimo;
    float limiteMaximo;
    bool limitesConfigurados;

    EstadoSensor() : ultimoValor(0.0f), limiteMinimo(0.0f), limiteMaximo(0.0f), limitesConfigurados(false) {}
};

class RegistroSensores {
public:
    // Registra um sensor novo no sistema, caso ainda nao exista
    void registrarSensor(const string& idSensor);

    // Verifica se um sensor com esse id ja foi registrado
    bool sensorExiste(const string& idSensor);

    // Atualiza o ultimo valor lido de um sensor
    void atualizarLeitura(const string& idSensor, float valor);

    // Retorna o ultimo valor lido, indicando sucesso por meio de encontrado
    float obterUltimoValor(const string& idSensor, bool& encontrado);

    // Define os limites minimo e maximo de operacao de um sensor
    void definirLimites(const string& idSensor, float minimo, float maximo);

    // Retorna o estado completo do sensor, incluindo limites configurados
    bool obterEstado(const string& idSensor, EstadoSensor& estadoSaida);

    // Retorna uma copia de todos os ids de sensores registrados, util pra REQ_DATA com ALVO ALL
    map<string, EstadoSensor> obterTodosOsSensores();

private:
    map<string, EstadoSensor> sensores;
    mutex mutexAcesso;
};

} // namespace plant

#endif