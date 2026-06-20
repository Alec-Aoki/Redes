#include "connection_handler.h"
#include "sensor_registry.h"
#include "actuator_registry.h"
#include "../common/socket_utils.h"
#include "../common/device_ids.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
using namespace std;

/*
Ponto de entrada do processo Gerenciador
Esse processo roda como servidor TCP passivo, ficando em loop infinito
de accept esperando novas conexoes de sensores, atuadores e clientes

Cada conexao aceita ganha uma thread propria pra ser tratada, e essa
thread roda enquanto a conexao estiver aberta
Os registros de sensores e atuadores sao criados uma unica vez aqui
no main e compartilhados entre todas as threads por referencia, ja
que representam o estado global da estufa
*/

int main() {
    /*
    Forca o flush do buffer de saida apos cada operacao de cout
    Sem isso, em alguns casos as mensagens de log só apareceriam quando o buffer
    enchesse ou o processo terminasse
    */
    cout.setf(ios::unitbuf);

    plant::RegistroSensores registroSensores;
    plant::RegistroAtuadores registroAtuadores;

    int socketServidor = plant::criarSocketServidor(plant::PORTA_GERENCIADOR);
    if (socketServidor < 0) {
        cerr << "Nao foi possivel iniciar o gerenciador, encerrando\n";
        return 1;
    }

    cout << "[GERENCIADOR] Escutando na porta " << plant::PORTA_GERENCIADOR << "\n";

    while (true) {
        int socketConexao = accept(socketServidor, nullptr, nullptr);
        if (socketConexao < 0) {
            cerr << "Erro ao aceitar conexao, continuando o loop\n";
            continue;
        }

        /*
        Cria uma thread nova pra tratar essa conexao especifica
        usamos ref pra passar os registros por referencia, ja que
        por padrao thread copia os argumentos recebidos
        detach() libera a thread pra rodar de forma independente, ja
        que nao precisamos esperar (join) ela terminar no main
        */
        thread threadConexao(plant::tratarConexao, socketConexao, ref(registroSensores), ref(registroAtuadores));
        threadConexao.detach();
    }

    return 0;
}