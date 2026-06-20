#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <string>
#include "plant_message.h"
using namespace std;

/*
Esse modulo encapsula as chamadas de socket POSIX (camada de
transporte TCP) usadas por todos os processos do sistema
O objetivo eh isolar os detalhes de baixo nivel (criacao de socket, bind, listen,
accept, connect, send, recv) em funcoes simples e reutilizaveis, pra
que os modulos de logica de aplicacao (gerenciador, sensor, atuador,
cliente) nao precisem lidar diretamente com a API de sockets
*/

namespace plant {

/* Cria um socket TCP servidor, faz bind na porta informada e comeca a
escutar conexoes. Retorna o descritor do socket, ou -1 em caso de
falha (a funcao imprime a causa do erro antes de retornar) */
int criarSocketServidor(int porta);

/* Conecta como cliente TCP ao endereco e porta informados
Retorna o descritor do socket conectado, ou -1 em caso de falha */
int conectarAoServidor(const string& endereco, int porta);

/* Envia uma string completa pelo socket, garantindo que todos os bytes
sejam transmitidos mesmo que send() retorne um envio parcial
Retorna true se o envio foi bem sucedido */
bool enviarTexto(int descritorSocket, const string& texto);

/* Le exatamente uma mensagem PLANT completa do socket: primeiro le o
header byte a byte ate encontrar a sequencia \n\n, faz o parsing
desse header, e em seguida le exatamente "tamanho" bytes de payload
Retorna true se uma mensagem completa e valida foi recebida, e
preenche mensagemSaida com os dados decodificados
Retorna false em caso de desconexao do peer ou erro de leitura */
bool receberMensagem(int descritorSocket, Mensagem& mensagemSaida);

// Fecha um descritor de socket de forma segura
void fecharSocket(int descritorSocket);

}

#endif