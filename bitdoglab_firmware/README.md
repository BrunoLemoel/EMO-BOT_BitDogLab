# 🤖 Emo-Bot — BitDogLab (Raspberry Pi Pico W)

Firmware nativo em C desenvolvido com o **Raspberry Pi Pico SDK** para a placa **BitDogLab** (EmbarcaTech), substituindo o hardware original (ESP32) e tirando proveito dos recursos integrados da placa.

---

## 📋 Mapeamento de Hardware (BitDogLab)

| Componente | Pino GPIO | Função no Emo-Bot |
|---|---|---|
| **Display OLED SSD1306** | **I2C1** (SDA: GP14, SCL: GP15) | 10 Expressões faciais animadas, piscada suave e mensagens |
| **Matriz 5x5 WS2812B** | **GPIO 7** (PIO0) | Boca expressiva / ícones animados combinando com a emoção |
| **LED RGB** | **R: GP13, G: GP11, B: GP12** | Cor temática da emoção atual |
| **Buzzer A** | **GPIO 21** (PWM) | Efeitos sonoros robóticos (chimes/beeps) ao trocar de humor |
| **Botão A** | **GPIO 5** (Pull-up) | Alterna manualmente entre as 10 expressões |
| **Botão B** | **GPIO 6** (Pull-up) | Dispara animação de piscar olhos e som de alerta |
| **Comunicação USB** | **USB CDC Nativo** (115200) | Recebe comandos do Python Dashboard no PC |

---

## ⚡ Como Gravar na BitDogLab (Pico W)

### Método 1: Drag-and-Drop do arquivo `.uf2` (Mais rápido!)
1. O arquivo binário já compilado está pronto em:
   `bitdoglab_firmware/build/bitdoglab_emo_bot.uf2`
2. Segure o botão **BOOTSEL** da Raspberry Pi Pico W na BitDogLab e conecte o cabo USB ao computador.
3. A placa aparecerá como um pendrive chamado **`RPI-RP2`**.
4. Arraste e solte o arquivo `bitdoglab_emo_bot.uf2` dentro da unidade.
5. A placa reiniciará instantaneamente executando o Emo-Bot!

### Método 2: Compilando via CMake / VS Code
No terminal (ou usando a extensão Raspberry Pi Pico no VS Code):
```bash
cd bitdoglab_firmware
cmake -B build -G Ninja -DPICO_BOARD=pico_w
cmake --build build
```

---

## 🎮 Teste Standalone (Sem o PC)
Assim que liga, a placa exibe a tela de inicialização com efeito sonoro e inicia no modo `normal`.
- Pressione o **Botão A** para alternar entre as 11 funções:
  `Normal` ➔ `Happy` ➔ `Sad` ➔ `Angry` ➔ `Surprised` ➔ `Love` ➔ `Confused` ➔ `Thinking` ➔ `Wink` ➔ `Sleepy` ➔ **`Onda Arco-Íris (Wave)`**
  - Ao chegar em **Onda Arco-Íris**, a matriz 5x5 executa o jogo de luzes fluido contínuo e só sai desse modo se o **Botão A** for pressionado novamente!
- Pressione o **Botão B** para testar a piscada suave e o bip.
- A cada 5 a 8 segundos o robô pisca os olhos automaticamente!

---

## 🖥️ Conexão com o Python Dashboard

1. Abra o dashboard no computador:
   ```bash
   python main.py
   ```
2. Na barra lateral esquerda em **Connection Mode**, selecione **USB Serial**.
3. Selecione a porta COM correspondente à Raspberry Pi Pico W (115200 baud).
4. Clique em **Connect USB**.
5. Teste clicando nos botões de emoções ou digite/fale com o robô!
   - A voz da IA falará no **alto-falante do seu computador**.
   - A BitDogLab expressará o sentimento simultaneamente no **OLED**, na **Matriz 5x5**, no **LED RGB** e no **Buzzer**!
   - Clique em **`Onda 🌈`** para ativar o modo contínuo de onda de luzes, ou **`Standby 💤`** para testar o modo ocioso.

---

## 💤 Modo Standby / Ocioso ("Brincando Consigo Mesmo")

Se o Emo-Bot ficar **3 minutos** sem receber nenhum comando serial e sem toque nos botões físicos, ele entra automaticamente no modo Standby interativo:

- 🌈 **Jogo de Luzes na Matriz 5x5 (Alterna a cada 60 segundos)**:
  - **Estilo 0 — Onda Arco-Íris (Wave)**: Cores cromáticas fluidas se propagando em ondas diagonais (60s).
  - **Estilo 1 — Vagalumes Cintilantes (Fireflies)**: Partículas mágicas que surgem e esmaecem suavemente (60s).
  - **Estilo 2 — Espiral Giratória (Spiral Orbit)**: Cometa de luz que percorre as bordas até o centro da matriz (60s).
  - **Estilo 3 — Equalizador Saltitante (Equalizer)**: Barras verticais que sobem e descem ritmicamente (60s).
  - **Estilo 4 — Coração Respirando (Breathing Heart)**: Pulsação suave com brilho senoidal em tons de magenta e vermelho (60s).
- 🎶 **Buzzer "Falante" em Volume Meio Baixo (Alterna a cada 2 minutos)**:
  - Acionamento via PWM com duty cycle reduzido (~8%), simulando que o robô está falando/balbuciando consigo mesmo sem incomodar.
  - Alterna ciclicamente entre 5 variações melódicas diferentes a cada 2 minutos:
    1. *Hmm-hum* (curioso ascendente)
    2. *Bibip-bop* (conversinha alegre)
    3. *Uuu-wa?* (pergunta suave)
    4. *Pip-pip-peep* (estalos robóticos brincalhões)
    5. *Ooh-hmm* (suspiro acolhedor)
- 🎭 **Alternância de Expressões**:
  - Muda a cada 60 segundos acompanhando o estilo de luz: `Normal` ➔ `Feliz` ➔ `Confuso` ➔ `Pensando` ➔ `Piscando`, com piscadas naturais nos olhos a cada 7 segundos.
- 💬 **Mensagem "HELLO!" Animada**:
  - Após os 5 estilos, o texto "HELLO!" em tamanho 2x se move suavemente da esquerda para a direita do display OLED até sumir, reiniciando o ciclo.
- ⚡ **Despertar Instantâneo (Wake-up)**:
  - Ao receber qualquer comando USB ou ao pressionar o Botão A ou B, o robô acorda instantaneamente tocando um jingle alegre de despertar e voltando ao modo normal.


