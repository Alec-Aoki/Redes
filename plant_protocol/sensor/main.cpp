#include "../common/plant_message.h"
#include "../common/socket_utils.h"
#include "../common/device_ids.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <unistd.h>
using namespace std;

/*
Ponto de entrada do processo Sensor
Esse processo representa um unico sensor da estufa (temperatura,
umidade ou CO2), recebido por argumento de linha de comando

Ex.:
  ./sensor TERMO 01
  ./sensor UMI 01
  ./sensor CO2 01

O processo conecta no Gerenciador, faz o handshake HELLO/ACK, e entra
em loop enviando DATA a cada 1 segundo com um valor simulado por
variacao aleatoria pequena em torno do valor anterior
*/

// Retorna o valor inicial de simulacao de acordo com o subtipo de sensor escolhido
static float valorInicialParaSubtipo(const string& subtipo) {
    if (subtipo == plant::subtipo_sensor::TEMPERATURA) {
        return 22.0f;
    } else if (subtipo == plant::subtipo_sensor::UMIDADE) {
        return 50.0f;
    } else if (subtipo == plant::subtipo_sensor::CO2) {
        return 600.0f;
    }
    return 0.0f;
}

/*
Gera uma variacao aleatoria pequena, entre -0.5 e 0.5
usada pra simular a leitura do sensor de forma simples, sem depender
de nenhum padrao mais complexo como funcao senoidal ou arquivo externo
*/
static float gerarVariacaoAleatoria() {
    int passoAleatorio = rand() % 101; // Numero inteiro de 0 a 100
    float variacao = (passoAleatorio / 100.0f) - 0.5f; // Mapeia para -0.5 ate 0.5
    return variacao;
}

int main(int argumentosTotais, char* argumentos[]) {
    // Forca o flush do buffer de saida apos cada cout, mesma justificativa do gerenciador
    cout.setf(ios::unitbuf);

    if (argumentosTotais != 3) {
        cerr << "Uso esperado: ./sensor SUBTIPO NUMERO\n";
        cerr << "Exemplo: ./sensor TERMO 01\n";
        return 1;
    }

    string subtipo = plant::toMaiusculas(argumentos[1]);
    string numero = argumentos[2];

    if (subtipo != plant::subtipo_sensor::TEMPERATURA && subtipo != plant::subtipo_sensor::UMIDADE && subtipo != plant::subtipo_sensor::CO2) {
        cerr << "Subtipo invalido, use TERMO, UMI ou CO2\n";
        return 1;
    }

    string idSensor = plant::tipo_dispositivo::SENSOR + "-" + subtipo + "-" + numero;

    // Semente do gerador aleatorio baseada no relogio, pra cada instancia simular valores diferentes
    srand(static_cast<unsigned int>(time(nullptr)) + getpid());

    int descritorSocket = plant::conectarAoServidor("127.0.0.1", plant::PORTA_GERENCIADOR);
    if (descritorSocket < 0) {
        cerr << "Nao foi possivel conectar ao gerenciador\n";
        return 1;
    }

    // Etapa de handshake, conforme especificado no protocolo PLANT
    string mensagemHello = plant::construirMensagem(plant::tipo_mensagem::HELLO, idSensor, "");
    plant::enviarTexto(descritorSocket, mensagemHello);
    cout << "[" << idSensor << "] HELLO enviado\n";

    plant::Mensagem respostaHandshake;
    if (!plant::receberMensagem(descritorSocket, respostaHandshake) || respostaHandshake.tipo != plant::tipo_mensagem::ACK) {
        cerr << "[" << idSensor << "] Handshake falhou, encerrando\n";
        plant::fecharSocket(descritorSocket);
        return 1;
    }
    cout << "[" << idSensor << "] Handshake confirmado pelo gerenciador\n";

    float valorAtual = valorInicialParaSubtipo(subtipo);

    // Loop principal de envio de leituras, uma leitura por segundo, conforme requisito 1.3 da especificacao
    while (true) {
        valorAtual += gerarVariacaoAleatoria();

        ostringstream payload;
        payload << "VALOR: " << valorAtual;

        string mensagemData = plant::construirMensagem(plant::tipo_mensagem::DATA, idSensor, payload.str());
        if (!plant::enviarTexto(descritorSocket, mensagemData)) {
            cerr << "[" << idSensor << "] Falha ao enviar DATA, conexao perdida\n";
            break;
        }

        cout << "[" << idSensor << "] DATA enviado valor " << valorAtual << "\n";

        this_thread::sleep_for(chrono::seconds(1));
    }

    plant::fecharSocket(descritorSocket);
    return 0;
}