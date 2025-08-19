<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=02A6F4&height=120&section=header"/>
<h1 align="center">Embarcatech - Projeto Monitor Interativo de Cor e Luz utilizando a placa BitDogLab </h1>

## Objetivo do Projeto

Desenvolver um sistema embarcado interativo utilizando a placa BitDogLab com Raspberry Pi Pico W, que realiza a leitura em tempo real de um sensor de cor (GY-33) e um sensor de luminosidade (GY-302). O sistema fornece respostas visuais dinâmicas através de uma matriz de LEDs WS2812 e de um display OLED, além de emitir alertas sonoros com um buzzer.

## 🗒️ Lista de requisitos

- **Leitura Contínua de Sensores:** Coleta dados de cor (RGB) do GY-33 e de iluminância (Lux) do GY-302;
- **Interface de Usuário com Menu:** Um menu interativo de 3 telas (Menu Principal, Status, Valores RGB) é exibido no display OLED e controlado pelos botões A, B e Joystick;
- **Atuadores Visuais Sincronizados:**
  - A matriz de LEDs WS2812 reflete a cor detectada pelo sensor;
  - O brilho da matriz é ajustado proporcionalmente à luminosidade do ambiente.
- **Alertas Sonoros Inteligentes:** O buzzer emite apitos intermitentes em situações de alerta, como baixa luminosidade ou detecção de cores específicas (vermelho intenso).
- **Código Organizado:** O projeto foi desenvolvido em C com o Pico SDK, seguindo as boas práticas de estruturação e comentários.
- **Técnicas Implementadas:** Utilização de I2C para múltiplos sensores e display, PIO para controle preciso da matriz WS2812, PWM para o buzzer, e interrupções de GPIO para uma navegação de menu responsiva com debounce de software.

## 🛠 Tecnologias

1. **Microcontrolador:** Raspberry Pi Pico W (na BitDogLab).
2. **Sensor de Cor:** Módulo GY-33 (TCS34725).
3. **Sensor de Luz:** Módulo GY-302 (BH1750).
4. **Display:** OLED SSD1306 128x64 pixels, via I2C (i2c1, GPIO 14/15).
5. **Matriz de LEDs:** WS2812 5x5, via PIO (GPIO 7).
6. **Buzzer:** Atuador sonoro, via PWM (GPIO 10).
7. **Botões:**
   - **Botão A (GPIO 5):** Navega para a tela de Status.
   - **Botão B (GPIO 6):** Navega para a tela de Valores.
   - **Botão do Joystick (GPIO 22):** Retorna ao Menu Principal.
8. **Linguagem de Programação:** C
9. **Framework:** Pico SDK


## 🔧 Funcionalidades Implementadas:

**Funções dos Componentes**

- **Display OLED (SSD1306):** 
  - **Tela de Menu:** Apresenta as opções de navegação;
  - **Tela de Status:** Exibe em tempo real a luminosidade (em Lux), o nome da cor detectada (exp: "Azul", "Amarelo") e o estado atual do sistema ("Normal", "Luz Baixa");
  - **Tela de Valores:** Mostra os valores numéricos (0-255) das componentes Vermelho, Verde e Azul lidas pelo sensor;
- **Matriz de LEDs (WS2812):**
  - **Cor Dinâmica:** A cor dos 25 LEDs muda para corresponder à cor do objeto posicionado em frente ao sensor GY-33;
  - **Brilho Adaptativo:** A intensidade da matriz aumenta ou diminui de acordo com a luz ambiente medida pelo GY-302, apagando-se completamente no escuro.
- **Buzzer:**
    - Emite um beep intermitente a cada 2 segundos;
    - É acionado em duas condições de alerta: luminosidade abaixo de 50 Lux ou detecção de vermelho intenso.
- **Lógica de Cores:**
    - Uma função customizada analisa os valores RGB e consegue identificar 6 cores (Vermelho, Verde, Azul, Amarelo, Ciano, Magenta), além de "Branco" e "Escuro", tornando o sistema mais preciso.

## 🚀 Passos para Compilação e Upload do projeto Ohmímetro com Matriz de LEDs

1. **Instale o Ambiente**:
- Configure o Pico SDK.
  
2. **Compile o Código**:
- **Crie uma pasta `build`**:
   
   ```bash
   mkdir build
   cd build
   cmake ..
   make

3. **Transferir o firmware para a placa:**

- Conectar a placa BitDogLab ao computador via USB.
- Copiar o arquivo .uf2 gerado para o drive da placa.

4. **Testar o projeto**

- Após o upload, a placa irá reiniciar e o menu principal aparecerá no display.
- Use os botões A e B para navegar pelas telas e o Joystick para voltar ao menu.
- Aproxime objetos coloridos do sensor GY-33 e observe a matriz de LEDs mudar de cor.
- Cubra e ilumine o sensor GY-302 para ver o brilho da matriz se adaptar e o alerta de "Luz Baixa" ser acionado.


🛠🔧🛠🔧🛠🔧


## 🎥 Demonstração: 

- Para ver o funcionamento do projeto, acesse o vídeo de demonstração em:

