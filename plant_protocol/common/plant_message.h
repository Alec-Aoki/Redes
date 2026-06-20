#ifndef PLANT_MESSAGE_H
#define PLANT_MESSAGE_H

#include <string>
#include <map>
using namespace std;

/*
Esse modulo implementa a camada de mensagens do protocolo
Uma mensagem tem o seguinte formato textual:
   PLANT/1.0 TIPO\n
   ID: ID-DISPOSITIVO\n
   TAMANHO: N\n
   \n
   payload com N bytes (se TAMANHO > 0)

O header sempre termina com uma linha em branco (sequencia \n\n),
que sinaliza o inicio do payload. O campo TAMANHO informa exatamente
quantos bytes de payload devem ser lidos a seguir, o que eh essencial
pra o parsing correto sobre TCP: como o TCP entrega um fluxo continuo
de bytes (sem preservar fronteiras de mensagem), nao podemos confiar
em delimitadores dentro do payload. Sabendo o tamanho exato, lemos
precisamente os bytes que pertencem a mensagem atual, mesmo que o
proximo recv() ja traga dados da mensagem seguinte
*/

namespace plant {

/* Estrutura que representa uma mensagem ja decodificada, pronta
pra ser interpretada pela logica de aplicacao (gerenciador, sensor,
atuador ou cliente) */
struct Mensagem {
    string versao; // Ex: "1.0"
    string tipo; // Ex: "HELLO", "DATA", "ACK", etc.
    string idDispositivo; // Ex: "SENS-TERMO-01"
    int tamanhoPayload; // Quantidade de bytes do payload
    string payloadBruto; // Conteudo do payload, ainda sem parsing de campos

    Mensagem() : tamanhoPayload(0) {}
};

/* Constroi a string completa de uma mensagem (header + payload),
pronta pra ser enviada pelo socket. O payload deve ser passado ja
formatado (ex., "VALOR: 24.5"), e esta funcao calcula o
tamanho automaticamente a partir do tamanho da string recebida */
string construirMensagem(const string& tipo, const string& idDispositivo, const string& payload = "");

/* Constroi o payload de uma mensagem ERR a partir de um codigo numerico
e uma descricao textual, no formato "CODIGO: cod, MSG: msg" */
string construirPayloadErro(int codigo, const string& mensagemErro);

/* Faz o parsing de um header ja recebido (string contendo todas as
linhas do header, sem incluir a linha em branco final) e preenche os
campos versao, tipo, idDispositivo e tamanhoPayload na mensagem
fornecida por referencia. Retorna false se o header estiver mal
formatado */
bool parsearHeader(const string& textoHeader, Mensagem& mensagemSaida);

/* Interpreta um payload no formato "CHAVE1: valor1, CHAVE2: valor2" e
retorna um mapa de chave pra valor. As chaves sao convertidas pra
maiusculas, ja que o protocolo nao diferencia caixa em chaves e
comandos. Os valores mantem a grafia original (case sensitive) */
map<string, string> parsearCamposPayload(const string& payload);

/* Converte uma string pra maiusculas. Usada pra compracoes de chaves
e comandos, que nao diferenciam caixa segundo a especificacao */
string toMaiusculas(const string& texto);

}

#endif