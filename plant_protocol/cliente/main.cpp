#include "../common/plant_message.h"
#include "../common/socket_utils.h"
#include "../common/device_ids.h"
#include <iostream>
#include <sstream>
#include <limits>
using namespace std;

/*
Ponto de entrada do processo Cliente
Esse processo representa o cliente externo da estufa, que pode
configurar limites de operacao dos sensores e requisitar a ultima
leitura armazenada no Gerenciador

O cliente roda um menu interativo no terminal, esperando o usuario
escolher uma acao a cada iteracao do loop principal
*/

// Imprime as opcoes disponiveis no menu
static void mostrarMenu() {
    cout << "\n--- MENU CLIENTE ESTUFA ---\n";
    cout << "1 Configurar limites de um sensor (CONFIG)\n";
    cout << "2 Requisitar leitura de um sensor especifico (REQ_DATA)\n";
    cout << "3 Requisitar leitura de todos os sensores (REQ_DATA ALL)\n";
    cout << "0 Sair\n";
    cout << "Escolha uma opcao: ";
}

// Envia uma mensagem CONFIG com os limites informados pelo usuario
static void enviarConfig(int descritorSocket, const string& idCliente) {
    string idSensorAlvo;
    float minimo;
    float maximo;

    cout << "ID do sensor alvo, exemplo SENS-TERMO-01: ";
    cin >> idSensorAlvo;
    cout << "Valor minimo: ";
    cin >> minimo;
    cout << "Valor maximo: ";
    cin >> maximo;

    ostringstream payload;
    payload << "ALVO: " << idSensorAlvo << ", MIN: " << minimo << ", MAX: " << maximo;

    string mensagem = plant::construirMensagem(plant::tipo_mensagem::CONFIG, idCliente, payload.str());
    plant::enviarTexto(descritorSocket, mensagem);

    plant::Mensagem resposta;
    if (!plant::receberMensagem(descritorSocket, resposta)) {
        cout << "Conexao perdida ao aguardar resposta\n";
        return;
    }

    if (resposta.tipo == plant::tipo_mensagem::ACK) {
        cout << "Limites configurados com sucesso\n";
    } else {
        cout << "Erro recebido do gerenciador: " << resposta.payloadBruto << "\n";
    }
}

// Envia uma mensagem REQ_DATA pedindo a leitura de um sensor especifico ou de todos
static void enviarReqData(int descritorSocket, const string& idCliente, bool todosOsSensores) {
    string idSensorAlvo = "ALL";

    if (!todosOsSensores) {
        cout << "ID do sensor alvo, exemplo SENS-TERMO-01: ";
        cin >> idSensorAlvo;
    }

    string payload = "ALVO: " + idSensorAlvo;
    string mensagem = plant::construirMensagem(plant::tipo_mensagem::REQ_DATA, idCliente, payload);
    plant::enviarTexto(descritorSocket, mensagem);

    plant::Mensagem resposta;
    if (!plant::receberMensagem(descritorSocket, resposta)) {
        cout << "Conexao perdida ao aguardar resposta\n";
        return;
    }

    if (resposta.tipo == plant::tipo_mensagem::RESP_DATA) {
        cout << "Leitura recebida: " << resposta.payloadBruto << "\n";
    } else {
        cout << "Erro recebido do gerenciador: " << resposta.payloadBruto << "\n";
    }
}

int main() {
    // Forca o flush do buffer de saida apos cada cout, mesma justificativa do gerenciador
    cout.setf(ios::unitbuf);

    string idCliente = plant::tipo_dispositivo::CLIENTE + "-EXT-01";

    int descritorSocket = plant::conectarAoServidor("127.0.0.1", plant::PORTA_GERENCIADOR);
    if (descritorSocket < 0) {
        cerr << "Nao foi possivel conectar ao gerenciador\n";
        return 1;
    }

    // Etapa de handshake, conforme especificado no protocolo PLANT
    string mensagemHello = plant::construirMensagem(plant::tipo_mensagem::HELLO, idCliente, "");
    plant::enviarTexto(descritorSocket, mensagemHello);

    plant::Mensagem respostaHandshake;
    if (!plant::receberMensagem(descritorSocket, respostaHandshake) || respostaHandshake.tipo != plant::tipo_mensagem::ACK) {
        cerr << "Handshake falhou, encerrando\n";
        plant::fecharSocket(descritorSocket);
        return 1;
    }
    cout << "Conectado ao gerenciador como " << idCliente << "\n";

    int opcaoEscolhida = -1;
    while (opcaoEscolhida != 0) {
        mostrarMenu();
        cin >> opcaoEscolhida;

        if (cin.fail()) {
            // Limpa o estado de erro do cin caso o usuario digite algo que nao seja numero
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Entrada invalida, tente novamente\n";
            continue;
        }

        switch (opcaoEscolhida) {
            case 1:
                enviarConfig(descritorSocket, idCliente);
                break;
            case 2:
                enviarReqData(descritorSocket, idCliente, false);
                break;
            case 3:
                enviarReqData(descritorSocket, idCliente, true);
                break;
            case 0:
                cout << "Encerrando cliente\n";
                break;
            default:
                cout << "Opcao invalida\n";
        }
    }

    plant::fecharSocket(descritorSocket);
    return 0;
}