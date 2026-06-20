#ifndef ORQUESTRADOR_H
#define ORQUESTRADOR_H

#include <string>
#include <vector>
using namespace std;

/*
Esse modulo define a estrutura de configuracao de um dispositivo que o
orquestrador deve iniciar como processo filho (sensor ou atuador)

O orquestrador nao junta sensor e atuador num so processo, ja que isso
violaria o requisito do trabalho de cada componente ser um processo
separado no SO
O que ele faz eh dar fork/exec em cada dispositivo como um processo
filho de verdade (com PID proprio), e so concentra a saida de todos
esses processos em um unico terminal, prefixando cada linha com o id
do dispositivo de origem
*/

namespace orquestrador {

// Representa um dispositivo a ser iniciado pelo orquestrador
struct ConfigDisp {
    std::string tipoBinario; // "sensor" ou "atuador"
    std::string subtipo; // ex.: TERMO, AQUEC
    std::string numero; // ex.: 01

    ConfigDisp(const std::string& tipo, const std::string& sub, const std::string& num)
                : tipoBinario(tipo), subtipo(sub), numero(num) {}
};

/*
Lista default de dispositivos, usada quando o orquestrador eh chamado
sem nenhum argumento
Cobre um exemplar de cada subtipo de sensor e de atuador da estufa
*/
std::vector<ConfigDisp> listaDispoDef();

/*
Faz o parsing dos argumentos de linha de comando no formato
tipo:subtipo:numero, por exemplo sensor:TERMO:01
Retorna a lista de dispositivos a serem iniciados, ou uma lista vazia
Se algum argumento estiver mal formatado (nesse caso, a mensagem de
erro ja eh impressa dentro da propria funcao)
*/
std::vector<ConfigDisp> parsearArgs(int argumentosTotais, char* argumentos[]);

} 

#endif
