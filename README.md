# DinLooper

DinLooper é uma loop station VST3 criada para tocar ao vivo.

A ideia é simples: gravar loops e overdubs rapidamente, sem interromper a
performance e sem precisar navegar por telas complicadas. O plugin foi pensado
principalmente para uso no REAPER e pode ser controlado com mouse, MIDI, OSC,
pedal ou Stream Deck.

## O que ele faz

- Grava loops estéreo de até 3 minutos.
- Adiciona overdubs como layers independentes.
- Permite até 16 layers tocando ao mesmo tempo.
- Oferece volume, medidor, Mute, Solo e Delete para cada layer.
- Recupera layers apagadas usando Undo e Redo.
- Inicia a gravação imediatamente ou espera um sinal de áudio ou MIDI.
- Permite ajustar visualmente o threshold do trigger de áudio.
- Mostra a posição e a duração do loop.
- Possui medidores estéreo de entrada e saída.
- Possui volume master e Audio Thru opcional.
- Pode ser controlado pelo sistema Learn do REAPER.
- Possui interface redimensionável.

## Começando rapidamente

1. Insira o DinLooper em uma track do REAPER.
2. Escolha o modo de trigger.
3. Pressione **REC**.
4. Toque ou cante.
5. Pressione **FINISH** para concluir.
6. O loop começa a tocar automaticamente.
7. Pressione **REC** novamente para gravar um overdub.

Cada overdub aparece como uma nova layer, com seus próprios controles.

## Modos de trigger

O menu **Trigger** define quando a gravação começa:

- **Instant:** começa assim que REC é pressionado.
- **Audio + MIDI:** começa com o primeiro sinal válido de áudio ou MIDI.
- **Audio Only:** espera o áudio ultrapassar o threshold.
- **MIDI Only:** espera uma mensagem Note On.

O threshold pode ser ajustado arrastando a linha azul no medidor de entrada.

## Controles principais

- **REC:** prepara ou inicia uma gravação. Durante a gravação, muda para
  **FINISH** e confirma o áudio gravado.
- **REC PEDAL:** funciona como REC, mas permite concluir a gravação com o pedal
  de sustain, tanto ao pressionar quanto ao soltar o pedal.
- **PLAY:** volta a reproduzir um loop parado.
- **STOP:** salva a gravação atual, quando houver, e para a reprodução.
- **CANCEL:** descarta somente a gravação atual. As layers anteriores continuam
  tocando.
- **REWIND:** volta ao início do loop.
- **UNDO / REDO:** desfazem ou refazem a última operação com layers.
- **RESET:** apaga a sessão atual e reinicia o looper.

## Layers

Cada nova gravação recebe um número próprio e aparece no mixer central.

Em cada layer é possível:

- acompanhar o nível do áudio;
- ajustar o volume;
- ativar Mute;
- ativar Solo;
- apagar usando o botão X.

Uma layer apagada pode ser recuperada com Undo enquanto seu espaço não tiver
sido usado por uma nova gravação. O limite é de 16 layers ativas, mas a
numeração pode continuar aumentando durante a sessão.

## Audio Thru

Por padrão, o **Audio Thru fica desligado**. Nesse modo, a saída do plugin contém
somente os loops gravados, evitando duplicar o áudio que já está sendo ouvido
pelo REAPER.

Ative **Audio Thru** se quiser que o DinLooper também envie o áudio original da
entrada para a saída.

## REAPER, MIDI e OSC

Os principais controles e os volumes, Mutes e Solos das 16 posições de layer
aparecem como parâmetros do VST3. Isso permite usar:

- MIDI Learn;
- OSC;
- pedais;
- controladores;
- Stream Deck;
- automação do REAPER.

## Instalação no Windows

1. Baixe o arquivo mais recente na página de
   [Releases](https://github.com/randomicidio/DinLooper/releases).
2. Extraia a pasta `DinLooper.vst3`.
3. Copie essa pasta para:
   `C:\Program Files\Common Files\VST3`
   ou para outra pasta VST3 configurada no REAPER.
4. Abra o REAPER e faça um novo scan de plugins, se necessário.

O pacote atual é destinado ao Windows x64.

## Limitações atuais

O DinLooper ainda está em desenvolvimento.

- O áudio das layers ainda não é restaurado ao fechar e reabrir o projeto.
- Os ajustes dos parâmetros são salvos, mas os loops gravados ainda não.
- Sincronização com BPM, quantização, importação e exportação serão adicionadas
  em etapas futuras.

Antes de usar em uma apresentação importante, faça testes com seu computador,
interface de áudio, controlador e projeto do REAPER.

## Versão atual

[DinLooper v1.0.0 Alpha 2](https://github.com/randomicidio/DinLooper/releases/tag/v1.0.0-alpha.2)

## Desenvolvimento

O DinLooper é um projeto aberto, desenvolvido em C++ com JUCE.

Código-fonte, histórico de mudanças e versões estão disponíveis neste
repositório.
