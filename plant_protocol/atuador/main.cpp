#include "../common/plant_message.h"
#include "../common/socket_utils.h"
#include "../common/device_ids.h"
#include <iostream>
using namespace std;

/*
Ponto de entrada do processo Atuador
Esse processo representa um unico atuador da estufa (aquecedor,
resfriador, irrigacao ou injetor de CO2), recebido por argumento de
linha de comando

Exemplo:
  ./atuador AQUEC 01
  ./atuador RESF 01
  ./atuador IRR 01
  ./atuador INJ 01

Apos o handshake, o atuador fica em loop esperando mensagens CMD
enviadas pelo Gerenciador, e responde com ACK pra confirmar a mudanca
de estado
*/

int main(int argumentosTotais, char* argumentos[]) {
    // Forca o flush do buffer de saida apos cada cout, mesma justificativa do gerenciador
    cout.setf(ios::unitbuf);

    if (argumentosTotais != 3) {
        cerr << "Uso esperado: ./atuador SUBTIPO NUMERO\n";
        cerr << "Exemplo: ./atuador AQUEC 01\n";
        return 1;
    }

    string subtipo = plant::toMaiusculas(argumentos[1]);
    string numero = argumentos[2];

    bool subtipoValido = (subtipo == plant::subtipo_atuador::AQUECEDOR || subtipo == plant::subtipo_atuador::RESFRIADOR ||
                            subtipo == plant::subtipo_atuador::INJETOR_CO2 || subtipo == plant::subtipo_atuador::IRRIGACAO);

    if (!subtipoValido) {
        cerr << "Subtipo invalido, use AQUEC, RESF, INJ ou IRR\n";
        return 1;
    }

    string idAtuador = plant::tipo_dispositivo::ATUADOR + "-" + subtipo + "-" + numero;

    int descritorSocket = plant::conectarAoServidor("127.0.0.1", plant::PORTA_GERENCIADOR);
    if (descritorSocket < 0) {
        cerr << "Nao foi possivel conectar ao gerenciador\n";
        return 1;
    }

    // Etapa de handshake, conforme especificado no protocolo PLANT
    string mensagemHello = plant::construirMensagem(plant::tipo_mensagem::HELLO, idAtuador, "");
    plant::enviarTexto(descritorSocket, mensagemHello);
    cout << "[" << idAtuador << "] HELLO enviado\n";

    plant::Mensagem respostaHandshake;
    if (!plant::receberMensagem(descritorSocket, respostaHandshake) || respostaHandshake.tipo != plant::tipo_mensagem::ACK) {
        cerr << "[" << idAtuador << "] Handshake falhou, encerrando\n";
        plant::fecharSocket(descritorSocket);
        return 1;
    }
    cout << "[" << idAtuador << "] Handshake confirmado pelo gerenciador\n";

    bool estadoLigado = false; // Todo atuador comeca desligado

    // Loop principal esperando comandos do gerenciador
    plant::Mensagem mensagemRecebida;
    while (plant::receberMensagem(descritorSocket, mensagemRecebida)) {
        if (mensagemRecebida.tipo != plant::tipo_mensagem::CMD) {
            cout << "[" << idAtuador << "] Mensagem inesperada recebida, ignorando\n";
            continue;
        }

        auto campos = plant::parsearCamposPayload(mensagemRecebida.payloadBruto);
        if (campos.find("ACAO") == campos.end()) {
            string payloadErro = plant::construirPayloadErro(plant::codigo_resposta::REQUISICAO_INVALIDA, "CAMPO ACAO AUSENTE");
            string mensagemErro = plant::construirMensagem(plant::tipo_mensagem::ERR, idAtuador, payloadErro);
            plant::enviarTexto(descritorSocket, mensagemErro);
            continue;
        }

        string acao = plant::toMaiusculas(campos["ACAO"]);
        estadoLigado = (acao == "ON");

        cout << "[" << idAtuador << "] CMD recebido, novo estado " << (estadoLigado ? "LIGADO" : "DESLIGADO") << "\n";

        // Confirma a execucao do comando com ACK
        string mensagemAck = plant::construirMensagem(plant::tipo_mensagem::ACK, idAtuador, "");
        plant::enviarTexto(descritorSocket, mensagemAck);
    }

    cout << "[" << idAtuador << "] Conexao com o gerenciador encerrada\n";
    plant::fecharSocket(descritorSocket);
    return 0;
}