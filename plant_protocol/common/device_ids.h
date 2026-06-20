#ifndef DEVICE_IDS_H
#define DEVICE_IDS_H

#include <string>
using namespace std;

/* Esse arquivo centraliza todas as constantes do protocolo PLANT que sao
compartilhadas entre os diferentes processos (gerenciador, sensor,
atuador e cliente) */

namespace plant {

// Versao do protocolo, usada na linha de requisicao de toda mensagem
const string VERSAO_PROTOCOLO = "1.0";

// Tipos de mensagem (linha de requisicao)
namespace tipo_mensagem {
    const string HELLO = "HELLO";
    const string ACK = "ACK";
    const string DATA = "DATA";
    const string CMD = "CMD";
    const string CONFIG = "CONFIG";
    const string REQ_DATA = "REQ_DATA";
    const string RESP_DATA = "RESP_DATA";
    const string ERR = "ERR";
}

// Tipos de dispositivo, usados como primeiro campo do ID-Dispositivo
namespace tipo_dispositivo {
    const string SENSOR = "SENS";
    const string ATUADOR = "ATU";
    const string CLIENTE = "CLI";
    const string GERENCIADOR = "GEREN";
}

// Subtipos de sensor
namespace subtipo_sensor {
    const string TEMPERATURA = "TERMO";
    const string UMIDADE = "UMI";
    const string CO2 = "CO2";
}

// Subtipos de atuador
namespace subtipo_atuador {
    const string AQUECEDOR = "AQUEC";
    const string RESFRIADOR = "RESF";
    const string INJETOR_CO2 = "INJ";
    const string IRRIGACAO = "IRR";
}

// Codigos de resposta numericos
namespace codigo_resposta {
    const int OK = 200;
    const int CRIADO = 201;
    const int REQUISICAO_INVALIDA = 400;
    const int NAO_REGISTRADO = 401;
    const int NAO_ENCONTRADO = 404;
    const int CONFLITO = 409;
    const int ERRO_INTERNO = 500;
}

/* Porta TCP padrao em que o Gerenciador escuta conexoes.
Mantida como constante pra que todos os processos clientes
(sensor, atuador, cliente) usem o mesmo valor sem precisar repeti-lo */
const int PORTA_GERENCIADOR = 8080;

// Tempo maximo, em segundos, que o Gerenciador aguarda pelo HELLO de um
// dispositivo recem conectado antes de encerrar a conexao por timeout
const int TIMEOUT_HANDSHAKE_SEGUNDOS = 5;

}

#endif