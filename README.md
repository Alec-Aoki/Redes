# Estufa Inteligente, protocolo PLANT

Implementacao do protocolo de camada de aplicacao PLANT (Protocolo para
Lavoura, com Atuadores, Nos e Telemetria), desenvolvido na Entrega 1 do
trabalho

## Integrantes

Dante Brito Lourenco
Juan Henriques Passos
Alec Campos Aoki

## SO e compilador utilizados

Linux nativo, g++ com suporte a C++17

## Estrutura do projeto

```
plant_protocol/
  common/ modulo compartilhado entre todos os processos
    plant_message.h/cpp construcao e parsing das mensagens PLANT
    socket_utils.h/cpp wrappers de socket TCP em POSIX
    device_ids.h constantes do protocolo (tipos, codigos, porta)
  gerenciador/ processo servidor da aplicacao
    main.cpp loop de accept, uma thread por conexao
    connection_handler.h/cpp logica de despacho por tipo de dispositivo e histerese
    sensor_registry.h/cpp estado dos sensores (leituras e limites), protegido por mutex
    actuator_registry.h/cpp estado dos atuadores (socket e ligado/desligado), protegido por mutex
  sensor/
    main.cpp binario unico parametrizado por linha de comando
  atuador/
    main.cpp binario unico parametrizado por linha de comando
  cliente/
    main.cpp menu interativo no terminal
  Makefile
```

Cada componente do sistema (Gerenciador, Sensor, Atuador, Cliente) eh
um processo separado no sistema operacional, conforme exigido na
especificacao do trabalho

## Compilando

Na raiz do projeto, basta rodar

```
make todos
```

Isso gera quatro binarios dentro da pasta `bin/`

```
bin/gerenciador
bin/sensor
bin/atuador
bin/cliente
```

Pra limpar os binarios gerados

```
make limpar
```

## Rodando o sistema

O Gerenciador precisa estar de pe antes de qlqlr outro processo se
conectar, ja que ele atua como servidor passivo da rede

Abra um terminal pra cada processo, na seguinte ordem

```
./bin/gerenciador
```

Depois, em terminais separados, os sensores (subtipo e numero como
argumento)

```
./bin/sensor TERMO 01
./bin/sensor UMI 01
./bin/sensor CO2 01
```

Os atuadores, do mesmo jeito

```
./bin/atuador AQUEC 01
./bin/atuador RESF 01
./bin/atuador IRR 01
./bin/atuador INJ 01
```

E o cliente, que abre um menu interativo

```
./bin/cliente
```

Nao tem ordem obrigatoria entre sensores, atuadores e cliente, mas
todos precisam do Gerenciador ja escutando na porta configurada
(8080 por padrao, ver `common/device_ids.h`)

## O que cada processo faz

**Gerenciador**: aceita conexoes de sensores, atuadores e clientes,
cada uma tratada em uma thread separada
Guarda a ultima leitura de cada sensor e os limites configurados pelo
Cliente
Quando uma leitura sai dos limites, aplica controle por histerese e
manda CMD pro atuador correspondente, sem repetir o comando se o
atuador ja estiver no estado certo

**Sensor**: conecta no Gerenciador, faz o handshake HELLO/ACK, e entra
em loop mandando DATA a cada 1 segundo, com o valor simulado por uma
pequena variacao aleatoria em torno da leitura anterior

**Atuador**: conecta no Gerenciador, faz o handshake, e fica esperando
mensagens CMD, respondendo ACK pra confirmar a mudanca de estado

**Cliente**: conecta no Gerenciador e oferece um menu pra configurar
limites de um sensor (CONFIG) ou pedir a ultima leitura de um sensor
especifico ou de todos (REQ_DATA)

## Associacao sensor para atuador

A logica de histerese usa uma associacao fixa por tipo, decidida no
codigo do Gerenciador, ja que o protocolo em si nao modela essa
relacao de forma explicita

TERMO controla AQUEC (liga abaixo do MIN) e RESF (liga acima do MAX)
UMI controla IRR (liga abaixo do MIN, desliga dentro da faixa segura)
CO2 controla INJ (liga abaixo do MIN, desliga dentro da faixa segura)

UMI e CO2 nao tem atuador de reducao na especificacao da estufa, por
isso so ligam o atuador respectivo quando a leitura fica baixa demais

## Observacoes de implementacao

Toda saida de log usa `std::cout.setf(std::ios::unitbuf)` no inicio do
main pra forcar o flush apos cada linha, garantindo que a troca de
mensagens apareca na tela em tempo real, mesmo se a saida for
redirecionada pra um arquivo durante testes

O parsing de mensagem le o socket byte a byte ate encontrar a
sequencia `\n\n`, extrai o campo TAMANHO do header, e so depois le
exatamente essa quantidade de bytes de payload, evitando problemas de
mensagens coladas no fluxo TCP