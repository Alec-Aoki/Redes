#include "connection_handler.h"
#include "../common/plant_message.h"
#include "../common/socket_utils.h"
#include "../common/device_ids.h"
#include "../common/log_thread_safe.h"
#include <sstream> 
#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
using namespace std;

namespace plant {

/*
Margem de histerese usada pra decidir quando desligar um atuador
Sem essa margem, o atuador ligaria e desligaria repetidamente toda
vez que a leitura oscilasse em torno do limite exato, mesmo que a
oscilacao fosse minima (ruido de sensor, por exemplo)
Com a margem, o atuador so desliga quando a leitura volta a uma
distancia segura dentro do limite configurado
*/
const float MARGEM_HISTERESE = 1.0f;

// Envia uma mensagem ERR generica de volta pro socket informado
static void enviarErro(int descritorSocket, int codigo, const string& mensagemErro, const string& idGerenciador) {
    string payload = construirPayloadErro(codigo, mensagemErro);
    string mensagem = construirMensagem(tipo_mensagem::ERR, idGerenciador, payload);
    enviarTexto(descritorSocket, mensagem);
}

// Envia um ACK simples (payload vazio) de volta pro socket informado
static void enviarAck(int descritorSocket, const string& idGerenciador) {
    string mensagem = construirMensagem(tipo_mensagem::ACK, idGerenciador, "");
    enviarTexto(descritorSocket, mensagem);
}

/*
Extrai o subtipo de um ID-Dispositivo
Ex: SENS-TERMO-01 vira TERMO
Usado pra decidir qual atuador acionar quando uma leitura de sensor
sai dos limites configurados
*/
static string extrairSubtipo(const string& idDispositivo) {
    size_t primeiroHifen = idDispositivo.find('-');
    if (primeiroHifen == string::npos) {
        return "";
    }
    size_t segundoHifen = idDispositivo.find('-', primeiroHifen + 1);
    if (segundoHifen == string::npos) {
        return idDispositivo.substr(primeiroHifen + 1);
    }
    return idDispositivo.substr(primeiroHifen + 1, segundoHifen - primeiroHifen - 1);
}

/*
Manda um CMD pro atuador informado, mas so se o estado atual guardado
no registro for diferente do estado desejado
Isso evita reenviar CMD ON repetidamente enquanto a leitura continuar
fora do limite, ja que o atuador so precisa de um comando quando o
estado dele de fato precisa mudar
*/
static void acionarAtuadorSeNecessario(const string& idAtuador, bool deveLigar, RegistroAtuadores& registroAtuadores, const string& idGerenciador) {
    bool atuadorConectado = false;
    bool estadoAtual = registroAtuadores.estaLigado(idAtuador, atuadorConectado);

    // Se o atuador nunca se conectou, nao tem o que fazer, a leitura so fica fora do limite mesmo
    if (!atuadorConectado) {
        ostringstream linhaLog;
        linhaLog << "[GERENCIADOR] Leitura fora do limite, mas " << idAtuador << " nao esta conectado";
        logThreadSafe(linhaLog.str()); 
        return;
    }

    if (estadoAtual == deveLigar) {
        return; // Atuador ja esta no estado correto, nao precisa mandar nada
    }

    bool socketEncontrado = false;
    int socketAtuador = registroAtuadores.obterSocket(idAtuador, socketEncontrado);
    if (!socketEncontrado) {
        return;
    }

    string acao = deveLigar ? "ON" : "OFF";
    string payload = "ACAO: " + acao;
    string mensagem = construirMensagem(tipo_mensagem::CMD, idGerenciador, payload);

    if (enviarTexto(socketAtuador, mensagem)) {
        ostringstream linhaLog;
        linhaLog << "[GERENCIADOR] CMD enviado para " << idAtuador << " acao " << acao << "\n";
        logThreadSafe(linhaLog.str());
        // Atualiza o estado otimisticamente, a resposta ACK/ERR do atuador eh tratada na thread dele
        registroAtuadores.definirEstadoLigado(idAtuador, deveLigar);
    }
}

/*
Aplica a logica de histerese pra decidir quais atuadores ligar ou
desligar com base na leitura mais recente de um sensor
A associacao sensor para atuador eh fixa por tipo, conforme definido:
TERMO controla AQUEC e RESF, UMI controla IRR, CO2 controla INJ
*/
static void aplicarHisterese(const string& subtipoSensor, float valorLido, float minimo, float maximo, RegistroAtuadores& registroAtuadores, const string& idGerenciador) {
    if (subtipoSensor == subtipo_sensor::TEMPERATURA) {
        string idAquecedor = tipo_dispositivo::ATUADOR + "-" + subtipo_atuador::AQUECEDOR + "-01";
        string idResfriador = tipo_dispositivo::ATUADOR + "-" + subtipo_atuador::RESFRIADOR + "-01";

        if (valorLido < minimo) {
            acionarAtuadorSeNecessario(idAquecedor, true, registroAtuadores, idGerenciador);
        } else if (valorLido > maximo) {
            acionarAtuadorSeNecessario(idResfriador, true, registroAtuadores, idGerenciador);
        } else {
            /*
            Dentro da faixa segura (considerando a margem de histerese),
            desliga os dois atuadores de temperatura caso estejam ligados
            A margem evita que o atuador fique ligando e desligando perto
            do limite exato
            */
            if (valorLido > minimo + MARGEM_HISTERESE) {
                acionarAtuadorSeNecessario(idAquecedor, false, registroAtuadores, idGerenciador);
            }
            if (valorLido < maximo - MARGEM_HISTERESE) {
                acionarAtuadorSeNecessario(idResfriador, false, registroAtuadores, idGerenciador);
            }
        }
    } else if (subtipoSensor == subtipo_sensor::UMIDADE) {
        string idIrrigacao = tipo_dispositivo::ATUADOR + "-" + subtipo_atuador::IRRIGACAO + "-01";

        if (valorLido < minimo) {
            acionarAtuadorSeNecessario(idIrrigacao, true, registroAtuadores, idGerenciador);
        } else if (valorLido > minimo + MARGEM_HISTERESE) {
            // estufa nao tem atuador pra reduzir umidade, so desliga a irrigacao quando ja estiver segura
            acionarAtuadorSeNecessario(idIrrigacao, false, registroAtuadores, idGerenciador);
        }
    } else if (subtipoSensor == subtipo_sensor::CO2) {
        string idInjetor = tipo_dispositivo::ATUADOR + "-" + subtipo_atuador::INJETOR_CO2 + "-01";

        if (valorLido < minimo) {
            acionarAtuadorSeNecessario(idInjetor, true, registroAtuadores, idGerenciador);
        } else if (valorLido > minimo + MARGEM_HISTERESE) {
            // estufa nao tem atuador pra reduzir CO2, so desliga o injetor quando ja estiver seguro
            acionarAtuadorSeNecessario(idInjetor, false, registroAtuadores, idGerenciador);
        }
    }
}

// Loop especifico pra conexoes de sensor, recebe DATA continuamente e aplica histerese
static void tratarSensor(int descritorSocket, const string& idSensor, RegistroSensores& registroSensores,
                        RegistroAtuadores& registroAtuadores, const string& idGerenciador) {
    registroSensores.registrarSensor(idSensor);
    logThreadSafe("[GERENCIADOR] Sensor " + idSensor + " registrado\n");

    Mensagem mensagemRecebida;
    while (receberMensagem(descritorSocket, mensagemRecebida)) {
        if (mensagemRecebida.tipo != tipo_mensagem::DATA) {
            enviarErro(descritorSocket, codigo_resposta::REQUISICAO_INVALIDA, "TIPO INESPERADO PARA SENSOR", idGerenciador);
            continue;
        }

        auto campos = parsearCamposPayload(mensagemRecebida.payloadBruto);
        if (campos.find("VALOR") == campos.end()) {
            enviarErro(descritorSocket, codigo_resposta::REQUISICAO_INVALIDA, "CAMPO VALOR AUSENTE", idGerenciador);
            continue;
        }

        float valorLido = atof(campos["VALOR"].c_str());
        registroSensores.atualizarLeitura(idSensor, valorLido);

        ostringstream linhaLog;
        linhaLog << "[GERENCIADOR] DATA de " << idSensor << " valor " << valorLido << "\n";
        logThreadSafe(linhaLog.str());

        // DATA nao exige resposta explicita em operacao normal, conforme especificado no relatorio

        EstadoSensor estadoAtual;
        if (registroSensores.obterEstado(idSensor, estadoAtual) && estadoAtual.limitesConfigurados) {
            string subtipo = extrairSubtipo(idSensor);
            aplicarHisterese(subtipo, valorLido, estadoAtual.limiteMinimo, estadoAtual.limiteMaximo, registroAtuadores, idGerenciador);
        }
    }

    logThreadSafe("[GERENCIADOR] Sensor " + idSensor + " desconectado\n");
}

// Loop especifico pra conexoes de atuador, recebe ACK/ERR em resposta aos comandos enviados
static void tratarAtuador(int descritorSocket, const string& idAtuador, RegistroAtuadores& registroAtuadores) {
    registroAtuadores.registrarAtuador(idAtuador, descritorSocket);
    logThreadSafe("[GERENCIADOR] Atuador " + idAtuador + " registrado\n");

    Mensagem mensagemRecebida;
    while (receberMensagem(descritorSocket, mensagemRecebida)) {
        /* Atuador so responde ACK (confirmando o CMD) ou ERR (falha de
        hardware), entao so logamos a resposta aqui
        Se vier ERR, assumimos estado padrao seguro desligando o atuador no registro */
        if (mensagemRecebida.tipo == tipo_mensagem::ACK) {
            logThreadSafe("[GERENCIADOR] " + idAtuador + " confirmou ACK\n");
        } else if (mensagemRecebida.tipo == tipo_mensagem::ERR) {
            logThreadSafe("[GERENCIADOR] " + idAtuador + " retornou ERR, assumindo estado seguro\n");
            registroAtuadores.definirEstadoLigado(idAtuador, false);
        }
    }

    registroAtuadores.removerAtuador(idAtuador);
    logThreadSafe("[GERENCIADOR] Atuador " + idAtuador + " desconectado\n");
}

// Loop especifico pra conexoes de cliente, atende CONFIG e REQ_DATA
static void tratarCliente(int descritorSocket, const string& idCliente, RegistroSensores& registroSensores, const string& idGerenciador) {
    logThreadSafe("[GERENCIADOR] Cliente " + idCliente + " conectado\n");

    Mensagem mensagemRecebida;
    while (receberMensagem(descritorSocket, mensagemRecebida)) {
        if (mensagemRecebida.tipo == tipo_mensagem::CONFIG) {
            auto campos = parsearCamposPayload(mensagemRecebida.payloadBruto);
            if (campos.find("ALVO") == campos.end() || campos.find("MIN") == campos.end() || campos.find("MAX") == campos.end()) {
                enviarErro(descritorSocket, codigo_resposta::REQUISICAO_INVALIDA, "CAMPOS OBRIGATORIOS AUSENTES", idGerenciador);
                continue;
            }

            string idAlvo = campos["ALVO"];
            if (!registroSensores.sensorExiste(idAlvo)) {
                enviarErro(descritorSocket, codigo_resposta::NAO_ENCONTRADO, "SENSOR NAO ENCONTRADO", idGerenciador);
                continue;
            }

            float minimo = atof(campos["MIN"].c_str());
            float maximo = atof(campos["MAX"].c_str());
            if (minimo > maximo) {
                enviarErro(descritorSocket, codigo_resposta::REQUISICAO_INVALIDA, "MIN MAIOR QUE MAX", idGerenciador);
                continue;
            }

            registroSensores.definirLimites(idAlvo, minimo, maximo);

            ostringstream linhaLog;
            linhaLog << "[GERENCIADOR] Limites de " << idAlvo << " configurados min " << minimo << " max " << maximo << "\n";
            logThreadSafe(linhaLog.str());

            enviarAck(descritorSocket, idGerenciador);

        } else if (mensagemRecebida.tipo == tipo_mensagem::REQ_DATA) {
            auto campos = parsearCamposPayload(mensagemRecebida.payloadBruto);
            if (campos.find("ALVO") == campos.end()) {
                enviarErro(descritorSocket, codigo_resposta::REQUISICAO_INVALIDA, "CAMPO ALVO AUSENTE", idGerenciador);
                continue;
            }

            string idAlvo = campos["ALVO"];

            if (toMaiusculas(idAlvo) == "ALL") {
                /* Monta o payload de resposta concatenando todos os
                sensores conhecidos no formato ID: valor, separados por
                virgula */
                auto todosOsSensores = registroSensores.obterTodosOsSensores();
                string payloadResposta;
                bool primeiro = true;
                for (auto const& parSensor : todosOsSensores) {
                    if (!primeiro) {
                        payloadResposta += ", ";
                    }
                    payloadResposta += parSensor.first + ": " + to_string(parSensor.second.ultimoValor);
                    primeiro = false;
                }
                string mensagemResposta = construirMensagem(tipo_mensagem::RESP_DATA, idGerenciador, payloadResposta);
                enviarTexto(descritorSocket, mensagemResposta);
            } else {
                bool encontrado = false;
                float valor = registroSensores.obterUltimoValor(idAlvo, encontrado);
                if (!encontrado) {
                    enviarErro(descritorSocket, codigo_resposta::NAO_ENCONTRADO, "SENSOR NAO ENCONTRADO", idGerenciador);
                    continue;
                }
                string payloadResposta = idAlvo + ": " + to_string(valor);
                string mensagemResposta = construirMensagem(tipo_mensagem::RESP_DATA, idGerenciador, payloadResposta);
                enviarTexto(descritorSocket, mensagemResposta);
            }

        } else {
            enviarErro(descritorSocket, codigo_resposta::REQUISICAO_INVALIDA, "TIPO INESPERADO PARA CLIENTE", idGerenciador);
        }
    }

    logThreadSafe("[GERENCIADOR] Cliente " + idCliente + " desconectado\n");
}

void tratarConexao(int descritorSocket, RegistroSensores& registroSensores, RegistroAtuadores& registroAtuadores) {
    string idGerenciador = tipo_dispositivo::GERENCIADOR + "-MAIN-01";

    /*
    Define um timeout de leitura no socket pra que a chamada de
    receberMensagem usada no handshake nao bloqueie indefinidamente
    se o dispositivo nunca mandar o HELLO
    Isso implementa o requisito de encerrar a conexao se o handshake
    nao for concluido dentro do tempo limite
    */
    struct timeval tempoLimite;
    tempoLimite.tv_sec = TIMEOUT_HANDSHAKE_SEGUNDOS;
    tempoLimite.tv_usec = 0;
    setsockopt(descritorSocket, SOL_SOCKET, SO_RCVTIMEO, &tempoLimite, sizeof(tempoLimite));

    Mensagem mensagemHello;
    if (!receberMensagem(descritorSocket, mensagemHello) || mensagemHello.tipo != tipo_mensagem::HELLO) {
        cout << "[GERENCIADOR] Handshake nao concluido a tempo, encerrando conexao\n";
        fecharSocket(descritorSocket);
        return;
    }

    /*
    Apos o handshake, removemos o timeout de leitura
    Sensores e atuadores ficam conectados por longos periodos sem
    necessariamente mandar dados a todo instante (no caso do atuador,
    ele so manda algo quando responde a um CMD), entao um timeout
    continuaria derrubando conexoes legitimas e ociosas
    */
    tempoLimite.tv_sec = 0;
    tempoLimite.tv_usec = 0;
    setsockopt(descritorSocket, SOL_SOCKET, SO_RCVTIMEO, &tempoLimite, sizeof(tempoLimite));

    string idDispositivo = mensagemHello.idDispositivo;
    enviarAck(descritorSocket, idGerenciador);

    /*
    Decide o tipo de dispositivo pelo prefixo do ID
    Por exemplo, SENS-TERMO-01 comeca com SENS, ATU-AQUEC-01 comeca com
    ATU, e assim por diante
    */
    if (idDispositivo.rfind(tipo_dispositivo::SENSOR, 0) == 0) {
        tratarSensor(descritorSocket, idDispositivo, registroSensores, registroAtuadores, idGerenciador);
    } else if (idDispositivo.rfind(tipo_dispositivo::ATUADOR, 0) == 0) {
        tratarAtuador(descritorSocket, idDispositivo, registroAtuadores);
    } else if (idDispositivo.rfind(tipo_dispositivo::CLIENTE, 0) == 0) {
        tratarCliente(descritorSocket, idDispositivo, registroSensores, idGerenciador);
    } else {
        cout << "[GERENCIADOR] Tipo de dispositivo desconhecido para ID " << idDispositivo << "\n";
    }

    fecharSocket(descritorSocket);
}

}