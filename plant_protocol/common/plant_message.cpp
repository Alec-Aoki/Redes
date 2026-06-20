#include "plant_message.h"
#include "device_ids.h"
#include <sstream>
#include <algorithm>
#include <cctype>
using namespace std;

namespace plant {

string toMaiusculas(const string& texto) {
    string resultado = texto;
    transform(resultado.begin(), resultado.end(), resultado.begin(),
                [](unsigned char caractere) { return toupper(caractere); });
    return resultado;
}

/* Remove espacos em branco nas extremidades de uma string.
Usado pra separar campos de header e payload */
static string removerEspacosExtremidade(const string& texto) {
    size_t inicio = texto.find_first_not_of(" \t\r\n");
    if (inicio == string::npos) {
        return "";
    }
    size_t fim = texto.find_last_not_of(" \t\r\n");
    return texto.substr(inicio, fim - inicio + 1);
}

string construirMensagem(const string& tipo, const string& idDispositivo, const string& payload) {
    /* O header eh formado por tres linhas obrigatorias ou condicionais,
    cada uma terminada por \n. A linha em branco extra ao final
    (resultando em \n\n) marca a transicao pro payload */
    ostringstream mensagem;
    mensagem << "PLANT/" << VERSAO_PROTOCOLO << " " << tipo << "\n";
    mensagem << "ID: " << idDispositivo << "\n";
    mensagem << "TAMANHO: " << payload.size() << "\n";
    mensagem << "\n";
    mensagem << payload;
    return mensagem.str();
}

string construirPayloadErro(int codigo, const string& mensagemErro) {
    ostringstream payload;
    payload << "CODIGO: " << codigo << ", MSG: " << mensagemErro;
    return payload.str();
}

bool parsearHeader(const string& textoHeader, Mensagem& mensagemSaida) {
    /* O header chega como um bloco de linhas sepradas por \n.
    Dividimos essas linhas e processamos cada uma de acordo com seu
    prefixo conhecido */
    istringstream fluxoHeader(textoHeader);
    string linhaAtual;

    bool linhaRequisicaoEncontrada = false;
    bool idDispositivoEncontrado = false;
    mensagemSaida.tamanhoPayload = 0; // tamanho eh condicional, entao o padrao eh zero

    while (getline(fluxoHeader, linhaAtual)) {
        linhaAtual = removerEspacosExtremidade(linhaAtual);
        if (linhaAtual.empty()) {
            continue;
        }

        // A linha de requisicao tem o formato "PLANT/VERSAO TIPO"
        if (linhaAtual.rfind("PLANT/", 0) == 0) {
            istringstream fluxoLinha(linhaAtual);
            string tokenVersao;
            string tokenTipo;
            fluxoLinha >> tokenVersao >> tokenTipo;

            // tokenVersao vem como "PLANT/1.0", entao extraimos apenas
            // a parte numerica apos a barra
            size_t posicaoBarra = tokenVersao.find('/');
            if (posicaoBarra == string::npos || tokenTipo.empty()) {
                return false; // Header mal formado
            }
            mensagemSaida.versao = tokenVersao.substr(posicaoBarra + 1);
            mensagemSaida.tipo = toMaiusculas(tokenTipo);
            linhaRequisicaoEncontrada = true;
            continue;
        }

        // O resto das linhas seguem o formato "CHAVE: valor"
        size_t posicaoDoisPontos = linhaAtual.find(':');
        if (posicaoDoisPontos == string::npos) {
            continue; // Linha desconhecida, ignoramos
        }

        string chave = toMaiusculas(removerEspacosExtremidade(linhaAtual.substr(0, posicaoDoisPontos)));
        string valor = removerEspacosExtremidade(
            linhaAtual.substr(posicaoDoisPontos + 1));

        if (chave == "ID") {
            // O ID-Dispositivo mantem a grafia original, nao eh convertido
            // pra maiusculas
            mensagemSaida.idDispositivo = valor;
            idDispositivoEncontrado = true;
        } else if (chave == "TAMANHO") {
            mensagemSaida.tamanhoPayload = atoi(valor.c_str());
        }
    }

    return linhaRequisicaoEncontrada && idDispositivoEncontrado;
}

map<string, string> parsearCamposPayload(const string& payload) {
    /* Campos de payload sao separados por virgula, e cada campo segue o
    formato "CHAVE: valor". Ex:
    "ALVO: SENS-TERMO-01, MIN: 18.0, MAX: 26.0" */
    map<string, string> campos;
    istringstream fluxoPayload(payload);
    string segmento;

    while (getline(fluxoPayload, segmento, ',')) {
        size_t posicaoDoisPontos = segmento.find(':');
        if (posicaoDoisPontos == string::npos) {
            continue;
        }
        string chave = toMaiusculas(removerEspacosExtremidade(segmento.substr(0, posicaoDoisPontos)));
        string valor = removerEspacosExtremidade(segmento.substr(posicaoDoisPontos + 1));
        campos[chave] = valor;
    }

    return campos;
}

}