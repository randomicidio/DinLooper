# DinLooper

DinLooper is a live-performance VST3 loop station built with C++ and JUCE,
designed primarily for use inside REAPER.

O DinLooper é uma loop station VST3 criada para performances ao vivo. O foco
do projeto é oferecer operação rápida, previsível e confiável, permitindo que
o músico grave e controle loops com poucos comandos.

## Principais recursos

- Gravação e reprodução de loops estéreo.
- Overdub em layers independentes.
- Undo e redo de layers, incluindo a primeira gravação.
- Trigger de gravação instantâneo, por áudio, por MIDI ou por áudio + MIDI.
- Threshold configurável para o trigger de áudio.
- Fluxo `REC PEDAL`, encerrado pelo pedal de sustain MIDI CC64.
- Controles REC, REC PEDAL, PLAY, STOP, REWIND, UNDO, REDO e RESET.
- Parâmetros automatizáveis compatíveis com REAPER Learn e controle OSC.
- Barra de progresso, duração do loop e indicador de estado.
- Até 16 layers, com buffers preparados fora da thread de áudio.
- Duração máxima de 3 minutos por layer.

## Comportamento de áudio

O DinLooper não funciona como Audio Thru. O sinal recebido na entrada é usado
somente para gravação e detecção de trigger. A saída contém exclusivamente os
loops produzidos pelo plugin, evitando a duplicação do áudio que já está sendo
reproduzido pelo REAPER.

## Instalação

1. Baixe o pacote mais recente na página de
   [Releases](https://github.com/randomicidio/DinLooper/releases).
2. Extraia a pasta `DinLooper.vst3`.
3. Copie a pasta para o diretório VST3 usado pelo REAPER, como
   `C:\Program Files\Common Files\VST3`, ou para uma pasta personalizada.
4. Faça um rescan dos plugins no REAPER.

## Estado do projeto

O projeto está em fase alpha. Teste cuidadosamente antes de utilizá-lo em uma
apresentação ao vivo.

Versão atual:
[DinLooper v1.0.0 Alpha 1](https://github.com/randomicidio/DinLooper/releases/tag/v1.0.0-alpha.1).

## Desenvolvimento

O projeto utiliza JUCE 8, Visual Studio 2022 e C++17. A arquitetura principal é:

`PluginEditor -> PluginProcessor -> LooperEngine`

O `LooperController` permanece fora do fluxo principal por enquanto.
