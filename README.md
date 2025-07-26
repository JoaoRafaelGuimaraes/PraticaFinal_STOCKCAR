# Stock Car em FPG FPGA 

Projeto Final do Curso Arquitetura de Alto desempenho 

Departamento de Computação - UFSCar

Professor Dr. Emerson Carlos Pedrino

## Grupo

Gabriel Andreazi Bertho, 790780 

João Rafael de Freitas Guimarães, 800295 

Caroline Elisa Duarte de Souza, 795565
## Descrição

O *Stock Car* é um jogo de corrida 2D desenvolvido para plataformas embarcadas com FPGA, utilizando interface VGA e programação em C. O objetivo é simular uma pista em movimento, onde o jogador controla um carro para desviar de obstáculos, acumulando quilômetros e enfrentando dificuldade crescente.

## Demonstração em Vídeo

Assista ao funcionamento completo do jogo:

> 🎥 [Demo no YouTube](https://youtu.be/GpylnnH9F2Q)

## Funcionalidades Principais

* **Controle de Carro:** Movimento lateral para esquerda e direita.
* **Geração Procedural de Pista:** Pistas armazenadas em vetores circulares, com curvas aleatórias.
* **Obstáculos Dinâmicos:** Carros inimigos surgem na pista e se deslocam verticalmente.
* **Modo "Túnel":** Iluminação limitada para criar efeito de faróis, alternando a cada 10 segundos.
* **Contadores de Tempo e Distância:** Apresentação de tempo decorrido e distância percorrida (km).
* **Detecção de Colisão:** Verificação de colisão contra bordas da pista e obstáculos.

## Tecnologias Utilizadas

* **Linguagem C** para controle direto de hardware.
* **FPGA** com saída VGA.
* **Quartus Prime 18.1 Lite Edition** para síntese e compilação do design.
* **Altera Monitor Program (AMP)** para download do bitstream na placa via JTAG.

## Requisitos

* Kit FPGA compatível com saída VGA (por exemplo, placa DE1-SoC).
* Quartus Prime 18.1 Lite Edition instalado.
* Altera Monitor Program (AMP) configurado no mesmo diretório do Quartus.
* Cabo USB-Blaster (ou equivalente) para programação JTAG.

## Instalação e Execução

1. **Clone o repositório**

2. **Crie um projeto no Intel Monitor Program com o template Video-out**
   
4. **Substitua o video.c pelo arquivo deste repositório**

5. **Inicie o Jogo**

   * Conecte o monitor VGA à saída da FPGA.
   * Pressione **KEY2** para iniciar.

## Controles

* **KEY0:** Mover carro para a esquerda
* **KEY1:** Mover carro para a direita
* **KEY2:** Iniciar ou reiniciar o jogo

## Como Jogar

1. Após compilação no IMP, ligue o monitor VGA.
2. Pressione **KEY2** para começar.
3. Use **KEY0** e **KEY1** para evitar colisões.
4. Sobreviva o máximo que puder e acompanhe km e tempo na tela.
5. Ao colidir, aparecerá "GAME OVER". Pressione **KEY2** para recomeçar.




