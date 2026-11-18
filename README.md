# SmartHome — protótipo ESP32 e Losant

Protótipo para enviar dados elétricos à Losant e receber comandos de saída. Por padrão, as leituras são simuladas e a saída física do relé fica desabilitada.

## Primeiro uso

1. No Arduino IDE, instale o suporte a ESP32 e as bibliotecas [Losant MQTT Arduino](https://github.com/Losant/losant-mqtt-arduino) e ArduinoJson compatível com a versão 6. Para o modo de sensor, instale também [PZEM-004T v3.0](https://github.com/mandulaj/PZEM-004T-v30).
2. Na pasta `SmartHomePrototipo`, copie `secrets.example.h` para `secrets.h` e preencha as credenciais **novas** localmente. Nunca envie `secrets.h` ao GitHub. O `.gitignore` deve permanecer na raiz do repositório.
3. Abra `SmartHomePrototipo.ino`, selecione a placa e a porta e carregue o programa. Abra o monitor serial em 115200 baud.
4. Configure na Losant os atributos `voltage`, `current`, `power`, `frequency`, `powerFact`, `simulated` e `relayOn` com tipos adequados. Envie comandos `on`, `off` ou `toggle` para alterar o estado solicitado.

Em simulação, os valores não são medições; `simulated` é verdadeiro. `relayOn` é o estado solicitado pelo software, sem confirmação física do relé. O estado volta a desligado ao reiniciar; em desconexão ele mantém a última solicitação. A saída física também pode variar durante o boot, dependendo do módulo.

## Sensor e relé

O diagrama fornecido parece mostrar um medidor com transformador de corrente. A versão exata não está confirmada. O suporte opcional do código é **somente para PZEM-004T v3.0**. Confirme o modelo e os níveis de tensão antes de alterar `USE_PZEM` para `1`; GPIO 16 e 17 são exemplos de RX/TX e devem ser conferidos na montagem. A alimentação de 5 V de um módulo não significa que sua saída serial seja segura para os pinos de 3,3 V do ESP32; use interface adequada conforme as especificações do modelo.

`ENABLE_RELAY` começa em `0`. Antes de habilitar, confirme a polaridade ativa e o circuito do módulo, e teste primeiro sem carga de rede. A ilustração não demonstra uma ligação completa e segura do relé; não a use como esquema de ligação à rede. Rede elétrica pode causar choque ou incêndio: montagem e verificação por profissional qualificado, com proteção, isolação e componentes adequados.

## Observações

A conexão usa o certificado CA presente no projeto original: verifique a cadeia TLS atual da Losant antes de usar, pois ela pode mudar. A reconexão é tentada periodicamente, mas a chamada de conexão da biblioteca pode bloquear por um intervalo. O protótipo não foi testado em hardware nem validado como instrumento de medição ou sistema de proteção.

As credenciais anteriormente publicadas devem ser revogadas/trocadas. Substituir o arquivo atual não apaga os segredos de commits antigos; trate também o histórico e eventuais cópias.
