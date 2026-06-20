# Estufa Inteligente, protocolo PLANT

Implementacao do protocolo de camada de aplicação PLANT (Protocolo para
Lavoura, com Atuadores, Nos e Telemetria), desenvolvido na Entrega 1 do
trabalho.

## Integrantes

Dante Brito Lourenco
Juan Henriques Passos
Alec Campos Aoki

## SO e compilador utilizados

Linux nativo, g++ com suporte a C++17

## Estrutura do projeto

```
plant_protocol/
  common/
    plant_message.h/cpp
    socket_utils.h/cpp
    device_ids.h
  gerenciador/
    main.cpp
    connection_handler.h/cpp
    sensor_registry.h/cpp
    actuator_registry.h/cpp
  sensor/
    main.cpp
  atuador/
    main.cpp
  cliente/
    main.cpp
  orquestrador/
  Makefile
```

`plant_message.h/cpp`: construção e parsing das mensagens

`socket_utils.h/cpp`: wrappers de socket TCP em POSIX

`device_ids.h`: constantes do protocolo (tipos, códigos, portas etc.)

`gerenciador/`: processo servidor da aplicação

`gerenciador/main.cpp`: loop de accept, com uma thread por conexão

`connection_handler.h/cpp`: lógica de despacho por tipo de dispositivo e histerese

`sensor_registry.h/cpp`: estado dos sensores (leituras e limites), protegido por mutex

`actuator_registry.h/cpp`: estado dos atuadores (socket e ligado/desligado),protegido por mutex

`sensor/main.cpp`: binário único parametrizado por linha de comando

`atuador/main.cpp`: binário único parametrizado por linha de comando

`cliente/main.cpp`: menu interativo no terminal

`orquestrador/main.cpp`: fork/exec de cada dispositivo, leitura multiplexada via pipe

`orquestrador.h/cpp`: lista default e parsing de argumetnos de linha de comando

Cada componente do sistema (Gerenciador, Sensor, Atuador, Cliente) é
um processo separado no sistema operacional, conforme exigido na
especificação do trabalho.

## Compilando

Na raiz do projeto, basta rodar

```
make todos
```

Isso gera cinco binários dentro da pasta `bin/`:

```
bin/gerenciador
bin/sensor
bin/atuador
bin/cliente
bin/orquestrador
```

Pra limpar os binarios gerados:

```
make limpar
```

## Rodando o sistema
### Ordem de execução recomendada

```
terminal 1: ./bin/gerenciador
terminal 2: ./bin/orquestrador
terminal 3: ./bin/cliente
```

### Forma Alternativa 1

Abra um terminal pra cada processo.

Primeiro:

```
./bin/gerenciador
```

Depois (em terminais separados) os sensores (subtipo e número como
argumento):

```
./bin/sensor TERMO 01
./bin/sensor UMI 01
./bin/sensor CO2 01
```

Os atuadores do mesmo jeito:

```
./bin/atuador AQUEC 01
./bin/atuador RESF 01
./bin/atuador IRR 01
./bin/atuador INJ 01
```

E o cliente, que abre um menu interativo:

```
./bin/cliente
```

Não há ordem obrigatória entre sensores, atuadores e cliente, mas
todos precisam do Gerenciador já escutando na porta configurada
(8080 por padrao, ver `common/device_ids.h`)

### Forma Alternativa 2

Primeiro, rode o gerenciador:

```
./bin/gerenciador
```

Em seguida, rode o orquestrador. Sem nenhum argumento, ele sobe uma lista fixa com um exemplar de cada
subtipo de sensor e de atuador da estufa:

```
./bin/orquestrador
```

Isso equivale a rodar, em terminais separados:

```
./bin/sensor TERMO 01
./bin/sensor UMI 01
./bin/sensor CO2 01
./bin/atuador AQUEC 01
./bin/atuador RESF 01
./bin/atuador IRR 01
./bin/atuador INJ 01
```

Se quiser controlar exatamente quais dispositivos sobem, passe um
argumento pra cada um, no formato `tipo:subtipo:numero`:

```
./bin/orquestrador sensor:TERMO:01 atuador:AQUEC:01
```

Pra encerrar todos os processos de uma vez, basta um `Ctrl+C` no
terminal onde o Orquestrador está rodando (o encerramento será propagado
pra todos os filhos automaticamente).

## Detalhamento

**Gerenciador**: aceita conexões de sensores, atuadores e clientes,
cada uma tratada em uma thread separada.
Guarda a última leitura de cada sensor e os limites configurados pelo
Cliente.
Quando uma leitura sai dos limites, aplica controle por histerese e
manda CMD pro atuador correspondente, sem repetir o comando se o
atuador já estiver no estado certo.

**Sensor**: conecta no Gerenciador, faz o handshake HELLO/ACK, e entra
em loop mandando DATA a cada 1 segundo, com o valor simulado por uma
pequena variação aleatória em torno da leitura anterior.

**Atuador**: conecta no Gerenciador, faz o handshake, e fica esperando
mensagens CMD, respondendo ACK pra confirmar a mudança de estado.

**Cliente**: conecta no Gerenciador e oferece um menu pra configurar
limites de um sensor (CONFIG) ou pedir a última leitura de um sensor
específico ou de todos (REQ_DATA).

## Associação sensor para atuador

A lógica de histerese usa uma associação fixa por tipo, decidida no
código do Gerenciador, já que o protocolo em si não modela essa
relação de forma explícita.

TERMO controla AQUEC (liga abaixo do MIN) e RESF (liga acima do MAX).
UMI controla IRR (liga abaixo do MIN, desliga dentro da faixa segura).
CO2 controla INJ (liga abaixo do MIN, desliga dentro da faixa segura).

UMI e CO2 não tem atuador de reducao na especificação da estufa, por
isso só ligam o atuador respectivo quando a leitura fica baixa demais.

## Observações de implementação

Toda saída de log usa `std::cout.setf(std::ios::unitbuf)` no início do
main pra forçar o flush após cada linha, garantindo que a troca de
mensagens apareca na tela em tempo real, mesmo se a saída for
redirecionada pra um arquivo durante testes.

O parsing de mensagem lê o socket byte a byte até encontrar a
sequência `\n\n`, extrai o campo TAMANHO do header, e só depois lê
exatamente essa quantidade de bytes de payload, evitando problemas de
mensagens coladas no fluxo TCP.