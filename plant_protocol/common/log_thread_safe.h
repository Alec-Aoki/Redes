#ifndef LOG_THREAD_SAFE_H
#define LOG_THREAD_SAFE_H

#include <string>
using namespace std;

/*
cout nao eh thread safe pra escritas concorrentes de linhas
completas
Quando duas threads chamam "cout << a << b << c" ao mesmo tempo,
os caracteres de uma chamada podem se intercalar com os da outra,
ja que cada operador << eh uma escrita separada no buffer
Isso fica bem visivel no Gerenciador, que roda uma thread por conexao
e varias threads escrevem log ao mesmo tempo (cada sensor, cada
atuador respondendo)

Esse modulo concentra toda escrita no console por tras de um mutex
unico, garantindo que cada linha de log seja escrita de forma atomica,
sem se misturar com a linha de outra thread
*/

namespace plant {

// Escreve uma linha completa no console de forma atomica entre threads
void logThreadSafe(const string& linha);

}

#endif