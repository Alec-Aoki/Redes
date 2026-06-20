#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "sensor_registry.h"
#include "actuator_registry.h"
using namespace std;

/*
Esse modulo contem a logica executada por cada thread do Gerenciador
Cada conexao aceita (de um sensor, atuador ou cliente) gera uma thread
nova, e essa thread roda a funcao tratarConexao do inicio ao fim da
vida da conexao

O fluxo geral eh:
1 espera o HELLO do dispositivo (com timeout)
2 identifica o tipo do dispositivo pelo prefixo do ID (SENS, ATU, CLI)
3 entra no loop de tratamento especifico daquele tipo de dispositivo

Os registros de sensores e atuadores sao recebidos por referencia pq
sao compartilhados entre todas as threads (cada thread atende uma
conexao diferente, mas todas leem e escrevem no mesmo estado global
da estufa)
*/

namespace plant {

// Funcao principal executada por cada thread, recebe o socket aceito e os registros compartilhados
void tratarConexao(int descritorSocket, RegistroSensores& registroSensores, RegistroAtuadores& registroAtuadores);

}

#endif